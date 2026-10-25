from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Dict, Optional
import math

import cv2
import numpy as np


@dataclass
class DetectorConfig:
    # The right side is closer to the camera, so the ROI reaches
    # almost to the image edge.
    roi_min_x_ratio: float = 0.03
    roi_max_x_ratio: float = 0.97
    roi_min_y_ratio: float = 0.05
    roi_max_y_ratio: float = 0.95

    # Base size range. A perspective correction is added per candidate.
    min_radius_ratio: float = 0.014
    expected_radius_ratio: float = 0.030
    max_radius_ratio: float = 0.060

    min_area_ratio: float = 0.00015
    max_area_ratio: float = 0.01400

    min_circularity: float = 0.25
    min_solidity: float = 0.55
    min_fill_ratio: float = 0.28
    min_aspect_ratio: float = 0.40
    max_aspect_ratio: float = 2.20

    continuity_distance_ratio: float = 0.58

    # The puck appears larger on the right side near the camera.
    near_camera_side: str = "right"
    near_camera_radius_gain: float = 0.34


def clamp01(value: float) -> float:
    return max(0.0, min(1.0, value))


class FastPuckDetector:
    def __init__(
        self,
        config: Optional[DetectorConfig] = None,
    ) -> None:
        self.config = config or DetectorConfig()
        self.previous_center: Optional[
            tuple[float, float]
        ] = None
        self.previous_radius: Optional[float] = None
        self.lost_frames = 0

    def reset(self) -> None:
        self.previous_center = None
        self.previous_radius = None
        self.lost_frames = 0

    def _red_mask(
        self,
        roi_bgr: np.ndarray,
    ) -> np.ndarray:
        hsv = cv2.cvtColor(
            roi_bgr,
            cv2.COLOR_BGR2HSV,
        )

        # Lower saturation/value thresholds help the close right side,
        # where the red puck can be brighter or partially washed out.
        mask_1 = cv2.inRange(
            hsv,
            np.array(
                [0, 42, 24],
                dtype=np.uint8,
            ),
            np.array(
                [18, 255, 255],
                dtype=np.uint8,
            ),
        )

        mask_2 = cv2.inRange(
            hsv,
            np.array(
                [164, 42, 24],
                dtype=np.uint8,
            ),
            np.array(
                [180, 255, 255],
                dtype=np.uint8,
            ),
        )

        mask = cv2.bitwise_or(
            mask_1,
            mask_2,
        )

        kernel = np.ones(
            (3, 3),
            dtype=np.uint8,
        )

        mask = cv2.morphologyEx(
            mask,
            cv2.MORPH_OPEN,
            kernel,
            iterations=1,
        )

        mask = cv2.morphologyEx(
            mask,
            cv2.MORPH_CLOSE,
            kernel,
            iterations=1,
        )

        return mask

    def _perspective_factor(
        self,
        center_x: float,
        image_width: int,
    ) -> float:
        normalized_x = clamp01(
            center_x /
            max(image_width, 1)
        )

        if (
            self.config.near_camera_side ==
            "right"
        ):
            return normalized_x

        return 1.0 - normalized_x

    def extract_candidates(
        self,
        image_bgr: np.ndarray,
    ) -> list[Dict[str, float]]:
        if (
            image_bgr is None or
            image_bgr.size == 0
        ):
            raise ValueError(
                "Invalid image"
            )

        image_height, image_width = (
            image_bgr.shape[:2]
        )

        image_area = float(
            image_width *
            image_height
        )

        config = self.config

        minimum_x = int(
            image_width *
            config.roi_min_x_ratio
        )

        maximum_x = int(
            image_width *
            config.roi_max_x_ratio
        )

        minimum_y = int(
            image_height *
            config.roi_min_y_ratio
        )

        maximum_y = int(
            image_height *
            config.roi_max_y_ratio
        )

        minimum_x = max(
            0,
            min(
                minimum_x,
                image_width - 1,
            ),
        )

        maximum_x = max(
            minimum_x + 1,
            min(
                maximum_x,
                image_width,
            ),
        )

        minimum_y = max(
            0,
            min(
                minimum_y,
                image_height - 1,
            ),
        )

        maximum_y = max(
            minimum_y + 1,
            min(
                maximum_y,
                image_height,
            ),
        )

        roi = image_bgr[
            minimum_y:maximum_y,
            minimum_x:maximum_x,
        ]

        mask = self._red_mask(
            roi
        )

        contours, _ = cv2.findContours(
            mask,
            cv2.RETR_EXTERNAL,
            cv2.CHAIN_APPROX_SIMPLE,
        )

        minimum_area = (
            image_area *
            config.min_area_ratio
        )

        maximum_area = (
            image_area *
            config.max_area_ratio
        )

        candidates: list[
            Dict[str, float]
        ] = []

        for contour in contours:
            area = float(
                cv2.contourArea(
                    contour
                )
            )

            if (
                area < minimum_area or
                area > maximum_area
            ):
                continue

            perimeter = float(
                cv2.arcLength(
                    contour,
                    True,
                )
            )

            if perimeter <= 0.0:
                continue

            circularity = (
                4.0 *
                math.pi *
                area /
                (
                    perimeter *
                    perimeter
                )
            )

            if (
                circularity <
                config.min_circularity
            ):
                continue

            (
                local_x,
                local_y,
            ), radius = (
                cv2.minEnclosingCircle(
                    contour
                )
            )

            radius = float(radius)

            center_x = (
                float(local_x) +
                minimum_x
            )

            center_y = (
                float(local_y) +
                minimum_y
            )

            perspective = (
                self._perspective_factor(
                    center_x,
                    image_width,
                )
            )

            expected_radius = (
                image_width *
                config.expected_radius_ratio *
                (
                    1.0 +
                    config.near_camera_radius_gain *
                    perspective
                )
            )

            minimum_radius = max(
                image_width *
                config.min_radius_ratio,
                expected_radius *
                0.48,
            )

            maximum_radius = min(
                image_width *
                config.max_radius_ratio,
                expected_radius *
                1.48,
            )

            if (
                radius < minimum_radius or
                radius > maximum_radius
            ):
                continue

            _, _, box_width, box_height = (
                cv2.boundingRect(
                    contour
                )
            )

            if box_height <= 0:
                continue

            aspect_ratio = (
                box_width /
                float(box_height)
            )

            if not (
                config.min_aspect_ratio <=
                aspect_ratio <=
                config.max_aspect_ratio
            ):
                continue

            hull = cv2.convexHull(
                contour
            )

            hull_area = float(
                cv2.contourArea(
                    hull
                )
            )

            solidity = (
                area /
                hull_area
                if hull_area > 0.0
                else 0.0
            )

            if (
                solidity <
                config.min_solidity
            ):
                continue

            enclosing_area = (
                math.pi *
                radius *
                radius
            )

            fill_ratio = (
                area /
                enclosing_area
                if enclosing_area > 0.0
                else 0.0
            )

            fill_ratio = clamp01(
                fill_ratio
            )

            if (
                fill_ratio <
                config.min_fill_ratio
            ):
                continue

            radius_tolerance = max(
                expected_radius *
                0.52,
                1.0,
            )

            size_score = clamp01(
                1.0 -
                abs(
                    radius -
                    expected_radius
                ) /
                radius_tolerance
            )

            shape_score = clamp01(
                0.40 * circularity +
                0.34 * solidity +
                0.26 * fill_ratio
            )

            candidates.append(
                {
                    "x": center_x,
                    "y": center_y,
                    "area": area,
                    "radius": radius,
                    "circularity": circularity,
                    "solidity": solidity,
                    "fill_ratio": fill_ratio,
                    "size_score": size_score,
                    "shape_score": shape_score,
                    "base_score": clamp01(
                        0.58 * size_score +
                        0.42 * shape_score
                    ),
                }
            )

        return candidates

    def detect_pair(
        self,
        first_image: np.ndarray,
        second_image: np.ndarray,
    ) -> Dict[str, Any]:
        first_height, first_width = (
            first_image.shape[:2]
        )

        second_height, second_width = (
            second_image.shape[:2]
        )

        first_candidates = (
            self.extract_candidates(
                first_image
            )
        )

        second_candidates = (
            self.extract_candidates(
                second_image
            )
        )

        if not first_candidates:
            return {
                "success": False,
                "error": (
                    "No red circular candidates "
                    "in first image"
                ),
                "first_candidate_count": 0,
                "second_candidate_count":
                    len(second_candidates),
            }

        if not second_candidates:
            return {
                "success": False,
                "error": (
                    "No red circular candidates "
                    "in second image"
                ),
                "first_candidate_count":
                    len(first_candidates),
                "second_candidate_count": 0,
            }

        scale_x = (
            second_width /
            max(first_width, 1)
        )

        scale_y = (
            second_height /
            max(first_height, 1)
        )

        radius_scale = (
            scale_x +
            scale_y
        ) / 2.0

        diagonal = math.hypot(
            second_width,
            second_height,
        )

        minimum_motion = max(
            2.2,
            second_width *
            0.0045,
        )

        maximum_motion = (
            diagonal *
            0.75
        )

        pair_options: list[
            Dict[str, Any]
        ] = []

        for first_candidate in (
            first_candidates
        ):
            first_x = (
                first_candidate["x"] *
                scale_x
            )

            first_y = (
                first_candidate["y"] *
                scale_y
            )

            first_radius = (
                first_candidate["radius"] *
                radius_scale
            )

            for second_candidate in (
                second_candidates
            ):
                second_x = (
                    second_candidate["x"]
                )

                second_y = (
                    second_candidate["y"]
                )

                second_radius = (
                    second_candidate["radius"]
                )

                radius_ratio = (
                    second_radius /
                    max(
                        first_radius,
                        0.001,
                    )
                )

                # More tolerant because a puck moving toward the
                # close right side changes apparent size.
                if not (
                    0.50 <=
                    radius_ratio <=
                    1.90
                ):
                    continue

                displacement = math.hypot(
                    second_x -
                    first_x,
                    second_y -
                    first_y,
                )

                if (
                    displacement >
                    maximum_motion
                ):
                    continue

                size_match_score = clamp01(
                    1.0 -
                    abs(
                        second_radius -
                        first_radius
                    ) /
                    max(
                        second_radius,
                        first_radius,
                        1.0,
                    )
                )

                motion_score = clamp01(
                    (
                        displacement -
                        minimum_motion
                    ) /
                    max(
                        diagonal *
                        0.20,
                        1.0,
                    )
                )

                average_base_score = (
                    first_candidate[
                        "base_score"
                    ] +
                    second_candidate[
                        "base_score"
                    ]
                ) / 2.0

                score = (
                    0.48 * motion_score +
                    0.24 * size_match_score +
                    0.28 * average_base_score
                )

                if (
                    displacement <
                    minimum_motion
                ):
                    score -= 0.38

                pair_options.append(
                    {
                        "score": score,
                        "displacement":
                            displacement,
                        "first": {
                            **first_candidate,
                            "x": first_x,
                            "y": first_y,
                            "radius":
                                first_radius,
                        },
                        "second":
                            second_candidate,
                    }
                )

        if not pair_options:
            return {
                "success": False,
                "error": (
                    "No matching circular object "
                    "between images"
                ),
                "first_candidate_count":
                    len(first_candidates),
                "second_candidate_count":
                    len(second_candidates),
            }

        best_pair = max(
            pair_options,
            key=lambda item:
                item["score"],
        )

        if (
            best_pair["displacement"] <
            minimum_motion
        ):
            return {
                "success": False,
                "error": (
                    "Circular candidates found, "
                    "but none moved enough"
                ),
                "first_candidate_count":
                    len(first_candidates),
                "second_candidate_count":
                    len(second_candidates),
                "best_displacement":
                    round(
                        best_pair[
                            "displacement"
                        ],
                        3,
                    ),
            }

        return {
            "success": True,
            "first": best_pair["first"],
            "second": best_pair["second"],
            "pair_score": round(
                best_pair["score"],
                4,
            ),
            "displacement": round(
                best_pair["displacement"],
                3,
            ),
            "first_candidate_count":
                len(first_candidates),
            "second_candidate_count":
                len(second_candidates),
        }

    def detect_near_previous(
        self,
        image_bgr: np.ndarray,
        previous_center: tuple[
            float,
            float,
        ],
        previous_radius: float,
    ) -> Dict[str, Any]:
        """
        Detect a puck that has stopped.

        This method does not choose the best red object in the whole image.
        It only accepts a candidate near the last known puck location and
        with a similar radius. Therefore fixed red hardware and the large
        striker are not selected.
        """
        image_height, image_width = (
            image_bgr.shape[:2]
        )

        candidates = (
            self.extract_candidates(
                image_bgr
            )
        )

        diagonal = math.hypot(
            image_width,
            image_height,
        )

        maximum_distance = (
            diagonal *
            0.13
        )

        accepted: list[
            Dict[str, Any]
        ] = []

        for candidate in candidates:
            distance = math.hypot(
                candidate["x"] -
                previous_center[0],
                candidate["y"] -
                previous_center[1],
            )

            if (
                distance >
                maximum_distance
            ):
                continue

            radius_ratio = (
                candidate["radius"] /
                max(
                    previous_radius,
                    0.001,
                )
            )

            if not (
                0.55 <=
                radius_ratio <=
                1.80
            ):
                continue

            distance_score = clamp01(
                1.0 -
                distance /
                max(
                    maximum_distance,
                    1.0,
                )
            )

            radius_score = clamp01(
                1.0 -
                abs(
                    candidate["radius"] -
                    previous_radius
                ) /
                max(
                    candidate["radius"],
                    previous_radius,
                    1.0,
                )
            )

            confidence = clamp01(
                0.50 * distance_score +
                0.25 * radius_score +
                0.25 * candidate[
                    "base_score"
                ]
            )

            accepted.append(
                {
                    **candidate,
                    "confidence":
                        confidence,
                    "distance_from_previous":
                        distance,
                }
            )

        if not accepted:
            return {
                "detected": False,
                "image_width":
                    image_width,
                "image_height":
                    image_height,
                "candidate_count":
                    len(candidates),
            }

        best = max(
            accepted,
            key=lambda candidate:
                candidate["confidence"],
        )

        return {
            "detected": True,
            "x": round(best["x"], 3),
            "y": round(best["y"], 3),
            "image_width": image_width,
            "image_height": image_height,
            "area": round(
                best["area"],
                3,
            ),
            "radius": round(
                best["radius"],
                3,
            ),
            "confidence": round(
                best["confidence"],
                4,
            ),
            "candidate_count":
                len(candidates),
            "distance_from_previous":
                round(
                    best[
                        "distance_from_previous"
                    ],
                    3,
                ),
        }

    def detect(
        self,
        image_bgr: np.ndarray,
        previous_center: Optional[
            tuple[float, float]
        ] = None,
    ) -> Dict[str, Any]:
        image_height, image_width = (
            image_bgr.shape[:2]
        )

        candidates = (
            self.extract_candidates(
                image_bgr
            )
        )

        if not candidates:
            self.lost_frames += 1

            if self.lost_frames >= 2:
                self.previous_center = None
                self.previous_radius = None

            return {
                "detected": False,
                "x": None,
                "y": None,
                "image_width":
                    image_width,
                "image_height":
                    image_height,
                "area": 0.0,
                "radius": 0.0,
                "confidence": 0.0,
                "candidate_count": 0,
            }

        continuity_center = (
            previous_center
            if previous_center is not None
            else self.previous_center
        )

        diagonal = math.hypot(
            image_width,
            image_height,
        )

        continuity_distance = (
            diagonal *
            self.config
              .continuity_distance_ratio
        )

        for candidate in candidates:
            proximity_score = 0.45

            if (
                continuity_center is not None
            ):
                distance = math.hypot(
                    candidate["x"] -
                    continuity_center[0],
                    candidate["y"] -
                    continuity_center[1],
                )

                proximity_score = clamp01(
                    1.0 -
                    distance /
                    max(
                        continuity_distance,
                        1.0,
                    )
                )

            radius_continuity_score = 0.5

            if (
                self.previous_radius is not None
            ):
                radius_continuity_score = (
                    clamp01(
                        1.0 -
                        abs(
                            candidate[
                                "radius"
                            ] -
                            self.previous_radius
                        ) /
                        max(
                            candidate[
                                "radius"
                            ],
                            self.previous_radius,
                            1.0,
                        )
                    )
                )

            candidate["confidence"] = (
                clamp01(
                    0.52 *
                      candidate[
                          "base_score"
                      ] +
                    0.30 *
                      proximity_score +
                    0.18 *
                      radius_continuity_score
                )
            )

        best = max(
            candidates,
            key=lambda candidate:
                candidate["confidence"],
        )

        self.previous_center = (
            best["x"],
            best["y"],
        )

        self.previous_radius = (
            best["radius"]
        )

        self.lost_frames = 0

        return {
            "detected": True,
            "x": round(best["x"], 3),
            "y": round(best["y"], 3),
            "image_width":
                image_width,
            "image_height":
                image_height,
            "area": round(
                best["area"],
                3,
            ),
            "radius": round(
                best["radius"],
                3,
            ),
            "confidence": round(
                best["confidence"],
                4,
            ),
            "candidate_count":
                len(candidates),
        }


_default_detector = FastPuckDetector()


def detect_puck(
    image_bgr: np.ndarray,
) -> Dict[str, Any]:
    return _default_detector.detect(
        image_bgr
    )
