# dondron_state_machine

Line 1 public behavior tree — states through **TRACK** only. No ENGAGE subtree, no detection→setpoint mapping.

## Public state graph

```
IDLE → ARM → TAKEOFF → SEARCH → ACQUIRE → TRACK
```

**TRACK** is the public terminal autonomous state. Visual lock is maintained in BT state by monitoring `/detections` stability — not by publishing bbox-derived setpoints.

## BT nodes

| Node | Type | Role |
|------|------|------|
| `RC_Override_Not_Active` | Condition | Fails when PX4 leaves Offboard (RC override / mode change) |
| `Arm` | Action | `VEHICLE_CMD_COMPONENT_ARM_DISARM` |
| `Takeoff` | Action | `VEHICLE_CMD_NAV_TAKEOFF` |
| `EnableOffboard` | Action | Publishes `/flight_api/enable_offboard` |
| `WaitOffboard` | Action | Waits for Offboard nav state |
| `ClimbToAltitude` | Action | Closed-loop climb to target height via `/fmu/out/vehicle_local_position_v1` |
| `ExecuteSearchPattern` | Action | Weave search (forward + yaw) via `/flight_api/cmd_setpoint` |
| `ExecuteOrbitSearch` | Action | Closed-loop circular orbit (NED tangent velocity from local position) |
| `TargetAcquired` | Condition | Reads `/detections` for target class (read-only) |
| `TrackTarget` | Action | Maintains visual lock via `/detections` stability |
| `RTL` / `Land` | Action | DISENGAGE subtree on Offboard exit |

## Topics used

| Topic | Direction | Notes |
|-------|-----------|-------|
| `/detections` | subscribe (read-only) | `TargetAcquired`, `TrackTarget` |
| `/flight_api/cmd_setpoint` | publish | Climb + search pattern velocities |
| `/flight_api/enable_offboard` | publish | Offboard mode request |
| `/fmu/in/vehicle_command` | publish | Arm, takeoff, RTL, land |
| `/fmu/out/vehicle_status_v4` | subscribe | RC override / offboard monitoring |
| `/fmu/out/vehicle_local_position_v1` | subscribe | Closed-loop altitude in `ClimbToAltitude` |

## ClimbToAltitude (closed-loop)

Reads PX4 fused local position (`z` in NED → height = `-z` m above local origin). Publishes NED velocity setpoints on `/flight_api/cmd_setpoint` until measured height reaches `altitude_m` within `tolerance_m` (default 0.3 m). If already at/above the target band on entry (e.g. BT relaunch mid-flight), returns SUCCESS without climbing again. Times out to FAILURE after `timeout_s` (default 30 s) if altitude is never reached or no position messages arrive.

BT ports: `altitude_m`, `climb_rate_mps`, `tolerance_m`, `timeout_s`.

## Search pattern switch

ROS param / launch arg `search_pattern`:

| Value | BT node | Behavior |
|-------|---------|----------|
| `weave` (default) | `ExecuteSearchPattern` | Forward + sin yaw weave (M2/M3 default) |
| `orbit` | `ExecuteOrbitSearch` | CCW orbit: expand to radius, tangent velocity + yaw nose toward center (default in `mission.xml`: 25 m, 1.2 m/s, center 8.5 N / 0 E, 120 s) |

Set on `state_machine_node` or via `sil_public.launch.py search_pattern:=orbit`.

For camera + bbox overlay during orbit: `gz_x500_mono_cam`, `camera_bridge`, sim props, `use_yolo:=true`, then either:

```bash
ros2 run dondron_perception detection_visualizer.py --ros-args -p show_window:=true
# or
ros2 run rqt_image_view rqt_image_view /detections/image_annotated
```

## Main PC SIL verify (ClimbToAltitude)

```bash
# Terminal stack: agent → PX4 gz_x500_mono_cam → sil_public (see docs/sil-bridge.md)
ros2 topic list | grep local_position   # expect /fmu/out/vehicle_local_position_v1
ros2 topic echo /fmu/out/vehicle_local_position_v1 --field z

# Mid-flight relaunch regression: while drone is airborne after first climb,
# kill and relaunch only state_machine_node — must log
# "already at X m (target 3.0 m) — skipping climb" and NOT compound altitude.
```

## Build

```bash
sudo apt install ros-jazzy-behaviortree-cpp   # if not in dev image
cd ~/Projects/dondron/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select dondron_state_machine
source install/setup.bash
```

## Run (with perception stub + flight_api)

```bash
export ROS_DOMAIN_ID=42
ros2 launch dondron_state_machine state_machine.launch.py
```

For SIL with PX4 on domain 0, launch via `dondron_bringup/sil_public.launch.py` with `ros_domain_id:=0`.

## Public boundary

- No ENGAGE actions
- No bbox→setpoint mapping in BT nodes
- `TrackTarget` does not publish flight commands — visual lock only
