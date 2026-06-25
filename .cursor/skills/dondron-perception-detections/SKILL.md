---
name: dondron-perception-detections
description: Implement or stub dondron_perception /detections publisher. Use for perception node work, Detection2DArray, or camera pipeline integration.
---

# Perception — `/detections` publisher

Package: `dondron_perception`

## Topic contract

| Topic | Type | Role |
|-------|------|------|
| `/camera/image_raw` | `sensor_msgs/msg/Image` | input |
| `/detections` | `vision_msgs/msg/Detection2DArray` | output |

Freeze this contract at Line 1 gate (vault: `project-branches-and-repos.md`).

## Implementation options

### A. SIL stub (fastest integration)

- Timer or image callback publishes empty or synthetic `Detection2DArray`
- Proves bringup + topic wiring before RKNN/YOLO

### B. Gazebo camera path

- Subscribe to bridged Gazebo camera topic (may differ from `/camera/image_raw` — remap in launch)
- Optional: ground-truth bbox from sim plugins for SIL tests

### C. Full inference (later)

1. `cv_bridge` → OpenCV
2. YOLOv8n → RKNN INT8 on Orange Pi; CPU/GPU dev path on Main PC for training
3. Fill `vision_msgs/Detection2D` per detection: `bbox`, `results[].hypothesis.class_id`, `score`

## Range in message

Monocular estimate (Phase 1):

```
range_m = (focal_length_px × real_target_width_m) / bbox_width_px
```

Store range in a custom field or derived topic — document choice in package README. Known target width is required.

## Node skeleton checklist

- [ ] `package.xml`: `rclcpp`, `sensor_msgs`, `vision_msgs`, `cv_bridge`
- [ ] Image subscriber + detections publisher
- [ ] Launch file in `dondron_bringup` or package `launch/`
- [ ] `colcon build --packages-select dondron_perception`
- [ ] Verify: `ros2 topic echo /detections --once`

## Public boundary

- Publish `/detections` only — **no** flight setpoints from this node
- No detection→setpoint mapping (Module C — private repo)

See `.cursor/rules/perception.mdc` and vault blueprint §4.3.
