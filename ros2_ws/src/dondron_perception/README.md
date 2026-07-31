# dondron_perception

Line 1 Module A — object recognition. Subscribes to camera images, publishes `vision_msgs/Detection2DArray` on `/detections`.

**Public boundary:** this package publishes `/detections` only. No flight setpoints, no detection→setpoint mapping.

Contract freeze artifact for milestone M1; tag `line1-topics-v1` at Line 1 gate.

## Topics (frozen)

| Topic | Type | Direction | QoS |
|-------|------|-----------|-----|
| `/camera/image_raw` | `sensor_msgs/msg/Image` | subscribe | `SensorDataQoS` (best effort, keep last 5) |
| `/camera/camera_info` | `sensor_msgs/msg/CameraInfo` | paired input (M3+) | `SensorDataQoS` |
| `/detections` | `vision_msgs/msg/Detection2DArray` | publish | reliable, keep last 10 |

Remap `image_topic` in launch when the SIL camera source differs (see [Gazebo SIL remap](#gazebo-sil-remap)).

## `/detections` message contract

### Header

| Field | Value |
|-------|-------|
| `header.frame_id` | `camera_optical_frame` (REP-103 optical frame from `dondron_description`) |
| `header.stamp` | Latest received `/camera/image_raw` stamp when available; node clock otherwise |

### Bounding box (`detections[].bbox`)

`vision_msgs/BoundingBox2D` in **pixel coordinates** relative to the image origin (top-left, +X right, +Y down):

| Field | Semantics |
|-------|-----------|
| `center.position.x` | Horizontal center of bbox (pixels) |
| `center.position.y` | Vertical center of bbox (pixels) |
| `size_x` | Full width of bbox (pixels) |
| `size_y` | Full height of bbox (pixels) |

### Class ID and score (`detections[].results[]`)

Each detection has one or more `vision_msgs/ObjectHypothesisWithPose` entries:

| Field | Semantics |
|-------|-----------|
| `hypothesis.class_id` | Decimal string class index (COCO-style table below) |
| `hypothesis.score` | Confidence in \[0.0, 1.0\] |

**Line 1 target classes (M1 stub uses class `0`):**

| `class_id` | Name | Notes |
|------------|------|-------|
| `0` | `target` | Primary recognition object (sim / field target) |
| `1` | `person` | Reserved — not used in M1 |

### Range encoding (decided)

Monocular range (Phase 1) is stored in **`results[].pose.pose.position.z`** (meters, optical-frame depth along +Z).

```
range_m = (focal_length_px × real_target_width_m) / bbox_width_px
```

- Units: meters, positive toward the scene (+Z in `camera_optical_frame`).
- `pose.pose.position.x` and `.y` are unused (0) until a 3D bearing model is added.
- No separate range topic in Line 1; custom message deferred to Line 2+ if covariance or multiple range sources are needed.

**Monocular range parameters (M3+):**

| Parameter | Default | Role |
|-----------|---------|------|
| `target_width_m` | 0.30 | Known physical width of class `0` target |
| `focal_length_px` | from `/camera/camera_info` `K[0]` | Horizontal focal length |

### Empty-detection behavior

| Mode | Behavior |
|------|----------|
| **M1 stub** (current) | Always publishes one synthetic detection at ≥1 Hz for SIL wiring |
| **M3+ inference** | Publishes `detections: []` when no object exceeds score threshold; rate matches camera or `publish_rate_hz` param |

Downstream nodes must handle an empty array (no lock, search continues).

### Publish rate

- Minimum: **1 Hz** (contract floor for BT / diagnostics).
- M1 stub default: **2 Hz** (`publish_rate_hz` parameter).
- M3+: match camera frame rate or cap via parameter.

## `/camera/image_raw` contract

### Camera driver location (decided)

**Separate from `dondron_perception`.** This node only subscribes to `/camera/image_raw`.

| Environment | Driver node |
|-------------|-------------|
| Gazebo SIL | `ros_gz_bridge` (or PX4 model bridge) publishes image + `camera_info` |
| Orange Pi / hardware | Dedicated `v4l2_camera` or OAK-D / RealSense driver node |

Swapping IMX219 → OAK-D / RealSense changes only the driver; perception topic names stay fixed.

### Image format (target)

| Property | Value |
|----------|-------|
| Resolution | 640 × 480 |
| Encoding | `rgb8` or `bgr8` from Gazebo bridge (YOLO path converts via numpy — no `cv_bridge`) |
| `header.frame_id` | `camera_optical_frame` |

### `/camera/camera_info` pairing

Published on the same prefix as the image (`/camera/camera_info`). Required for monocular range from bbox width.

**M1 SIL placeholder intrinsics (640×480):**

```
fx = fy = 554.25   # ~90° HFOV at 640 px width
cx = 320.0
cy = 240.0
```

Distortion coefficients zero until camera calibration (M4 gate).

## Gazebo SIL (M3)

### Camera model

Use PX4 target **`gz_x500_mono_cam`** — plain `gz_x500` has no onboard camera.

```bash
source /opt/ros/jazzy/setup.bash
cd ~/PX4-Autopilot
make px4_sitl gz_x500_mono_cam
```

### Camera bridge

Gazebo publishes on **gz-transport**; perception expects ROS topics on the frozen contract. Use `dondron_bringup` bridge launch (recommended):

```bash
export ROS_DOMAIN_ID=0
ros2 launch dondron_bringup camera_bridge.launch.py
```

Default gz source (verify with `gz topic -l | grep -i camera`):

```text
/world/default/model/x500_mono_cam_0/link/camera_link/sensor/camera/image
/world/default/model/x500_mono_cam_0/link/camera_link/sensor/camera/camera_info
```

Bridged ROS topics: `/camera/image_raw`, `/camera/camera_info`.

### Sim target

```bash
export ROS_LOG_DIR=/tmp/ros_log && mkdir -p /tmp/ros_log
ros2 launch dondron_bringup sim_target.launch.py
```

Default target: red stop sign (YOLO COCO class 11 filtered in node → contract class `"0"`). See `dondron_bringup/models/sim_target/`.

### Inference modes

| Mode | Launch | Node |
|------|--------|------|
| M1 stub | `use_stub:=true` (default) | C++ `perception_node` timer |
| M3 blob (Mac/CI) | `use_stub:=false` | C++ `perception_node` brightness blob |
| **M3 YOLO (Main PC GPU)** | `use_stub:=false use_yolo:=true` | Python `yolo_inference_node.py` |

**YOLO one-time venv setup (Main PC):**

```bash
python3 -m venv --system-site-packages ~/dondron_yolo_venv
~/dondron_yolo_venv/bin/pip install ultralytics
```

The YOLO node avoids `cv_bridge` (numpy/opencv ABI mismatch with system ROS Python) — converts `sensor_msgs/Image` to numpy directly. See script header in `scripts/yolo_inference_node.py`.

```bash
ros2 launch dondron_perception perception.launch.py use_stub:=false use_yolo:=true
```

YOLO filters pretrained detections to COCO class **`yolo_source_class_id`** (default `11` = stop sign) and publishes contract class **`target_class_id`** (default `"0"`). Publishes **all** boxes above `score_threshold`, not only the top-scoring one.

### Detection HUD (debug)

Optional overlay — not flight-critical:

```bash
ros2 run dondron_perception detection_visualizer.py
ros2 run rqt_image_view rqt_image_view /detections/image_annotated
```

### Verify

```bash
ros2 topic echo /detections --once
ros2 topic hz /camera/image_raw
```

M1 stub still publishes synthetic `/detections` without a live camera. M3+ publishes empty `detections: []` when nothing exceeds threshold.

Full SIL flow: [docs/sil-bridge.md](../../../docs/sil-bridge.md). Vault ops: `sil-m3-camera-view-and-manual-rc`.

## Build and run

```bash
cd ~/Projects/dondron/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select dondron_perception
source install/setup.bash
```

```bash
# M1 stub (no camera required)
ros2 run dondron_perception perception_node

# M3 blob inference (Mac/CI — requires image + camera_info)
ros2 run dondron_perception perception_node --ros-args -p use_stub:=false

# M3 YOLO GPU (Main PC — requires venv + ultralytics)
ros2 launch dondron_perception perception.launch.py use_stub:=false use_yolo:=true

# Or via launch (stub / blob)
ros2 launch dondron_perception perception.launch.py
ros2 launch dondron_perception perception.launch.py use_stub:=false
```

```bash
# Verify output
ros2 topic echo /detections --once
```

## Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `use_stub` | `true` | M1 timer stub (`true`) or image-callback inference (`false`) |
| `use_yolo` | `false` | Main PC: run `yolo_inference_node.py` instead of C++ node |
| `image_topic` | `/camera/image_raw` | Image subscription topic |
| `camera_info_topic` | `/camera/camera_info` | CameraInfo for monocular range (M3+) |
| `detections_topic` | `/detections` | Detection publisher topic |
| `frame_id` | `camera_optical_frame` | Output `header.frame_id` |
| `publish_rate_hz` | `2.0` | Stub publish rate (`use_stub:=true` only) |
| `score_threshold` | `0.5` | Minimum detection score (inference / YOLO) |
| `target_width_m` | `0.30` | Known physical width for monocular range |
| `target_class_id` | `"0"` | Contract class id published in `/detections` |
| `yolo_source_class_id` | `11` | COCO class YOLO filters on (11 = stop sign) |
| `model_path` | `yolov8n.pt` | Ultralytics weights (`use_yolo:=true`) |
| `yolo_python` | `~/dondron_yolo_venv/bin/python3` | Interpreter with ultralytics installed |
| `device` | `auto` | `cuda:0`, `cpu`, or auto-detect |
| `brightness_threshold` | `200` | CPU blob placeholder (Mac/CI; not YOLO) |
| `default_focal_length_px` | `554.25` | Fallback fx when CameraInfo not yet received |
| `stub_class_id` | `"0"` | Synthetic detection class |
| `stub_score` | `0.95` | Synthetic confidence |
| `stub_range_m` | `10.0` | Synthetic range in `pose.position.z` |
| `stub_bbox_center_x` | `320.0` | Synthetic bbox center X (px) |
| `stub_bbox_center_y` | `240.0` | Synthetic bbox center Y (px) |
| `stub_bbox_size_x` | `80.0` | Synthetic bbox width (px) |
| `stub_bbox_size_y` | `60.0` | Synthetic bbox height (px) |

## Roadmap

| Milestone | Scope |
|-----------|-------|
| **M1** | Stub publisher, topic contract freeze |
| **M3** | YOLO GPU inference, Gazebo sim target, camera bridge, empty-array behavior, detection HUD — **Main PC verified 2026-07-28** |
| **M4** | Calibrated intrinsics, real-field recognition gate |

Bringup integration: `dondron_bringup/launch/sil_public.launch.py`, `camera_bridge.launch.py`, `sim_target.launch.py`.
