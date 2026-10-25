#include "Global.h"

// =====================================================
// Configuration
// =====================================================

PuckTrackingConfig puckTrackingConfig = {
  true,     // robotOnLeftSide
  false,    // flipCameraX - confirmed final orientation
  false,    // flipCameraY

  0.03f,    // roiMinXRatio
  0.97f,    // roiMaxXRatio
  0.05f,    // roiMinYRatio
  0.95f,    // roiMaxYRatio

  0.20f,    // defenseLineXRatio

  0.70f,    // velocityFilterAlpha
  0.20f,    // accelerationFilterAlpha

  10.0f,    // minimumApproachSpeedXPixelsPerSecond
  3500.0f,  // maximumValidPuckSpeedPixelsPerSecond
  0.14f,    // minimumDetectionConfidence

  0.01f,    // minimumDeltaTimeSeconds
  0.40f,    // maximumDeltaTimeSeconds

  0.70f,    // level 1 maximum intercept time
  1.00f,    // level 2 maximum intercept time
  1.45f,    // level 3 maximum intercept time
  2.20f,    // level 4 maximum intercept time

  0.04f,    // minimumTargetChangeCm
  5UL,      // minimumRobotCommandIntervalMs
  150UL,    // maximumTargetAgeMs
  2,        // lostFramesBeforeReset

  false,    // useAccelerationInPrediction
  0.24f     // hitWindowSeconds
};

PuckTrackingState puckTrackingState = {};
PendingRobotTarget pendingRobotTarget = {};

unsigned long lastTrackingRobotCommandMillis = 0;
unsigned long pendingServoHitSinceMillis = 0;
unsigned long lastServoHitMillis = 0;

float lastTrackingTargetXcm = 0.0f;
float lastTrackingTargetYcm = 0.0f;

bool hasLastTrackingTarget = false;
bool hitIssuedForCurrentApproach = false;
bool pendingServoHit = false;
bool lastObservationWasStationary = false;
bool stationaryPuckActionActive = false;

uint8_t notApproachingFrameCount = 0;
uint8_t trackingSerialDivider = 0;


// Dynamic and stationary-puck settings

const bool USE_DYNAMIC_XY_TARGET = true;

const float DYNAMIC_LOOKAHEAD_SECONDS =
  0.14f;


const float ROBOT_AREA_EDGE_RATIO =
  0.42f;


const float STATIONARY_SPEED_MAX_PIXELS_PER_SECOND =
  34.0f;

const unsigned long STATIONARY_SERVO_RETRY_MS =
  850UL;

const unsigned long MAX_SERVO_CORRECTION_WAIT_MS =
  70UL;


// Mathematical helpers

float clampFloatValue(
  float value,
  float minimumValue,
  float maximumValue
) {
  if (value < minimumValue) {
    return minimumValue;
  }

  if (value > maximumValue) {
    return maximumValue;
  }

  return value;
}


float linearInterpolate(
  float previousValue,
  float newValue,
  float alpha
) {
  alpha = clampFloatValue(
    alpha,
    0.0f,
    1.0f
  );

  return previousValue +
    alpha *
    (
      newValue -
      previousValue
    );
}


float reflectValueBetween(
  float value,
  float minimumValue,
  float maximumValue
) {
  float range =
    maximumValue -
    minimumValue;

  if (range <= 0.0f) {
    return minimumValue;
  }

  float period =
    2.0f *
    range;

  float shifted =
    fmodf(
      value -
      minimumValue,
      period
    );

  if (shifted < 0.0f) {
    shifted += period;
  }

  if (shifted > range) {
    shifted =
      period -
      shifted;
  }

  return minimumValue +
    shifted;
}


float getMaximumInterceptTimeForSelectedLevel() {
  switch (selectedLevel) {
    case 1:
      return
        puckTrackingConfig
          .level1MaximumInterceptTimeSeconds;

    case 2:
      return
        puckTrackingConfig
          .level2MaximumInterceptTimeSeconds;

    case 3:
      return
        puckTrackingConfig
          .level3MaximumInterceptTimeSeconds;

    case 4:
      return
        puckTrackingConfig
          .level4MaximumInterceptTimeSeconds;

    default:
      return
        puckTrackingConfig
          .level2MaximumInterceptTimeSeconds;
  }
}



// Pixel coordinates -> robot coordinates

