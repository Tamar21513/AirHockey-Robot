#include "Global.h"

bool robotMovementBusy = false;

bool movementStartedInsideHttpHandler = false;

const float POSITION_EPSILON_CM = 0.02f;
const float LIVE_RETARGET_MIN_CHANGE_CM = 0.04f;
const unsigned long LIVE_RETARGET_MIN_TIME_MS = 5UL;


bool fresherTrackingTargetRequiresRetarget(
  float activeTargetX,
  float activeTargetY
) {
  if (
    currentScreen != SCREEN_LIVE_GAME ||
    !pendingRobotTarget.valid ||
    shouldLockCurrentMovementForServo()
  ) {
    return false;
  }

  unsigned long targetAge =
    millis() -
    pendingRobotTarget.generatedAtMillis;

  if (
    targetAge >
    puckTrackingConfig.maximumTargetAgeMs
  ) {
    pendingRobotTarget.valid = false;
    return false;
  }

  float differenceX = fabsf(
    pendingRobotTarget.xCm -
    activeTargetX
  );

  float differenceY = fabsf(
    pendingRobotTarget.yCm -
    activeTargetY
  );

  return (
    differenceX >=
      LIVE_RETARGET_MIN_CHANGE_CM ||
    differenceY >=
      LIVE_RETARGET_MIN_CHANGE_CM
  );
}


void updateEstimatedPositionAfterPartialMove(
  float startX,
  float startY,
  float deltaX,
  float deltaY,
  unsigned long elapsedMs,
  unsigned long moveTimeX,
  unsigned long moveTimeY
) {
  float fractionX = 1.0f;
  float fractionY = 1.0f;

  if (moveTimeX > 0) {
    fractionX = fminf(
      1.0f,
      elapsedMs /
        (float)moveTimeX
    );
  }

  if (moveTimeY > 0) {
    fractionY = fminf(
      1.0f,
      elapsedMs /
        (float)moveTimeY
    );
  }

  currentX =
    startX +
    deltaX *
    fractionX;

  currentY =
    startY +
    deltaY *
    fractionY;

  currentX = constrain(
    currentX,
    MIN_X_CM,
    MAX_X_CM
  );

  currentY = constrain(
    currentY,
    MIN_Y_CM,
    MAX_Y_CM
  );
}


bool moveToPositionCm(
  float targetXcm,
  float targetYcm
) {
  targetXcm = constrain(
    targetXcm,
    MIN_X_CM,
    MAX_X_CM
  );

  targetYcm = constrain(
    targetYcm,
    MIN_Y_CM,
    MAX_Y_CM
  );

  float startX = currentX;
  float startY = currentY;

  float deltaX =
    targetXcm -
    startX;

  float deltaY =
    targetYcm -
    startY;

  int directionX = 0;
  int directionY = 0;

  if (deltaX > POSITION_EPSILON_CM) {
    directionX = 1;
  }
  else if (deltaX < -POSITION_EPSILON_CM) {
    directionX = -1;
  }

  if (deltaY > POSITION_EPSILON_CM) {
    directionY = 1;
  }
  else if (deltaY < -POSITION_EPSILON_CM) {
    directionY = -1;
  }

  unsigned long moveTimeX =
    directionX == 0
      ? 0UL
      : (unsigned long)(
          fabsf(deltaX) *
          CM_TO_TIME_MS_X
        );

  unsigned long moveTimeY =
    directionY == 0
      ? 0UL
      : (unsigned long)(
          fabsf(deltaY) *
          CM_TO_TIME_MS_Y
        );

  Serial.println();
  Serial.println("========== ROBOT MOVE ==========");

  Serial.print("Current X=");
  Serial.print(currentX, 2);
  Serial.print(" Y=");
  Serial.println(currentY, 2);

  Serial.print("Target X=");
  Serial.print(targetXcm, 2);
  Serial.print(" Y=");
  Serial.println(targetYcm, 2);

  Serial.print("Delta X=");
  Serial.print(deltaX, 2);
  Serial.print(" direction X=");
  Serial.println(directionX);

  Serial.print("Delta Y=");
  Serial.print(deltaY, 2);
  Serial.print(" direction Y=");
  Serial.println(directionY);

  bool motorXRunning =
    moveTimeX > 0;

  bool motorYRunning =
    moveTimeY > 0;

  /*
    Same XY is still considered a successfully reached target.
    processPuckTracking() can then activate the servo.
  */
  if (
    !motorXRunning &&
    !motorYRunning
  ) {
    currentX = targetXcm;
    currentY = targetYcm;

    Serial.println(
      "XY already reached - no DC movement needed"
    );

    return true;
  }

  unsigned long movementStartMillis =
    millis();

  if (motorXRunning) {
    setMotorX(
      directionX,
      DEFAULT_MOTOR_SPEED_X
    );
  }

  if (motorYRunning) {
    setMotorY(
      directionY,
      DEFAULT_MOTOR_SPEED_Y
    );
  }

  while (
    motorXRunning ||
    motorYRunning
  ) {
    /*
      Safety: stop even in the middle of a long XY movement when the
      bracelet loses contact or ESP-NOW communication is lost.
    */
    if (!isBraceletReadyForGame()) {
      unsigned long elapsedMs =
        millis() -
        movementStartMillis;

      stopMotors();

      updateEstimatedPositionAfterPartialMove(
        startX,
        startY,
        deltaX,
        deltaY,
        elapsedMs,
        moveTimeX,
        moveTimeY
      );

      Serial.println(
        "Bracelet safety stop during XY movement"
      );

      return false;
    }
    /*
      Accept new /puck-update requests while moving.
      This is what allows X and Y to reverse direction when
      a newer target arrives.
    */
    if (!movementStartedInsideHttpHandler) {
      server.handleClient();
    }

    yield();

    unsigned long elapsedMs =
      millis() -
      movementStartMillis;

    if (
      elapsedMs >=
        LIVE_RETARGET_MIN_TIME_MS &&
      fresherTrackingTargetRequiresRetarget(
        targetXcm,
        targetYcm
      )
    ) {
      stopMotors();

      updateEstimatedPositionAfterPartialMove(
        startX,
        startY,
        deltaX,
        deltaY,
        elapsedMs,
        moveTimeX,
        moveTimeY
      );

      Serial.print(
        "Movement interrupted. Estimated X="
      );
      Serial.print(currentX, 2);
      Serial.print(" Y=");
      Serial.println(currentY, 2);

      return false;
    }

    if (
      motorXRunning &&
      elapsedMs >= moveTimeX
    ) {
      stopMotorX();
      motorXRunning = false;
    }

    if (
      motorYRunning &&
      elapsedMs >= moveTimeY
    ) {
      stopMotorY();
      motorYRunning = false;
    }

    delay(1);
  }

  stopMotors();

  currentX = targetXcm;
  currentY = targetYcm;

  Serial.println("XY target reached");

  return true;
}


