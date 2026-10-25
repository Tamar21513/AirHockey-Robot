# Puck Detection Server

FastAPI server that reads frames from the ESP32-CAM, detects the puck and sends its position to the main ESP32.

## Addresses

Set these to match your network:

| Device | Where to set it |
|---|---|
| ESP32-CAM | `CAMERA_IP` in `main_pair_live.py` |
| Main ESP32 | `MAIN_ESP_IP` in `main_pair_live.py` |
| Computer running Python | `PYTHON_SERVER_IP` in `Arduino/AirHockeyMain/Global.h` |

Server port: `8001`.

## Before starting

1. Connect the computer, the ESP32-CAM and the main ESP32 to the same Wi-Fi network.
2. Open the camera Serial Monitor and check its IP address.
3. Open the main ESP Serial Monitor and check its IP address.
4. On the computer, run `ipconfig` and check the IPv4 address.
5. Update the addresses above if they are different.

## One-time installation

```
cd Python_Live_Pair_Detector
py -m pip install --upgrade pip
py -m pip install -r requirements.txt
```

## Start order

1. Power on the ESP32-CAM.
2. Open `http://<camera-ip>/capture` in a browser. A current image of the table must appear.
3. Power on the main ESP32 and check its IP in the Serial Monitor.
4. Run `run_live_pair_server.bat`.
5. Keep the server window open during the game.
6. Open `http://127.0.0.1:8001/camera/status`.

The server starts the capture loop automatically. There is no need to press Start.

## What the status should show

- `running: true`
- `last_camera_error: null`
- `last_esp_error: null`
- `processed_pairs` keeps increasing
- `sent_updates` keeps increasing
- `last_detection` updates when the puck is detected

Other pages: `http://127.0.0.1:8001/` (main page) and `http://127.0.0.1:8001/docs` (FastAPI docs).

## Troubleshooting

**The camera does not connect**

- Check that `/capture` opens in the browser.
- Check that no other program is pulling images from the camera at the same time.
- Check `CAMERA_IP` in `main_pair_live.py`.

**Sending to the ESP fails**

- Open `http://<main-esp-ip>/` in a browser.
- Check that the ESP is on the same network.
- Check `MAIN_ESP_IP`.
- Allow Python through Windows Firewall on private networks.

**The ESP cannot reach the Python server**

The server runs with `--host 0.0.0.0` so it is reachable from the network. Make sure `PYTHON_SERVER_IP` in `Global.h` equals the computer's IPv4 address.

## Pin map

| Part | Pin |
|---|---|
| IR, robot goal | GPIO 27 |
| IR, player goal | GPIO 34 |
| Distance sensor Trig | GPIO 16 |
| Distance sensor Echo | GPIO 35 |

If the distance sensor is an HC-SR04 powered from 5V, its Echo output can be 5V. Do not connect 5V directly to GPIO 35; use a voltage divider.

## Speed improvements

- The server tries to get a new frame every 20 ms.
- Sending to the ESP uses a queue that keeps only the newest update.
- The detection area covers almost the whole image.
- The shape conditions were relaxed to detect a blurred puck while it moves.