bool pixelPointToRobotCm(
  float pixelX,
  float pixelY,
  int imageWidth,
  int imageHeight,
  float &robotXcm,
  float &robotYcm
) {
  if (
    imageWidth <= 0 ||
    imageHeight <= 0
  ) {
    return false;
  }

  float minimumPixelX =
    imageWidth *
    puckTrackingConfig.roiMinXRatio;

  float maximumPixelX =
    imageWidth *
    puckTrackingConfig.roiMaxXRatio;

  float minimumPixelY =
    imageHeight *
    puckTrackingConfig.roiMinYRatio;

  float maximumPixelY =
    imageHeight *
    puckTrackingConfig.roiMaxYRatio;

  float pixelRangeX =
    maximumPixelX -
    minimumPixelX;

  float pixelRangeY =
    maximumPixelY -
    minimumPixelY;

  if (
    pixelRangeX <= 0.0f ||
    pixelRangeY <= 0.0f
  ) {
    return false;
  }

  float normalizedX =
    (
      pixelX -
      minimumPixelX
    ) /
    pixelRangeX;

  float normalizedY =
    (
      pixelY -
      minimumPixelY
    ) /
    pixelRangeY;

  normalizedX = clampFloatValue(
    normalizedX,
    0.0f,
    1.0f
  );

  normalizedY = clampFloatValue(
    normalizedY,
    0.0f,
    1.0f
  );

  if (
    puckTrackingConfig.flipCameraX
  ) {
    normalizedX =
      1.0f -
      normalizedX;
  }

  if (
    puckTrackingConfig.flipCameraY
  ) {
    normalizedY =
      1.0f -
      normalizedY;
  }

  robotXcm =
    MIN_X_CM +
    normalizedX *
    (
      MAX_X_CM -
      MIN_X_CM
    );

  robotYcm =
    MIN_Y_CM +
    normalizedY *
    (
      MAX_Y_CM -
      MIN_Y_CM
    );

  robotXcm = constrain(
    robotXcm,
    MIN_X_CM,
    MAX_X_CM
  );

  robotYcm = constrain(
    robotYcm,
    MIN_Y_CM,
    MAX_Y_CM
  );

  return true;
}


// =====================================================
// State helpers
// =====================================================

bool isPuckInsideRobotArea(
  float pixelX,
  int imageWidth
) {
  if (imageWidth <= 0) {
    return false;
  }

  if (
    puckTrackingConfig.robotOnLeftSide
  ) {
    return pixelX <=
      imageWidth *
      ROBOT_AREA_EDGE_RATIO;
  }

  return pixelX >=
    imageWidth *
    (
      1.0f -
      ROBOT_AREA_EDGE_RATIO
    );
}


bool isPuckMovingTowardRobot(
  float velocityX
) {
  if (
    puckTrackingConfig.robotOnLeftSide
  ) {
    return velocityX <
      -puckTrackingConfig
        .minimumApproachSpeedXPixelsPerSecond;
  }

  return velocityX >
    puckTrackingConfig
      .minimumApproachSpeedXPixelsPerSecond;
}


bool isPuckStationaryOrVerySlow() {
  return (
    lastObservationWasStationary ||
    !puckTrackingState.hasVelocity ||
    puckTrackingState.speed <=
      STATIONARY_SPEED_MAX_PIXELS_PER_SECOND
  );
}


void resetPuckTracking() {
  puckTrackingState =
    PuckTrackingState();

  pendingRobotTarget =
    PendingRobotTarget();

  lastTrackingRobotCommandMillis =
    0;

  pendingServoHitSinceMillis =
    0;

  lastServoHitMillis =
    0;

  lastTrackingTargetXcm =
    0.0f;

  lastTrackingTargetYcm =
    0.0f;

  hasLastTrackingTarget =
    false;

  hitIssuedForCurrentApproach =
    false;

  pendingServoHit =
    false;

  lastObservationWasStationary =
    false;

  stationaryPuckActionActive =
    false;

  notApproachingFrameCount =
    0;

  trackingSerialDivider =
    0;

  puckSpeed = 0;

  Serial.println(
    "Puck tracking state reset"
  );
}


void init_puck_tracking() {
  resetPuckTracking();

  Serial.println(
    "Dynamic XY and stationary-puck tracking initialized"
  );
}


void resetCurrentApproach() {
  /*
    Do not cancel a servo request or a movement that has already
    been accepted. This latch is one of the fixes for missing hits.
  */
  if (
    pendingServoHit ||
    pendingRobotTarget.valid ||
    robotMovementBusy
  ) {
    return;
  }

  hitIssuedForCurrentApproach =
    false;

  pendingServoHitSinceMillis =
    0;

  hasLastTrackingTarget =
    false;

  lastTrackingRobotCommandMillis =
    0;

  stationaryPuckActionActive =
    false;
}


void clearCurrentApproachIfNeeded(
  float velocityX
) {
  if (
    isPuckMovingTowardRobot(
      velocityX
    )
  ) {
    notApproachingFrameCount =
      0;

    return;
  }

  if (
    robotMovementBusy ||
    pendingServoHit ||
    pendingRobotTarget.valid
  ) {
    return;
  }

  if (
    notApproachingFrameCount < 3
  ) {
    notApproachingFrameCount++;
  }

  if (
    notApproachingFrameCount >= 3
  ) {
    resetCurrentApproach();
  }
}


bool targetChangedEnough(
  float targetXcm,
  float targetYcm
) {
  if (!hasLastTrackingTarget) {
    return true;
  }

  float differenceX =
    fabsf(
      targetXcm -
      lastTrackingTargetXcm
    );

  float differenceY =
    fabsf(
      targetYcm -
      lastTrackingTargetYcm
    );

  return (
    differenceX >=
      puckTrackingConfig
        .minimumTargetChangeCm ||
    differenceY >=
      puckTrackingConfig
        .minimumTargetChangeCm
  );
}