void handleMoveCommand() {
  if (
    server.method() ==
    HTTP_OPTIONS
  ) {
    sendOptions();
    return;
  }

  if (
    !server.hasArg("x") ||
    !server.hasArg("y")
  ) {
    sendJson(
      400,
      "{\"success\":false,"
      "\"message\":\"missing_x_or_y\"}"
    );

    return;
  }

  if (robotMovementBusy) {
    sendJson(
      409,
      "{\"success\":false,"
      "\"message\":\"robot_busy\"}"
    );

    return;
  }

  float targetX =
    constrain(
      server.arg("x").toFloat(),
      MIN_X_CM,
      MAX_X_CM
    );

  float targetY =
    constrain(
      server.arg("y").toFloat(),
      MIN_Y_CM,
      MAX_Y_CM
    );

  robotMovementBusy = true;
  movementStartedInsideHttpHandler = true;

  bool reachedTarget = moveToPositionCm(
    targetX,
    targetY
  );

  /*
    Manual /move always fires the servo, including when the
    requested XY equals the current XY.
  */
  if (reachedTarget) {
    hitPuck();
  }

  movementStartedInsideHttpHandler = false;
  robotMovementBusy = false;

  String response = "{";
  response += "\"success\":";
  response += reachedTarget ? "true," : "false,";
  response += "\"message\":\"";
  response += reachedTarget
    ? "robot_moved_and_hit"
    : "movement_interrupted";
  response += "\",";
  response += "\"x\":" + String(currentX, 2) + ",";
  response += "\"y\":" + String(currentY, 2);
  response += "}";

  sendJson(
    reachedTarget ? 200 : 409,
    response
  );
}


void handleRobotStatus() {
  String response = "{";

  response += "\"success\":true,";
  response += "\"current_x\":" +
    String(currentX, 2) + ",";
  response += "\"current_y\":" +
    String(currentY, 2) + ",";
  response += "\"busy\":";
  response += robotMovementBusy
    ? "true,"
    : "false,";
  response += "\"pending_target\":";
  response += pendingRobotTarget.valid
    ? "true,"
    : "false,";
  response += "\"pending_servo\":";
  response += shouldLockCurrentMovementForServo()
    ? "true"
    : "false";

  response += "}";

  sendJson(
    200,
    response
  );
}


void moveToDefensePosition() {
  moveToPositionCm(
    MAX_X_CM / 2.0f,
    2.0f
  );
}


void moveToStartPosition() {
  Serial.println(
    "Moving robot to start position"
  );

  moveToPositionCm(
    START_X_CM,
    START_Y_CM
  );

  Serial.println(
    "Robot reached start position"
  );
}
