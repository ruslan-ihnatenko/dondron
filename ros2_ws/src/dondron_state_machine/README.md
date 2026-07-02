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
| `ExecuteSearchPattern` | Action | Generic search velocities via `/flight_api/cmd_setpoint` |
| `TargetAcquired` | Condition | Reads `/detections` for target class (read-only) |
| `TrackTarget` | Action | Maintains visual lock via `/detections` stability |
| `RTL` / `Land` | Action | DISENGAGE subtree on Offboard exit |

## Topics used

| Topic | Direction | Notes |
|-------|-----------|-------|
| `/detections` | subscribe (read-only) | `TargetAcquired`, `TrackTarget` |
| `/flight_api/cmd_setpoint` | publish | Search pattern only — generic velocities |
| `/flight_api/enable_offboard` | publish | Offboard mode request |
| `/fmu/in/vehicle_command` | publish | Arm, takeoff, RTL, land |
| `/fmu/out/vehicle_status` | subscribe | RC override / offboard monitoring |

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