bool shouldLockCurrentMovementForServo() {
  if (
    !pendingServoHit ||
    pendingServoHitSinceMillis == 0
  ) {
    return false;
  }

  return (
    millis() -
      pendingServoHitSinceMillis >=
    MAX_SERVO_CORRECTION_WAIT_MS
  );
}


void queueRobotTarget(
  float targetXcm,
  float targetYcm,
  float interceptTimeSeconds,
  unsigned long now,
  bool forceQueue
) {
  bool commandIntervalPassed =
    now -
      lastTrackingRobotCommandMillis >=
    puckTrackingConfig
      .minimumRobotCommandIntervalMs;

  bool targetIsDifferent =
    targetChangedEnough(
      targetXcm,
      targetYcm
    );

  if (
    forceQueue ||
    (
      commandIntervalPassed &&
      targetIsDifferent
    )
  ) {
    pendingRobotTarget.valid =
      true;

    pendingRobotTarget.xCm =
      targetXcm;

    pendingRobotTarget.yCm =
      targetYcm;

    pendingRobotTarget
      .interceptTimeSeconds =
      interceptTimeSeconds;

    pendingRobotTarget
      .generatedAtMillis =
      now;

    lastTrackingRobotCommandMillis =
      now;

    lastTrackingTargetXcm =
      targetXcm;

    lastTrackingTargetYcm =
      targetYcm;

    hasLastTrackingTarget =
      true;
  }
}


void requestServoHit(
  unsigned long now
) {
  pendingServoHit =
    true;

  if (
    pendingServoHitSinceMillis == 0
  ) {
    pendingServoHitSinceMillis =
      now;
  }
}



// =====================================================
// Immediate action for every puck inside the robot area
// =====================================================

bool queueImmediateRobotAreaAction() {
  if (
    !puckTrackingState.hasPosition ||
    !isPuckInsideRobotArea(
      puckTrackingState.positionX,
      puckTrackingState.imageWidth
    )
  ) {
    return false;
  }

  float targetXcm = 0.0f;
  float targetYcm = 0.0f;

  if (!pixelPointToRobotCm(
        puckTrackingState.positionX,
        puckTrackingState.positionY,
        puckTrackingState.imageWidth,
        puckTrackingState.imageHeight,
        targetXcm,
        targetYcm
      )) {
    return false;
  }

  unsigned long now = millis();

  /*
    Always use the newest measured puck position.
    This branch does not wait for velocity or acceleration.
  */
  queueRobotTarget(
    targetXcm,
    targetYcm,
    0.0f,
    now,
    true
  );

  /*
    The servo request is latched. If a newer camera target arrives while
    the robot is moving, the XY movement is corrected first and the hit
    happens only after the newest target is reached.
  */
  requestServoHit(now);
  stationaryPuckActionActive =
    isPuckStationaryOrVerySlow();

  return true;
}


// =====================================================
// Stationary puck inside robot area
// =====================================================

bool queueStationaryPuckAction() {
  if (
    !puckTrackingState.hasPosition ||
    !isPuckInsideRobotArea(
      puckTrackingState.positionX,
      puckTrackingState.imageWidth
    ) ||
    !isPuckStationaryOrVerySlow()
  ) {
    return false;
  }

  float targetXcm = 0.0f;
  float targetYcm = 0.0f;

  if (!pixelPointToRobotCm(
        puckTrackingState.positionX,
        puckTrackingState.positionY,
        puckTrackingState.imageWidth,
        puckTrackingState.imageHeight,
        targetXcm,
        targetYcm
      )) {
    return false;
  }

  unsigned long now =
    millis();

  bool servoCooldownPassed =
    (
      lastServoHitMillis == 0 ||
      now -
        lastServoHitMillis >=
      STATIONARY_SERVO_RETRY_MS
    );

  bool robotNeedsMovement =
    (
      fabsf(
        targetXcm -
        currentX
      ) >= 0.04f ||
      fabsf(
        targetYcm -
        currentY
      ) >= 0.04f
    );

  /*
    If the puck is not adjacent, move exactly to it.
    If it is already adjacent, force the same XY into the queue,
    so processPuckTracking() reaches the servo path.
  */
  if (
    robotNeedsMovement ||
    servoCooldownPassed
  ) {
    queueRobotTarget(
      targetXcm,
      targetYcm,
      0.0f,
      now,
      true
    );
  }

  if (servoCooldownPassed) {
    stationaryPuckActionActive =
      true;

    hitIssuedForCurrentApproach =
      false;

    requestServoHit(
      now
    );
  }

  trackingSerialDivider++;

  if (
    trackingSerialDivider >= 5
  ) {
    trackingSerialDivider = 0;

    Serial.print(
      "Stationary puck target X="
    );
    Serial.print(targetXcm, 2);
    Serial.print(" Y=");
    Serial.print(targetYcm, 2);
    Serial.print(" move=");
    Serial.print(
      robotNeedsMovement
        ? "YES"
        : "NO"
    );
    Serial.print(" servo=");
    Serial.println(
      servoCooldownPassed
        ? "QUEUED"
        : "COOLDOWN"
    );
  }

  return true;
}


