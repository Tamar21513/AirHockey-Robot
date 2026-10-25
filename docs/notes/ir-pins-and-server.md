# V3 Update – IR Sensors and Pins

Changes made without changing the working motion algorithm:

1. The IR sensor in the robot goal moved to GPIO 27.
2. The IR sensor in the player goal moved to GPIO 34.
3. The distance sensor Echo moved to GPIO 35.
4. The distance sensor Trig stays on GPIO 16.
5. The goal sensors use FALLING interrupts so short passes are not missed.
6. `FuncationTouchSensor.ino` was replaced by `FuncationIRGoalSensors.ino`.
7. The camera address in the server was updated.
8. The server start file uses host `0.0.0.0`.

## IR wiring

| IR sensor | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| OUT, robot goal | GPIO 27 |
| OUT, player goal | GPIO 34 |

The sensors are active LOW: 1 = no puck, 0 = puck detected.
