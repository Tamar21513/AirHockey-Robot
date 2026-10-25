# ESP-NOW Bracelet – Wiring and Operation

## Small ESP wiring

### ADXL345

| Part | Small ESP |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Touch sensor

| Part | Small ESP |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SIG / OUT | GPIO 27 |

GPIO 27 is free, suitable for input and does not conflict with I2C. In the current firmware GPIO 27 is used with `touchRead()` and is connected directly to an aluminum foil pad (no VCC or GND on the foil).

## Communication without internet

The main ESP creates a local network named `AIRHOCKEY_DIRECT`.

The small ESP connects to this network only to sync to the same radio channel, and then sends the bracelet data with ESP-NOW. No router or internet is needed for the bracelet.

## Game start

1. The player chooses a level and a game mode.
2. The player presses Start Game.
3. The motors stop and the LED matrix turns red.
4. The screen shows "Wear Bracelet".
5. The sensor must be touched continuously for 2 seconds.
6. When the ESP-NOW confirmation arrives:
   - the matrix turns green;
   - the buzzer plays a confirmation sound;
   - the game starts.
7. If the bracelet connection is lost, the game does not start.

## Upload order

1. Upload `AirHockeyMain.ino` to the main ESP.
2. Upload `small_ESP/small_ESP.ino` to the small ESP.
3. Power on the main ESP first.
4. Then power on the bracelet.
5. The bracelet Serial Monitor should show `Local AP connected` and `ESP-NOW ready`.
6. The main ESP prints the MAC addresses and waits for packets.

## Working without a network

The bracelet communication and the wear confirmation work without a router and without internet. Firebase and the camera server still need their normal connections. The main firmware does not get stuck at startup without internet, but game-code screens that use Firebase cannot load a new game without a network.

## Safety during the game

The touch sensor must stay active during the whole game. If touch is lost, or no ESP-NOW messages arrive for 350 ms:

- both motors stop immediately;
- puck tracking is reset;
- the LED matrix turns red;
- the game goes to the GAME PAUSED screen;
- the pause time is not counted in the game timer;
- to continue, the player puts the bracelet back on and keeps continuous touch for 2 seconds.