// =====================================================
// Dynamic moving-puck target
// =====================================================

void calculateAndQueueRobotTarget() {
  if (
    currentScreen !=
      SCREEN_LIVE_GAME ||
    !puckTrackingState.hasPosition
  ) {
    return;
  }

  /*
    Highest priority: as soon as the puck enters the robot area,
    move directly to its newest measured position. Do not wait for
    velocity, acceleration or several additional frames.
  */
  if (queueImmediateRobotAreaAction()) {
    return;
  }

  /*
    Kept as a fallback for a stationary puck near the robot-area edge.
  */
  if (queueStationaryPuckAction()) {
    return;
  }

  if (
    !puckTrackingState.hasVelocity
  ) {
    return;
  }

  float velocityX =
    puckTrackingState.velocityX;

  float velocityY =
    puckTrackingState.velocityY;

  clearCurrentApproachIfNeeded(
    velocityX
  );

  if (
    !isPuckMovingTowardRobot(
      velocityX
    )
  ) {
    return;
  }

  if (
    hitIssuedForCurrentApproach
  ) {
    return;
  }

  float imageWidth =
    (float)
    puckTrackingState.imageWidth;

  float imageHeight =
    (float)
    puckTrackingState.imageHeight;

  if (
    imageWidth <= 0.0f ||
    imageHeight <= 0.0f
  ) {
    return;
  }

  float defenseLinePixelX =
    puckTrackingConfig.robotOnLeftSide
      ? imageWidth *
          puckTrackingConfig
            .defenseLineXRatio
      : imageWidth *
          (
            1.0f -
            puckTrackingConfig
              .defenseLineXRatio
          );

  float timeUntilDefense =
    (
      defenseLinePixelX -
      puckTrackingState.positionX
    ) /
    velocityX;

  float maximumInterceptTime =
    getMaximumInterceptTimeForSelectedLevel();

  if (
    timeUntilDefense <= 0.0f ||
    timeUntilDefense >
      maximumInterceptTime
  ) {
    return;
  }

  float targetPixelX;
  float targetPixelY;

  if (USE_DYNAMIC_XY_TARGET) {
    float predictionSeconds =
      fminf(
        DYNAMIC_LOOKAHEAD_SECONDS,
        timeUntilDefense
      );

    targetPixelX =
      puckTrackingState.positionX +
      velocityX *
      predictionSeconds;

    targetPixelY =
      puckTrackingState.positionY +
      velocityY *
      predictionSeconds;
  }
  else {
    targetPixelX =
      defenseLinePixelX;

    targetPixelY =
      puckTrackingState.positionY +
      velocityY *
      timeUntilDefense;
  }

  if (
    puckTrackingConfig
      .useAccelerationInPrediction
  ) {
    float predictionSeconds =
      fminf(
        DYNAMIC_LOOKAHEAD_SECONDS,
        timeUntilDefense
      );

    targetPixelX +=
      0.5f *
      puckTrackingState.accelerationX *
      predictionSeconds *
      predictionSeconds;

    targetPixelY +=
      0.5f *
      puckTrackingState.accelerationY *
      predictionSeconds *
      predictionSeconds;
  }

  float minimumPlayableX =
    imageWidth *
    puckTrackingConfig.roiMinXRatio;

  float maximumPlayableX =
    imageWidth *
    puckTrackingConfig.roiMaxXRatio;

  float minimumPlayableY =
    imageHeight *
    puckTrackingConfig.roiMinYRatio;

  float maximumPlayableY =
    imageHeight *
    puckTrackingConfig.roiMaxYRatio;

  targetPixelX =
    reflectValueBetween(
      targetPixelX,
      minimumPlayableX,
      maximumPlayableX
    );

  targetPixelY =
    reflectValueBetween(
      targetPixelY,
      minimumPlayableY,
      maximumPlayableY
    );

  float targetXcm = 0.0f;
  float targetYcm = 0.0f;

  if (!pixelPointToRobotCm(
        targetPixelX,
        targetPixelY,
        puckTrackingState.imageWidth,
        puckTrackingState.imageHeight,
        targetXcm,
        targetYcm
      )) {
    return;
  }

  unsigned long now =
    millis();

  queueRobotTarget(
    targetXcm,
    targetYcm,
    timeUntilDefense,
    now,
    false
  );

  /*
    The servo request is latched and remains active until hitPuck().
  */
  if (
    timeUntilDefense <=
      puckTrackingConfig
        .hitWindowSeconds
  ) {
    requestServoHit(
      now
    );
  }

  trackingSerialDivider++;

  if (
    trackingSerialDivider >= 6
  ) {
    trackingSerialDivider = 0;

    Serial.print("Moving target X=");
    Serial.print(targetXcm, 2);
    Serial.print(" Y=");
    Serial.print(targetYcm, 2);
    Serial.print(" vx=");
    Serial.print(velocityX, 1);
    Serial.print(" vy=");
    Serial.print(velocityY, 1);
    Serial.print(" defense t=");
    Serial.println(timeUntilDefense, 3);
  }
}


