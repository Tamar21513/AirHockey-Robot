#include "Global.h"

void init_distance_Sensor() {
  pinMode(
    DIST_TRIG_PIN,
    OUTPUT
  );

  pinMode(
    DIST_ECHO_PIN,
    INPUT
  );

  digitalWrite(
    DIST_TRIG_PIN,
    LOW
  );
}


float getDistanceCm() {
  digitalWrite(
    DIST_TRIG_PIN,
    LOW
  );

  delayMicroseconds(2);

  digitalWrite(
    DIST_TRIG_PIN,
    HIGH
  );

  delayMicroseconds(10);

  digitalWrite(
    DIST_TRIG_PIN,
    LOW
  );

  unsigned long duration =
    pulseIn(
      DIST_ECHO_PIN,
      HIGH,
      DIST_ECHO_TIMEOUT_US
    );

  if (duration == 0) {
    return -1.0f;
  }

  return
    duration *
    0.0343f /
    2.0f;
}
