#include "Global.h"

volatile bool robotGoalIRFlag = false;
volatile bool playerGoalIRFlag = false;

void IRAM_ATTR onRobotGoalIR() {
  robotGoalIRFlag = true;
}

void IRAM_ATTR onPlayerGoalIR() {
  playerGoalIRFlag = true;
}

void init_ir_goal_sensors() {
  pinMode(ROBOT_GOAL_IR_PIN, INPUT_PULLUP);

  pinMode(PLAYER_GOAL_IR_PIN, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(ROBOT_GOAL_IR_PIN),
    onRobotGoalIR,
    FALLING
  );

  attachInterrupt(
    digitalPinToInterrupt(PLAYER_GOAL_IR_PIN),
    onPlayerGoalIR,
    FALLING
  );
}

bool consumeRobotGoalIR() {
  bool detected;

  noInterrupts();
  detected = robotGoalIRFlag;
  robotGoalIRFlag = false;
  interrupts();

  if (detected) {
    Serial.println("ROBOT GOAL IR DETECTED");
  }

  return detected;
}

bool consumePlayerGoalIR() {
  bool detected;

  noInterrupts();
  detected = playerGoalIRFlag;
  playerGoalIRFlag = false;
  interrupts();

  if (detected) {
    Serial.println("PLAYER GOAL IR DETECTED");
  }

  return detected;
}