// =====================================================
// Update motion from one raw Python observation
// =====================================================

void updatePuckTracking(
  const PuckObservation &observation
) {
  if (!observation.detected) {
    puckTrackingState
      .lostFrameCount++;

    if (
      puckTrackingState
        .lostFrameCount >=
      puckTrackingConfig
        .lostFramesBeforeReset
    ) {
      puckTrackingState.hasPosition =
        false;

      puckTrackingState.hasVelocity =
        false;

      puckTrackingState.velocityX =
        0.0f;

      puckTrackingState.velocityY =
        0.0f;

      puckTrackingState.accelerationX =
        0.0f;

      puckTrackingState.accelerationY =
        0.0f;

      lastObservationWasStationary =
        false;

      /*
        Do not cancel a movement or hit that is already committed.
      */
      if (
        !pendingServoHit &&
        !pendingRobotTarget.valid &&
        !robotMovementBusy
      ) {
        resetCurrentApproach();
      }
    }

    return;
  }

  if (
    observation.imageWidth <= 0 ||
    observation.imageHeight <= 0 ||
    observation.confidence <
      puckTrackingConfig
        .minimumDetectionConfidence
  ) {
    return;
  }

  lastObservationWasStationary =
    observation.stationary;

  puckTrackingState.lostFrameCount =
    0;

  if (
    !puckTrackingState.hasPosition
  ) {
    puckTrackingState.hasPosition =
      true;

    puckTrackingState.hasVelocity =
      false;

    puckTrackingState.positionX =
      observation.xPixel;

    puckTrackingState.positionY =
      observation.yPixel;

    puckTrackingState.previousPositionX =
      observation.xPixel;

    puckTrackingState.previousPositionY =
      observation.yPixel;

    puckTrackingState.velocityX =
      0.0f;

    puckTrackingState.velocityY =
      0.0f;

    puckTrackingState.accelerationX =
      0.0f;

    puckTrackingState.accelerationY =
      0.0f;

    puckTrackingState.speed =
      0.0f;

    puckTrackingState.lastCaptureTimeMs =
      observation.capturedAtMs;

    puckTrackingState.lastFrameId =
      observation.frameId;

    puckTrackingState.imageWidth =
      observation.imageWidth;

    puckTrackingState.imageHeight =
      observation.imageHeight;

    /*
      The first observation may already be a stationary-puck fallback.
    */
    calculateAndQueueRobotTarget();

    return;
  }

  if (
    observation.frameId ==
    puckTrackingState.lastFrameId
  ) {
    return;
  }

  uint32_t deltaTimeMs =
    observation.capturedAtMs -
    puckTrackingState
      .lastCaptureTimeMs;

  float deltaTimeSeconds =
    deltaTimeMs /
    1000.0f;

  if (
    deltaTimeSeconds <
      puckTrackingConfig
        .minimumDeltaTimeSeconds ||
    deltaTimeSeconds >
      puckTrackingConfig
        .maximumDeltaTimeSeconds
  ) {
    puckTrackingState.positionX =
      observation.xPixel;

    puckTrackingState.positionY =
      observation.yPixel;

    puckTrackingState.previousPositionX =
      observation.xPixel;

    puckTrackingState.previousPositionY =
      observation.yPixel;

    puckTrackingState.lastCaptureTimeMs =
      observation.capturedAtMs;

    puckTrackingState.lastFrameId =
      observation.frameId;

    puckTrackingState.imageWidth =
      observation.imageWidth;

    puckTrackingState.imageHeight =
      observation.imageHeight;

    puckTrackingState.hasVelocity =
      false;

    puckTrackingState.speed =
      0.0f;

    calculateAndQueueRobotTarget();

    return;
  }

  float rawVelocityX =
    (
      observation.xPixel -
      puckTrackingState.positionX
    ) /
    deltaTimeSeconds;

  float rawVelocityY =
    (
      observation.yPixel -
      puckTrackingState.positionY
    ) /
    deltaTimeSeconds;

  float rawSpeed =
    sqrtf(
      rawVelocityX *
        rawVelocityX +
      rawVelocityY *
        rawVelocityY
    );

  if (
    rawSpeed >
    puckTrackingConfig
      .maximumValidPuckSpeedPixelsPerSecond
  ) {
    puckTrackingState.positionX =
      observation.xPixel;

    puckTrackingState.positionY =
      observation.yPixel;

    puckTrackingState.lastCaptureTimeMs =
      observation.capturedAtMs;

    puckTrackingState.lastFrameId =
      observation.frameId;

    puckTrackingState.imageWidth =
      observation.imageWidth;

    puckTrackingState.imageHeight =
      observation.imageHeight;

    puckTrackingState.hasVelocity =
      false;

    puckTrackingState.speed =
      0.0f;

    calculateAndQueueRobotTarget();

    return;
  }

  float previousFilteredVelocityX =
    puckTrackingState.velocityX;

  float previousFilteredVelocityY =
    puckTrackingState.velocityY;

  float filteredVelocityX =
    linearInterpolate(
      previousFilteredVelocityX,
      rawVelocityX,
      puckTrackingConfig
        .velocityFilterAlpha
    );

  float filteredVelocityY =
    linearInterpolate(
      previousFilteredVelocityY,
      rawVelocityY,
      puckTrackingConfig
        .velocityFilterAlpha
    );

  if (observation.stationary) {
    filteredVelocityX =
      0.0f;

    filteredVelocityY =
      0.0f;
  }

  float rawAccelerationX =
    (
      filteredVelocityX -
      previousFilteredVelocityX
    ) /
    deltaTimeSeconds;

  float rawAccelerationY =
    (
      filteredVelocityY -
      previousFilteredVelocityY
    ) /
    deltaTimeSeconds;

  puckTrackingState.accelerationX =
    observation.stationary
      ? 0.0f
      : linearInterpolate(
          puckTrackingState.accelerationX,
          rawAccelerationX,
          puckTrackingConfig
            .accelerationFilterAlpha
        );

  puckTrackingState.accelerationY =
    observation.stationary
      ? 0.0f
      : linearInterpolate(
          puckTrackingState.accelerationY,
          rawAccelerationY,
          puckTrackingConfig
            .accelerationFilterAlpha
        );

  puckTrackingState.previousPositionX =
    puckTrackingState.positionX;

  puckTrackingState.previousPositionY =
    puckTrackingState.positionY;

  puckTrackingState.positionX =
    observation.xPixel;

  puckTrackingState.positionY =
    observation.yPixel;

  puckTrackingState.velocityX =
    filteredVelocityX;

  puckTrackingState.velocityY =
    filteredVelocityY;

  puckTrackingState.speed =
    sqrtf(
      filteredVelocityX *
        filteredVelocityX +
      filteredVelocityY *
        filteredVelocityY
    );

  puckTrackingState.lastCaptureTimeMs =
    observation.capturedAtMs;

  puckTrackingState.lastFrameId =
    observation.frameId;

  puckTrackingState.imageWidth =
    observation.imageWidth;

  puckTrackingState.imageHeight =
    observation.imageHeight;

  puckTrackingState.hasVelocity =
    true;

  puckSpeed =
    (int)roundf(
      puckTrackingState.speed
    );

  calculateAndQueueRobotTarget();
}


