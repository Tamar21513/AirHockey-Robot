"""
Live moving + stationary puck detector.

Moving puck:
    compare two consecutive frames.

Stopped puck:
    keep the last valid puck center and search only near that location.

Python sends raw puck pixels to the ESP.
All target, motor and servo decisions remain on the ESP32.
"""

from __future__ import annotations

import asyncio
import time
from typing import Any, Dict, Optional

import cv2
import numpy as np
import requests
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from fast_detector import (
    DetectorConfig,
    FastPuckDetector,
)


# =========================================================
# Network
# =========================================================

CAMERA_IP = "192.168.1.101"
MAIN_ESP_IP = "192.168.1.103"

CAMERA_CAPTURE_URL = (
    f"http://{CAMERA_IP}/capture"
)

MAIN_ESP_PUCK_URL = (
    f"http://{MAIN_ESP_IP}/puck-update"
)


# =========================================================
# Timing
# =========================================================

CAPTURE_INTERVAL_SECONDS = 0.020
CAMERA_TIMEOUT_SECONDS = 0.45
ESP_REQUEST_TIMEOUT_SECONDS = 0.12

SEND_LOST_FRAMES = True

# The following values are measured in seconds.
STATIONARY_TRACK_MAX_AGE_SECONDS = 1.40
STATIONARY_MAX_DISPLACEMENT_PIXELS = 5.0


# =========================================================
# Application state
# =========================================================

