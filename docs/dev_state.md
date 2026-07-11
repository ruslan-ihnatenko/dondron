# DonDron — Development State

Code-repo session state (like UpAndDown `experiment_state_*.md`). Update at session end; vault task gets summary + branch/commit only.

## Last known SIL status

| Date | Machine | PX4 target | Agent connected | Notes |
|------|---------|------------|-----------------|-------|
| 2026-06-25 | Main PC | `gz_x500` | yes | Agent UDP 8888 → session established; `/fmu/out/*` on `ROS_DOMAIN_ID=0` (PX4 SITL default); `sensor_combined` echo OK; workspace overlay sourced |
| 2026-07-02 | Main PC | `gz_x500` | yes | `sil_public.launch.py` — Arm → Offboard → Climb → Search → **TRACK** (`visual lock acquired`); `/fmu/out/vehicle_status_v4`; SIL bench PX4 params in `dondron_bringup/README.md` |
| 2026-07-02 | Dev env | — | — | M2 packages build + node smoke test (no PX4); BT ticks Arm→…; topics `/detections`, `/flight_api/*`, `/fmu/in/*` verified |

## Package progress

- [x] `dondron_description` — minimal URDF/xacro; `camera_optical_frame` documented
- [x] `dondron_bringup` — `sil_manual.launch.py`, `sil_public.launch.py`; package README
- [x] `dondron_bridge` — agent launch (`agent.launch.py`), `ROS_DOMAIN_ID=42` default
- [x] `dondron_perception` — M1 stub + **M3 inference contract** (Mac slice A): `use_stub`, monocular range, synthetic-image launch tests; YOLO scaffold only
- [x] `dondron_flight_api` — `geometry_msgs/TwistStamped` cmd_setpoint; `FlightApiStatus`; harness; Offboard bridge
- [x] `dondron_state_machine` — BTCPP v4 through TRACK; `mission.xml`; no ENGAGE
- [ ] `dondron_diagnostics` — health monitoring, rosbag triggers

## Pending work

Line 1 milestones — vault tasks in `01_Projects/Robotics/DonDron/Tasks/`:

- [x] **M0** — Vault task `20260625-l1found`: scaffold `dondron_description` + `dondron_bridge`; colcon smoke build
- [x] **M1** — Task `20260625-l1perc`: perception stub + topic contract freeze
- [x] **M2** — Tasks `20260625-l1flight`, `20260625-l1bt`, `20260625-l1bring`: TRACK BT in `sil_public.launch.py` — **Main PC SIL verified 2026-07-02**, committed on `main`
- [ ] **M3** — YOLO + sim target in Gazebo — **Mac slice A done 2026-07-03** (inference contract + tests); Main PC: camera bridge, YOLO GPU, sim target, Mode A/B verify
- [ ] **M4** — Task `20260625-l1gate`: real-life recognition + Line 1 go/no-go metrics
- [ ] **Later** — Task `20260625-l1diag`: diagnostics + rosbag triggers

## Known issues

- PX4 SITL publishes `/fmu/out/vehicle_status_v4` (not unversioned `vehicle_status`) — `vehicle_status_topic` param defaults to `_v4`; match `px4_msgs` to PX4 firmware
- PX4 subscriptions use `SensorDataQoS` (best effort) to match uXRCE bridge
- Offboard mode command must use `DO_SET_MODE` **param2=6** (main mode), not nav_state 14
- SITL bench: battery failsafe can RTL immediately after takeoff — disable via params in `dondron_bringup/README.md` (`NAV_DLL_ACT`, `COM_LOW_BAT_ACT`, etc.)
- Search-pattern sway in Gazebo is expected (oscillating yaw rate at 10 Hz BT tick); TRACK holds visual lock only (no closed-loop flight)
- Mac Dev Container: node dev only; full SIL on Main PC
- M3 perception: Mac-verified `use_stub:=false` contract (gtest monocular range + synthetic Image/CameraInfo launch tests); CPU blob placeholder until Main PC YOLO
- `px4_msgs` — local workspace clone (`ros2_ws/src/px4_msgs`); not vendored in git; clone per `docs/sil-bridge.md`

## Last commit (Line 1)

- **M2:** `1abc10f` on `main` — `dondron_flight_api`, `dondron_state_machine`, `dondron_bringup` (`sil_public`); Main PC SIL TRACK verified 2026-07-02
- **M1:** `b11c4e7` on `main` — `dondron_perception` stub, `/detections` contract README
- **M0:** `88db8a7` on `main` — `dondron_description`, `dondron_bridge`, dev state

## CI / automated tests

- GitHub Actions: `.github/workflows/ci.yml` — colcon build/test + `scripts/check-public-boundary.sh`
- Local: skill `dondron-colcon-test`
- gtest: `dondron_flight_api` (`test_frame_transform`), `dondron_perception` (`test_monocular_range`)
- launch_testing: `dondron_perception` (`test_perception_launch`, `test_perception_inference_*_launch`)
- Agent evals: `.cursor/evals/` (manual)
