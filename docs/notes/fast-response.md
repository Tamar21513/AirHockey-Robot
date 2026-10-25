# Fast Response – Main Changes

## Pins

| Part | Pin |
|---|---|
| IR, robot goal | GPIO 27 |
| IR, player goal | GPIO 34 |
| Distance sensor Trig | GPIO 16 |
| Distance sensor Echo | GPIO 35 |

## Speed

- Motor X: PWM 240
- Motor Y: PWM 255
- Check for a new target while moving: every 5 ms
- Camera target valid for: up to 150 ms
- Image server: tries to get a frame every 20 ms

## New behavior

When the puck enters the robot area, the ESP does not wait for the speed or acceleration calculation. It immediately sends a target based on the latest puck position and corrects the move if a new target arrives while driving.

## Note

The position is still calculated from motor run time. Without encoders and home sensors a perfect physical position is not possible, so `CM_TO_TIME_MS_X` / `CM_TO_TIME_MS_Y` must be calibrated.
