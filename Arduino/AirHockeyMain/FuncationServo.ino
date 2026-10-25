#include "Global.h"

void init_servo() {
  Serial.println();
  Serial.println("========== SERVO INIT ==========");

  hitServo.setPeriodHertz(50);

  int servoChannel = hitServo.attach(SERVO_PIN, 500, 2400);

  Serial.print("Servo attach result/channel: ");
  Serial.println(servoChannel);

  Serial.print("Servo attached: ");
  Serial.println(hitServo.attached() ? "YES" : "NO");

  hitServo.write(SERVO_REST_ANGLE);

  delay(350);

  Serial.print("Servo rest angle: ");
  Serial.println(SERVO_REST_ANGLE);
  Serial.println("================================");
}


bool prepareCenterAdvanceForServo(int &directionX, unsigned long &movementTimeMs, float &finalXcm) {
  float requestedDeltaX = puckTrackingConfig.robotOnLeftSide ? SERVO_CENTER_ADVANCE_CM : -SERVO_CENTER_ADVANCE_CM;

  finalXcm = constrain(currentX + requestedDeltaX, MIN_X_CM, MAX_X_CM);

  float actualDeltaX = finalXcm - currentX;

  if (fabsf(actualDeltaX) < 0.01f) {
    directionX = 0;
    movementTimeMs = 0;

    return false;
  }

  directionX = actualDeltaX > 0.0f ? 1 : -1;

  movementTimeMs =
    (unsigned long)(
      fabsf(actualDeltaX) *
      CM_TO_TIME_MS_X
    );

  if (
    movementTimeMs <
    SERVO_CENTER_ADVANCE_MIN_MS
  ) {
    movementTimeMs =
      SERVO_CENTER_ADVANCE_MIN_MS;
  }

  return true;
}


void hitPuck() {
  Serial.println();
  Serial.println("========== SERVO HIT ==========");

  if (!hitServo.attached()) {
    hitServo.setPeriodHertz(50);

    int servoChannel = hitServo.attach(
      SERVO_PIN,
      500,
      2400
    );

    delay(100);
  }

  stopMotors();
  delay(40);

  hitServo.write(
    SERVO_REST_ANGLE
  );

  delay(70);

  int centerDirectionX = 0;
  unsigned long centerMovementTimeMs = 0;
  float centerFinalXcm = currentX;

  bool shouldAdvanceTowardCenter =
    prepareCenterAdvanceForServo(
      centerDirectionX,
      centerMovementTimeMs,
      centerFinalXcm
    );

  if (shouldAdvanceTowardCenter) {
    setMotorX(
      centerDirectionX,
      SERVO_CENTER_ADVANCE_SPEED
    );
  }
  else {
    Serial.println(
      "Center advance skipped - X limit reached"
    );
  }

  hitServo.write(
    SERVO_HIT_ANGLE
  );

  if (shouldAdvanceTowardCenter) {
    delay(
      centerMovementTimeMs
    );

    stopMotorX();

    currentX =
      centerFinalXcm;
  }

  unsigned long remainingServoPushTimeMs =
    SERVO_HIT_DELAY_MS;

  if (
    shouldAdvanceTowardCenter &&
    centerMovementTimeMs <
      SERVO_HIT_DELAY_MS
  ) {
    remainingServoPushTimeMs =
      SERVO_HIT_DELAY_MS -
      centerMovementTimeMs;
  }
  else if (
    shouldAdvanceTowardCenter &&
    centerMovementTimeMs >=
      SERVO_HIT_DELAY_MS
  ) {
    remainingServoPushTimeMs = 0;
  }

  if (
    remainingServoPushTimeMs > 0
  ) {
    delay(
      remainingServoPushTimeMs
    );
  }

  hitServo.write(
    SERVO_HIT_ANGLE
  );

  delay(30);

  hitServo.write(
    SERVO_REST_ANGLE
  );

  delay(
    SERVO_RETURN_DELAY_MS
  );

  stopMotorX();

  Serial.print(
    "Servo returned to rest. Estimated X="
  );

  Serial.println(
    currentX,
    2
  );

  Serial.println("===============================");
}