// =====================================================
// HTTP endpoint
// =====================================================

void handlePuckUpdate() {
  if (
    server.method() ==
    HTTP_OPTIONS
  ) {
    sendOptions();
    return;
  }

  String requestBody =
    server.arg("plain");

  if (
    requestBody.length() == 0
  ) {
    sendJson(
      400,
      "{\"success\":false,"
      "\"message\":\"empty_body\"}"
    );

    return;
  }

  StaticJsonDocument<576> document;

  DeserializationError jsonError =
    deserializeJson(
      document,
      requestBody
    );

  if (jsonError) {
    String response =
      "{\"success\":false,"
      "\"message\":\"invalid_json\","
      "\"detail\":\"" +
      String(jsonError.c_str()) +
      "\"}";

    sendJson(
      400,
      response
    );

    return;
  }

  PuckObservation observation;

  observation.detected =
    document["detected"] |
    false;

  observation.stationary =
    document["stationary"] |
    false;

  observation.frameId =
    document["frame_id"] |
    0;

  observation.capturedAtMs =
    document["captured_at_ms"] |
    0;

  observation.imageWidth =
    document["image_width"] |
    0;

  observation.imageHeight =
    document["image_height"] |
    0;

  observation.confidence =
    document["confidence"] |
    0.0f;

  observation.xPixel =
    document["x"] |
    0.0f;

  observation.yPixel =
    document["y"] |
    0.0f;

  observation.area =
    document["area"] |
    0.0f;

  observation.radius =
    document["radius"] |
    0.0f;

  if (
    observation.imageWidth <= 0 ||
    observation.imageHeight <= 0
  ) {
    sendJson(
      400,
      "{\"success\":false,"
      "\"message\":\"invalid_image_size\"}"
    );

    return;
  }

  updatePuckTracking(
    observation
  );

  String response = "{";

  response += "\"success\":true,";

  response += "\"game_active\":";
  response += (
      currentScreen ==
      SCREEN_LIVE_GAME
    )
      ? "true,"
      : "false,";

  response += "\"detected\":";
  response += observation.detected
    ? "true,"
    : "false,";

  response += "\"stationary\":";
  response += observation.stationary
    ? "true,"
    : "false,";

  response += "\"target_queued\":";
  response += pendingRobotTarget.valid
    ? "true,"
    : "false,";

  response += "\"servo_queued\":";
  response += pendingServoHit
    ? "true,"
    : "false,";

  response += "\"current_x\":";
  response += String(currentX, 2);
  response += ",";

  response += "\"current_y\":";
  response += String(currentY, 2);

  response += "}";

  sendJson(
    200,
    response
  );
}