app = FastAPI(
    title=(
        "Air Hockey Moving and "
        "Stationary Puck Detector"
    )
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

camera_session = requests.Session()
esp_session = requests.Session()

camera_session.headers.update(
    {"Connection": "keep-alive"}
)

esp_session.headers.update(
    {"Connection": "keep-alive"}
)

detector = FastPuckDetector(
    DetectorConfig(
        roi_min_x_ratio=0.03,
        roi_max_x_ratio=0.97,
        roi_min_y_ratio=0.05,
        roi_max_y_ratio=0.95,
        near_camera_side="right",
    )
)

latest_payload_queue: asyncio.Queue[
    Dict[str, Any]
] = asyncio.Queue(maxsize=1)

camera_loop_running = False
capture_task: Optional[
    asyncio.Task
] = None
sender_task: Optional[
    asyncio.Task
] = None

previous_frame: Optional[
    np.ndarray
] = None

last_tracked_center: Optional[
    tuple[float, float]
] = None

last_tracked_radius: Optional[
    float
] = None

last_tracked_monotonic = 0.0

frame_id = 0
processed_pairs = 0
detected_pairs = 0
stationary_fallbacks = 0
sent_updates = 0
dropped_stale_updates = 0

last_detection: Optional[
    Dict[str, Any]
] = None

last_pair_result: Optional[
    Dict[str, Any]
] = None

last_camera_error: Optional[str] = None
last_esp_error: Optional[str] = None

last_esp_response: Optional[
    Dict[str, Any]
] = None

last_capture_pair_ms = 0.0
last_esp_send_ms = 0.0


# =========================================================
# Helpers
# =========================================================

def download_camera_frame() -> np.ndarray:
    response = camera_session.get(
        CAMERA_CAPTURE_URL,
        timeout=CAMERA_TIMEOUT_SECONDS,
    )

    response.raise_for_status()

    encoded_image = np.frombuffer(
        response.content,
        dtype=np.uint8,
    )

    image = cv2.imdecode(
        encoded_image,
        cv2.IMREAD_COLOR,
    )

    if image is None:
        raise ValueError(
            "ESP32-CAM returned invalid JPEG"
        )

    return image


def build_detected_payload(
    detection: Dict[str, Any],
    current_frame_id: int,
    captured_at_ms: int,
    image_width: int,
    image_height: int,
    *,
    stationary: bool,
    confidence: float,
) -> Dict[str, Any]:
    return {
        "frame_id":
            current_frame_id,
        "captured_at_ms":
            captured_at_ms,
        "detected": True,
        "stationary":
            stationary,
        "image_width":
            image_width,
        "image_height":
            image_height,
        "confidence": float(
            max(
                0.0,
                min(
                    1.0,
                    confidence,
                ),
            )
        ),
        "x": float(
            detection["x"]
        ),
        "y": float(
            detection["y"]
        ),
        "area": float(
            detection["area"]
        ),
        "radius": float(
            detection["radius"]
        ),
    }


def build_lost_payload(
    current_frame_id: int,
    captured_at_ms: int,
    image_width: int,
    image_height: int,
) -> Dict[str, Any]:
    return {
        "frame_id":
            current_frame_id,
        "captured_at_ms":
            captured_at_ms,
        "detected": False,
        "stationary": False,
        "image_width":
            image_width,
        "image_height":
            image_height,
        "confidence": 0.0,
    }


def send_payload_to_esp(
    payload: Dict[str, Any],
) -> Dict[str, Any]:
    response = esp_session.post(
        MAIN_ESP_PUCK_URL,
        json=payload,
        timeout=ESP_REQUEST_TIMEOUT_SECONDS,
    )

    response.raise_for_status()

    try:
        response_body: Any = (
            response.json()
        )
    except ValueError:
        response_body = (
            response.text
        )

    return {
        "status_code":
            response.status_code,
        "body":
            response_body,
    }


async def keep_only_latest_payload(
    payload: Dict[str, Any],
) -> None:
    global dropped_stale_updates

    if latest_payload_queue.full():
        try:
            latest_payload_queue.get_nowait()
            latest_payload_queue.task_done()

            dropped_stale_updates += 1
        except asyncio.QueueEmpty:
            pass

    latest_payload_queue.put_nowait(
        payload
    )


# =========================================================
# Capture and detection
# =========================================================

async def capture_pair_loop() -> None:
    global previous_frame
    global last_tracked_center
    global last_tracked_radius
    global last_tracked_monotonic
    global frame_id
    global processed_pairs
    global detected_pairs
    global stationary_fallbacks
    global last_detection
    global last_pair_result
    global last_camera_error
    global last_capture_pair_ms

    print(
        "Moving + stationary capture loop started"
    )

    print(
        f"Camera: {CAMERA_CAPTURE_URL}"
    )

    while camera_loop_running:
        iteration_started = (
            time.perf_counter()
        )

        try:
            current_frame = (
                await asyncio.to_thread(
                    download_camera_frame
                )
            )

            captured_at_ms = (
                int(
                    time.monotonic() *
                    1000
                ) &
                0xFFFFFFFF
            )

            frame_id = (
                frame_id + 1
            ) & 0xFFFFFFFF

            image_height, image_width = (
                current_frame.shape[:2]
            )

            if previous_frame is None:
                previous_frame = (
                    current_frame
                )

                await asyncio.sleep(0)
                continue

            pair_result = (
                await asyncio.to_thread(
                    detector.detect_pair,
                    previous_frame,
                    current_frame,
                )
            )

            previous_frame = (
                current_frame
            )

            processed_pairs += 1
            last_pair_result = pair_result

            payload: Optional[
                Dict[str, Any]
            ] = None

            if pair_result["success"]:
                second = pair_result[
                    "second"
                ]

                displacement = float(
                    pair_result[
                        "displacement"
                    ]
                )

                stationary = (
                    displacement <=
                    STATIONARY_MAX_DISPLACEMENT_PIXELS
                )

                payload = build_detected_payload(
                    second,
                    frame_id,
                    captured_at_ms,
                    image_width,
                    image_height,
                    stationary=stationary,
                    confidence=float(
                        pair_result[
                            "pair_score"
                        ]
                    ),
                )

                detected_pairs += 1

                last_tracked_center = (
                    float(second["x"]),
                    float(second["y"]),
                )

                last_tracked_radius = float(
                    second["radius"]
                )

                last_tracked_monotonic = (
                    time.monotonic()
                )

            else:
                # Pair matching intentionally rejects an object that
                # did not move. When the puck has just stopped, use the
                # last known puck position and search only nearby.
                tracking_age = (
                    time.monotonic() -
                    last_tracked_monotonic
                )

                can_use_stationary_fallback = (
                    last_tracked_center is not None and
                    last_tracked_radius is not None and
                    tracking_age <=
                      STATIONARY_TRACK_MAX_AGE_SECONDS
                )

                if can_use_stationary_fallback:
                    stationary_result = (
                        await asyncio.to_thread(
                            detector.detect_near_previous,
                            current_frame,
                            last_tracked_center,
                            last_tracked_radius,
                        )
                    )

                    if stationary_result[
                        "detected"
                    ]:
                        payload = (
                            build_detected_payload(
                                stationary_result,
                                frame_id,
                                captured_at_ms,
                                image_width,
                                image_height,
                                stationary=True,
                                confidence=float(
                                    stationary_result[
                                        "confidence"
                                    ]
                                ),
                            )
                        )

                        stationary_fallbacks += 1

                        last_tracked_center = (
                            float(
                                stationary_result[
                                    "x"
                                ]
                            ),
                            float(
                                stationary_result[
                                    "y"
                                ]
                            ),
                        )

                        last_tracked_radius = (
                            float(
                                stationary_result[
                                    "radius"
                                ]
                            )
                        )

                        last_tracked_monotonic = (
                            time.monotonic()
                        )

            if (
                payload is None and
                SEND_LOST_FRAMES
            ):
                payload = build_lost_payload(
                    frame_id,
                    captured_at_ms,
                    image_width,
                    image_height,
                )

            if payload is not None:
                last_detection = payload

                await keep_only_latest_payload(
                    payload
                )

                if (
                    payload["detected"] and
                    processed_pairs % 15 == 0
                ):
                    print(
                        "Puck: "
                        f"x={payload['x']:.1f}, "
                        f"y={payload['y']:.1f}, "
                        f"r={payload['radius']:.1f}, "
                        f"stationary="
                        f"{payload['stationary']}"
                    )

            last_camera_error = None

        except Exception as error:
            last_camera_error = str(error)

            print(
                "Camera/detection error: "
                f"{error}"
            )

            previous_frame = None

        last_capture_pair_ms = (
            time.perf_counter() -
            iteration_started
        ) * 1000.0

        remaining_time = (
            CAPTURE_INTERVAL_SECONDS -
            (
                time.perf_counter() -
                iteration_started
            )
        )

        if remaining_time > 0.0:
            await asyncio.sleep(
                remaining_time
            )
        else:
            await asyncio.sleep(0)


# =========================================================
# ESP sender
# =========================================================

async def esp_sender_loop() -> None:
    global sent_updates
    global last_esp_error
    global last_esp_response
    global last_esp_send_ms

    print(
        "ESP sender loop started"
    )

    print(
        f"Main ESP: {MAIN_ESP_PUCK_URL}"
    )

    while camera_loop_running:
        try:
            payload = (
                await asyncio.wait_for(
                    latest_payload_queue.get(),
                    timeout=0.50,
                )
            )
        except asyncio.TimeoutError:
            continue

        send_started = (
            time.perf_counter()
        )

        try:
            last_esp_response = (
                await asyncio.to_thread(
                    send_payload_to_esp,
                    payload,
                )
            )

            sent_updates += 1
            last_esp_error = None

        except Exception as error:
            last_esp_error = str(error)

            print(
                f"ESP update error: {error}"
            )

        finally:
            last_esp_send_ms = (
                time.perf_counter() -
                send_started
            ) * 1000.0

            latest_payload_queue.task_done()


# =========================================================
# FastAPI
# =========================================================

@app.on_event("startup")
async def startup_event() -> None:
    global camera_loop_running
    global capture_task
    global sender_task

    if camera_loop_running:
        return

    camera_loop_running = True

    capture_task = asyncio.create_task(
        capture_pair_loop()
    )

    sender_task = asyncio.create_task(
        esp_sender_loop()
    )

    print(
        "Live detector started"
    )


@app.on_event("shutdown")
async def shutdown_event() -> None:
    global camera_loop_running

    camera_loop_running = False

    for task in (
        capture_task,
        sender_task,
    ):
        if task is not None:
            task.cancel()


@app.get("/")
def home() -> Dict[str, Any]:
    return {
        "message": (
            "Moving and stationary puck "
            "detector is running"
        ),
        "camera_url":
            CAMERA_CAPTURE_URL,
        "main_esp_url":
            MAIN_ESP_PUCK_URL,
        "running":
            camera_loop_running,
    }


@app.get("/camera/status")
def camera_status() -> Dict[str, Any]:
    return {
        "success": True,
        "running":
            camera_loop_running,
        "camera_url":
            CAMERA_CAPTURE_URL,
        "main_esp_url":
            MAIN_ESP_PUCK_URL,
        "last_camera_error":
            last_camera_error,
        "last_esp_error":
            last_esp_error,
        "last_esp_response":
            last_esp_response,
        "statistics": {
            "processed_pairs":
                processed_pairs,
            "detected_pairs":
                detected_pairs,
            "stationary_fallbacks":
                stationary_fallbacks,
            "sent_updates":
                sent_updates,
            "dropped_stale_updates":
                dropped_stale_updates,
        },
        "timing": {
            "capture_and_detection_ms":
                round(
                    last_capture_pair_ms,
                    2,
                ),
            "esp_send_ms":
                round(
                    last_esp_send_ms,
                    2,
                ),
        },
        "last_detection":
            last_detection,
        "last_pair_result":
            last_pair_result,
    }
