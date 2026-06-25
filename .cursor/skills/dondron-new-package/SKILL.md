---
name: dondron-new-package
description: Scaffold a new dondron_* ROS 2 package in ros2_ws/src. Use when creating packages, scaffolding nodes, or adding a new dondron component.
---

# Scaffold dondron_* ROS 2 package

## Prerequisites

- Workspace: `~/Projects/dondron/ros2_ws`
- Source ROS: `source /opt/ros/jazzy/setup.bash`
- Mac: run inside Dev Container; Main PC: native or container

## Steps

### 1. Create package

```bash
cd ~/Projects/dondron/ros2_ws/src
ros2 pkg create --build-type ament_cmake dondron_<name> \
  --dependencies rclcpp std_msgs
```

Add deps as needed (`sensor_msgs`, `vision_msgs`, `px4_msgs`, `behaviortree_cpp`, etc.).

### 2. Wire CMakeLists.txt and package.xml

- `find_package` + `ament_target_dependencies` for every dep
- Install targets, headers, launch files if present

### 3. Minimal node (optional)

- `src/<node>.cpp` with `rclcpp::Node` subclass
- `main()` spins node — proves build before feature work

### 4. Build and smoke test

```bash
cd ~/Projects/dondron/ros2_ws
colcon build --symlink-install --packages-select dondron_<name>
source install/setup.bash
ros2 run dondron_<name> <executable>   # if added
```

### 5. Register in bringup (when runnable)

Add to `dondron_bringup` launch file when the node is part of a stack.

### 6. Update tracking

- `docs/dev_state.md` — check package progress box
- Vault task — note package name if part of active task

## Public boundary check

- `dondron_flight_api`: no `/detections` subscription
- No guidance/intercept logic in any public package
- State machine: public states through TRACK only

See `.cursor/rules/ros2-package.mdc` and `AGENTS.md`.