// =====================================================
// Execute target and servo
// =====================================================

void processPuckTracking() {
  if (
    currentScreen !=
    SCREEN_LIVE_GAME
  ) {
    pendingRobotTarget.valid =
      false;

    pendingServoHit =
      false;

    pendingServoHitSinceMillis =
      0;

    stationaryPuckActionActive =
      false;

    return;
  }

  if (
    pendingRobotTarget.valid &&
    !robotMovementBusy
  ) {
    unsigned long targetAge =
      millis() -
      pendingRobotTarget
        .generatedAtMillis;

    if (
      targetAge >
      puckTrackingConfig
        .maximumTargetAgeMs
    ) {
      pendingRobotTarget.valid =
        false;

      /*
        Do not erase a latched servo request.
        The next stationary or moving observation will queue a fresh target.
      */
      return;
    }

    float targetXcm =
      pendingRobotTarget.xCm;

    float targetYcm =
      pendingRobotTarget.yCm;

    pendingRobotTarget.valid =
      false;

    robotMovementBusy =
      true;

    bool reachedTarget =
      moveToPositionCm(
        targetXcm,
        targetYcm
      );

    robotMovementBusy =
      false;

    if (!reachedTarget) {
      return;
    }

    bool hitWaitExpired =
      shouldLockCurrentMovementForServo();

    if (
      pendingRobotTarget.valid &&
      !hitWaitExpired
    ) {
      return;
    }

    /*
      pendingServoHit is a latch.
      Once requested, it is cleared only after hitPuck() actually runs.
    */
    if (pendingServoHit) {
      pendingRobotTarget.valid =
        false;

      pendingServoHit =
        false;

      robotMovementBusy =
        true;

      Serial.println(
        stationaryPuckActionActive
          ? "Stationary puck reached - servo hit"
          : "Final XY reached - servo hit"
      );

      hitPuck();

      robotMovementBusy =
        false;

      hitIssuedForCurrentApproach =
        true;

      lastServoHitMillis =
        millis();

      pendingServoHitSinceMillis =
        0;

      stationaryPuckActionActive =
        false;

      Serial.println(
        "Servo hit completed"
      );
    }
  }

  /*
    If the calculated target equals current XY, the motors do not move,
    but the latched servo request is still executed.
  */
  if (
    pendingServoHit &&
    !pendingRobotTarget.valid &&
    !robotMovementBusy
  ) {
    pendingServoHit =
      false;

    robotMovementBusy =
      true;

    Serial.println(
      "Puck already adjacent - servo hit without DC movement"
    );

    hitPuck();

    robotMovementBusy =
      false;

    hitIssuedForCurrentApproach =
      true;

    lastServoHitMillis =
      millis();

    pendingServoHitSinceMillis =
      0;

    stationaryPuckActionActive =
      false;
  }
}

