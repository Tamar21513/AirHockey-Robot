# Version 3 – Center Strike

## Change

Every time the servo strikes, the whole mechanism moves slightly toward the center of the table on the X axis.

## Sequence

1. The motors stop.
2. The servo goes to its rest position.
3. Motor X starts a small move toward the center.
4. The servo strikes while motor X is moving.
5. Motor X stops.
6. The servo returns to rest.

## Settings in `Global.h`

| Setting | Value |
|---|---|
| `SERVO_CENTER_ADVANCE_CM` | 0.35 cm |
| `SERVO_CENTER_ADVANCE_SPEED` | 145 |
| `SERVO_CENTER_ADVANCE_MIN_MS` | 35 ms |

## Direction

- `robotOnLeftSide = true`: the move toward the center is toward larger X.
- `robotOnLeftSide = false`: the move toward the center is toward smaller X.

## Calibration

If the advance is too large, change `SERVO_CENTER_ADVANCE_CM` to `0.20f`. If the move is barely visible, change it to `0.50f` or slightly increase `SERVO_CENTER_ADVANCE_MIN_MS`.

The code does not move the mechanism back after the strike. The new position is stored in `currentX` and used for the next move.

## Other fixes in this version

1. Both X and Y are calculated in Dynamic XY mode.
2. Both axes can move to a larger or smaller value.
3. A new camera target can stop an old target in the middle of a move.
4. The robot position is estimated after stopping in the middle of a move.
5. If the new X/Y equals the current X/Y, the servo still strikes.
6. The servo decision does not depend on whether the target changed.
7. The distance sensor is checked only every 100 ms instead of blocking 30 ms in every loop.
8. The Python server starts the camera loop automatically.
9. The ROI on the ESP matches the detection server: X from 0.20 to 0.91, Y from 0.06 to 0.94.
