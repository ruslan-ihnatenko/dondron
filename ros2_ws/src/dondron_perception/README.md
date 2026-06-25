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
| Encoding | `rgb8` (inference path uses `cv_bridge` in M3+) |
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

## Gazebo SIL remap

PX4 `gz_x500` ships a forward camera on the airframe model. The raw Gazebo topic name includes the world and model instance; **remap in launch** to the frozen `/camera/image_raw` contract.

Typical Gazebo Harmonic topic (verify with `gz topic -l` while sim is running):

```text
/world/default/model/x500_0/link/camera_link/sensor/imager/image
```

**Launch remap strategy** (until `dondron_bringup` `sil_public.launch.py` in M2):

```bash
# Terminal: perception stub with Gazebo image source
source /opt/ros/jazzy/setup.bash
source ~/Projects/dondron/ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=0   # match PX4 SITL when testing /fmu/out on same machine

ros2 launch dondron_perception perception.launch.py \
  image_topic:=/world/default/model/x500_0/link/camera_link/sensor/imager/image
```

Or run the node with remaps:

```bash
ros2 run dondron_perception perception_node --ros-args \
  -r /camera/image_raw:=/world/default/model/x500_0/link/camera_link/sensor/imager/image
```

**Bridge checklist (M3):**

1. Start Micro-XRCE agent + PX4 `gz_x500` (see `docs/sil-bridge.md`).
2. Run `ros_gz_image` / bridge launch to expose camera topics on ROS 2.
3. Launch `dondron_perception` with `image_topic` remap as above.
4. Verify: `ros2 topic echo /detections --once` → `header.frame_id: camera_optical_frame`.

M1 stub publishes valid `/detections` **without** a live camera (timer-only synthetic bbox). Image subscription proves topic wiring when a camera is available.

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

# Or via launch
ros2 launch dondron_perception perception.launch.py
```

```bash
# Verify output
ros2 topic echo /detections --once
```

## Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `image_topic` | `/camera/image_raw` | Image subscription topic |
| `detections_topic` | `/detections` | Detection publisher topic |
| `frame_id` | `camera_optical_frame` | Output `header.frame_id` |
| `publish_rate_hz` | `2.0` | Stub publish rate |
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
| **M1** (this) | Stub publisher, topic contract freeze |
| **M3** | `cv_bridge` + YOLO inference, Gazebo sim target, empty-array behavior |
| **M4** | Calibrated intrinsics, real-field recognition gate |

Bringup integration: `dondron_bringup/launch/sil_public.launch.py` (M2).
