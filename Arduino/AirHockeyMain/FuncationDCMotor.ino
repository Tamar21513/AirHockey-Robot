#include "Global.h"

void init_dc_motors() {
  pinMode(X_IN1, OUTPUT);
  pinMode(X_IN2, OUTPUT);
  pinMode(X_ENABLE, OUTPUT);

  pinMode(Y_IN1, OUTPUT);
  pinMode(Y_IN2, OUTPUT);
  pinMode(Y_ENABLE, OUTPUT);

  ledcSetup(
    MOTOR_X_PWM_CHANNEL,
    MOTOR_PWM_FREQ,
    MOTOR_PWM_RESOLUTION
  );

  ledcAttachPin(
    X_ENABLE,
    MOTOR_X_PWM_CHANNEL
  );

  ledcSetup(
    MOTOR_Y_PWM_CHANNEL,
    MOTOR_PWM_FREQ,
    MOTOR_PWM_RESOLUTION
  );

  ledcAttachPin(
    Y_ENABLE,
    MOTOR_Y_PWM_CHANNEL
  );

  stopMotors();
}


void setMotorX(int direction, int speedValue) {
  speedValue = constrain(
    speedValue,
    0,
    255
  );

  if (direction > 0) {
    digitalWrite(X_IN1, HIGH);
    digitalWrite(X_IN2, LOW);

    ledcWrite(
      MOTOR_X_PWM_CHANNEL,
      speedValue
    );
  }
  else if (direction < 0) {
    digitalWrite(X_IN1, LOW);
    digitalWrite(X_IN2, HIGH);

    ledcWrite(
      MOTOR_X_PWM_CHANNEL,
      speedValue
    );
  }
  else {
    stopMotorX();
  }
}


void setMotorY(int direction, int speedValue) {
  speedValue = constrain(
    speedValue,
    0,
    255
  );

  if (direction > 0) {
    digitalWrite(Y_IN1, HIGH);
    digitalWrite(Y_IN2, LOW);

    ledcWrite(
      MOTOR_Y_PWM_CHANNEL,
      speedValue
    );
  }
  else if (direction < 0) {
    digitalWrite(Y_IN1, LOW);
    digitalWrite(Y_IN2, HIGH);

    ledcWrite(
      MOTOR_Y_PWM_CHANNEL,
      speedValue
    );
  }
  else {
    stopMotorY();
  }
}


void stopMotorX() {
  digitalWrite(X_IN1, LOW);
  digitalWrite(X_IN2, LOW);

  ledcWrite(
    MOTOR_X_PWM_CHANNEL,
    0
  );
}


void stopMotorY() {
  digitalWrite(Y_IN1, LOW);
  digitalWrite(Y_IN2, LOW);

  ledcWrite(
    MOTOR_Y_PWM_CHANNEL,
    0
  );
}


void stopMotors() {
  stopMotorX();
  stopMotorY();
}