void updatePuckTracking(
    const PuckObservation& observation
) {
    // If the camera did not detect a puck
    if (!observation.detected) {
        puckTrackingState.lostFrameCount++;

        if (
            puckTrackingState.lostFrameCount >=
            puckTrackingConfig.lostFramesBeforeReset
        ) {
            resetPuckTracking();
        }

        return;
    }

    // A valid frame was received
    puckTrackingState.lostFrameCount = 0;

    // Prevent processing the same frame twice
    if (
        puckTrackingState.hasPosition &&
        observation.frameId <= puckTrackingState.lastFrameId
    ) {
        return;
    }

    float newPositionX = observation.xPixel;
    float newPositionY = observation.yPixel;

    // Flip the axes if the camera is mounted upside down
    if (puckTrackingConfig.flipCameraX) {
        newPositionX =
            observation.imageWidth - newPositionX;
    }

    if (puckTrackingConfig.flipCameraY) {
        newPositionY =
            observation.imageHeight - newPositionY;
    }

    // First frame: speed and acceleration cannot be calculated yet
    if (!puckTrackingState.hasPosition) {
        puckTrackingState.positionX = newPositionX;
        puckTrackingState.positionY = newPositionY;

        puckTrackingState.previousPositionX = newPositionX;
        puckTrackingState.previousPositionY = newPositionY;

        puckTrackingState.lastCaptureTimeMs =
            observation.capturedAtMs;

        puckTrackingState.lastFrameId =
            observation.frameId;

        puckTrackingState.imageWidth =
            observation.imageWidth;

        puckTrackingState.imageHeight =
            observation.imageHeight;

        puckTrackingState.hasPosition = true;
        puckTrackingState.hasVelocity = false;

        return;
    }

    // Time between two frames, in seconds
    uint32_t deltaTimeMs =
        observation.capturedAtMs -
        puckTrackingState.lastCaptureTimeMs;

    float deltaTimeSeconds =
        deltaTimeMs / 1000.0f;

    // Guard against a time that is too small or too large
    if (
        deltaTimeSeconds <
            puckTrackingConfig.minimumDeltaTimeSeconds ||
        deltaTimeSeconds >
            puckTrackingConfig.maximumDeltaTimeSeconds
    ) {
        puckTrackingState.positionX = newPositionX;
        puckTrackingState.positionY = newPositionY;

        puckTrackingState.previousPositionX = newPositionX;
        puckTrackingState.previousPositionY = newPositionY;

        puckTrackingState.lastCaptureTimeMs =
            observation.capturedAtMs;

        puckTrackingState.lastFrameId =
            observation.frameId;

        puckTrackingState.hasVelocity = false;

        return;
    }

    // Save the previous speed to calculate acceleration
    float previousVelocityX =
        puckTrackingState.velocityX;

    float previousVelocityY =
        puckTrackingState.velocityY;

    // Raw speed from position change divided by time
    float rawVelocityX =
        (
            newPositionX -
            puckTrackingState.positionX
        ) / deltaTimeSeconds;

    float rawVelocityY =
        (
            newPositionY -
            puckTrackingState.positionY
        ) / deltaTimeSeconds;

    float rawSpeed =
        sqrtf(
            rawVelocityX * rawVelocityX +
            rawVelocityY * rawVelocityY
        );

    // Reject an unrealistic jump in the camera measurement
    if (
        rawSpeed >
        puckTrackingConfig.maximumValidPuckSpeedPixelsPerSecond
    ) {
        puckTrackingState.lastCaptureTimeMs =
            observation.capturedAtMs;

        puckTrackingState.lastFrameId =
            observation.frameId;

        return;
    }

    float velocityAlpha =
        puckTrackingConfig.velocityFilterAlpha;

    // Speed filtering
    float filteredVelocityX =
        velocityAlpha * rawVelocityX +
        (1.0f - velocityAlpha) *
        previousVelocityX;

    float filteredVelocityY =
        velocityAlpha * rawVelocityY +
        (1.0f - velocityAlpha) *
        previousVelocityY;

    // Acceleration calculated from camera data
    float cameraAccelerationX = 0.0f;
    float cameraAccelerationY = 0.0f;

    if (puckTrackingState.hasVelocity) {
        cameraAccelerationX =
            (
                filteredVelocityX -
                previousVelocityX
            ) / deltaTimeSeconds;

        cameraAccelerationY =
            (
                filteredVelocityY -
                previousVelocityY
            ) / deltaTimeSeconds;
    }


    float sensorAccelerationX;
    float sensorAccelerationY;
    float sensorAccelerationZ;

    noInterrupts();

    sensorAccelerationX = braceletAccelerationX;
    sensorAccelerationY = braceletAccelerationY;
    sensorAccelerationZ = braceletAccelerationZ;

    interrupts();

    const float SENSOR_TO_PIXEL_ACCELERATION_X = 1.0f;
    const float SENSOR_TO_PIXEL_ACCELERATION_Y = 1.0f;

    float convertedSensorAccelerationX =
        sensorAccelerationX *
        SENSOR_TO_PIXEL_ACCELERATION_X;

    float convertedSensorAccelerationY =
        sensorAccelerationY *
        SENSOR_TO_PIXEL_ACCELERATION_Y;


    const float CAMERA_ACCELERATION_WEIGHT = 0.90f;
    const float SENSOR_ACCELERATION_WEIGHT = 0.10f;

    float combinedAccelerationX =
        CAMERA_ACCELERATION_WEIGHT *
        cameraAccelerationX +
        SENSOR_ACCELERATION_WEIGHT *
        convertedSensorAccelerationX;

    float combinedAccelerationY =
        CAMERA_ACCELERATION_WEIGHT *
        cameraAccelerationY +
        SENSOR_ACCELERATION_WEIGHT *
        convertedSensorAccelerationY;

    float accelerationAlpha =
        puckTrackingConfig.accelerationFilterAlpha;

    // Filtering the combined acceleration
    puckTrackingState.accelerationX =
        accelerationAlpha *
        combinedAccelerationX +
        (1.0f - accelerationAlpha) *
        puckTrackingState.accelerationX;

    puckTrackingState.accelerationY =
        accelerationAlpha *
        combinedAccelerationY +
        (1.0f - accelerationAlpha) *
        puckTrackingState.accelerationY;

    // Update the filtered speed
    puckTrackingState.velocityX =
        filteredVelocityX;

    puckTrackingState.velocityY =
        filteredVelocityY;

    puckTrackingState.speed =
        sqrtf(
            filteredVelocityX * filteredVelocityX +
            filteredVelocityY * filteredVelocityY
        );

    // Save the previous position
    puckTrackingState.previousPositionX =
        puckTrackingState.positionX;

    puckTrackingState.previousPositionY =
        puckTrackingState.positionY;

    // Update the current position
    puckTrackingState.positionX =
        newPositionX;

    puckTrackingState.positionY =
        newPositionY;

    puckTrackingState.lastCaptureTimeMs =
        observation.capturedAtMs;

    puckTrackingState.lastFrameId =
        observation.frameId;

    puckTrackingState.imageWidth =
        observation.imageWidth;

    puckTrackingState.imageHeight =
        observation.imageHeight;

    puckTrackingState.hasPosition = true;
    puckTrackingState.hasVelocity = true;

}
