# dondron_flight_api

Line 1 Module B — generic setpoint interface to PX4 via Micro-XRCE-DDS. **Detection-agnostic:** this package does **not** subscribe to `/detections`.

## Topics (frozen)

| Topic | Type | Direction | Notes |
|-------|------|-----------|-------|
| `/flight_api/cmd_setpoint` | `geometry_msgs/msg/TwistStamped` | subscribe | Velocity command input |
| `/flight_api/status` | `dondron_flight_api/msg/FlightApiStatus` | publish | Armed + offboard state for BT |
| `/flight_api/enable_offboard` | `std_msgs/msg/Bool` | subscribe | Request PX4 Offboard mode |
| `/fmu/in/offboard_control_mode` | `px4_msgs/msg/OffboardControlMode` | publish | Velocity offboard mode |
| `/fmu/in/trajectory_setpoint` | `px4_msgs/msg/TrajectorySetpoint` | publish | NED velocity setpoints |
| `/fmu/in/vehicle_command` | `px4_msgs/msg/VehicleCommand` | publish | Offboard mode switch |
| `/fmu/out/vehicle_status` | `px4_msgs/msg/VehicleStatus` | subscribe | Status for `/flight_api/status` |
| `/fmu/out/vehicle_attitude` | `px4_msgs/msg/VehicleAttitude` | subscribe | Body→NED transform |

**Message type decision:** `geometry_msgs/TwistStamped` (not a custom `dondron_msgs` package). Custom `FlightApiStatus` is defined in this package for BT status only.

## `/flight_api/cmd_setpoint` contract

| Field | Semantics |
|-------|-----------|
| `header.frame_id` | `body_frd` (default) or `ned` |
| `twist.linear.x/y/z` | Velocity [m/s] in the declared frame |
| `twist.angular.z` | Yaw rate [rad/s] in NED (positive = clockwise when viewed from above) |

**Frame handling:** When `frame_id` is `body_frd`, `flight_api_node` rotates linear velocity to NED using the latest `/fmu/out/vehicle_attitude` quaternion (body FRD → NED). When `frame_id` is `ned`, values pass through directly.

**Stale commands:** If no message arrives within `cmd_timeout_s` (default 0.5 s), the node publishes zero velocity hold.

**Publish rate:** PX4 offboard streams at `setpoint_rate_hz` (default 20 Hz). Input commands may arrive slower; the latest command is held until timeout.

## `/flight_api/status` contract

| Field | Semantics |
|-------|-----------|
| `armed` | `true` when PX4 `arming_state == ARMED` |
| `offboard_active` | `true` when `nav_state == OFFBOARD` |
| `px4_nav_state` | Raw PX4 `nav_state` byte |
| `nav_state_name` | Human-readable nav state string |

## Offboard mode management

1. Node streams `OffboardControlMode` (velocity) + `TrajectorySetpoint` at `setpoint_rate_hz`.
2. Publish `true` on `/flight_api/enable_offboard` (or set `auto_request_offboard` + armed vehicle) to send `VEHICLE_CMD_DO_SET_MODE` with **param2=6** (PX4 offboard main mode — not nav_state 14).
3. Streams at least 10 setpoint cycles before requesting offboard (PX4 requirement).

## Bench harness

```bash
source /opt/ros/jazzy/setup.bash
source ~/Projects/dondron/ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=0   # match PX4 SITL

# Terminal: flight API + harness (with agent + PX4 running)
ros2 launch dondron_flight_api flight_api.launch.py
ros2 launch dondron_flight_api harness.launch.py
```

Harness publishes forward body velocity + sinusoidal yaw rate and requests offboard after 2 s.

## Build

```bash
cd ~/Projects/dondron/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select px4_msgs dondron_flight_api
source install/setup.bash
```

## Public boundary

- **Must not** subscribe to `/detections`
- No visual servo or bbox→setpoint mapping (Module C — private repo Line 2+)
