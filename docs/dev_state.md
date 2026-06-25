# DonDron — Development State

Code-repo session state (like UpAndDown `experiment_state_*.md`). Update at session end; vault task gets summary + branch/commit only.

## Last known SIL status

| Date | Machine | PX4 target | Agent connected | Notes |
|------|---------|------------|-----------------|-------|
| 2026-06-25 | Main PC | `gz_x500` | yes | Agent UDP 8888 → session established; `/fmu/out/*` on `ROS_DOMAIN_ID=0` (PX4 SITL default); `sensor_combined` echo OK; workspace overlay sourced |

## Package progress

- [x] `dondron_description` — minimal URDF/xacro; `camera_optical_frame` documented
- [ ] `dondron_bringup` — launch files, system params
- [x] `dondron_bridge` — agent launch (`agent.launch.py`), `ROS_DOMAIN_ID=42` default
- [x] `dondron_perception` — M1 stub node; `/detections` contract in package README; `perception.launch.py`
- [ ] `dondron_flight_api` — generic setpoint interface
- [ ] `dondron_state_machine` — BT through TRACK
- [ ] `dondron_diagnostics` — health monitoring, rosbag triggers

## Pending work

Line 1 milestones — vault tasks in `01_Projects/Robotics/DonDron/Tasks/`:

- [x] **M0** — Vault task `20260625-l1found`: scaffold `dondron_description` + `dondron_bridge`; colcon smoke build
- [x] **M1** — Task `20260625-l1perc`: perception stub + topic contract freeze
- [ ] **M2** — Tasks `20260625-l1flight`, `20260625-l1bt`, `20260625-l1bring`: TRACK BT in `sil_public.launch.py`
- [ ] **M3** — YOLO + sim target in Gazebo (perception task continuation)
- [ ] **M4** — Task `20260625-l1gate`: real-life recognition + Line 1 go/no-go metrics
- [ ] **Later** — Task `20260625-l1diag`: diagnostics + rosbag triggers

## Known issues

- PX4 SITL uXRCE uses DDS domain **0**; DonDron fleet default is **42** — match domain when bridging SIL vs hardware
- Mac Dev Container: node dev only; full SIL on Main PC
- `px4_msgs` cloned locally under `ros2_ws/src/` (not in git) — needed for M2 `dondron_flight_api`; consider vcstool entry later

## Last commit (Line 1)

- **M1:** `b11c4e7` on `main` — `dondron_perception` stub, `/detections` contract README
- **M0:** `88db8a7` on `main` — `dondron_description`, `dondron_bridge`, dev state
