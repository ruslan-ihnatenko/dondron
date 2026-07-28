# DonDron — Development State

Code-repo session state (like UpAndDown `experiment_state_*.md`). Update at session end; vault task gets summary + branch/commit only.

## Last known SIL status

| Date | Machine | PX4 target | Agent connected | Notes |
|------|---------|------------|-----------------|-------|
| 2026-06-25 | Main PC | `gz_x500` | yes | Agent UDP 8888 → session established; `/fmu/out/*` on `ROS_DOMAIN_ID=0` (PX4 SITL default); `sensor_combined` echo OK; workspace overlay sourced |
| 2026-07-02 | Main PC | `gz_x500` | yes | `sil_public.launch.py` — Arm → Offboard → Climb → Search → **TRACK** (`visual lock acquired`); `/fmu/out/vehicle_status_v4`; SIL bench PX4 params in `dondron_bringup/README.md` |
| 2026-07-02 | Dev env | — | — | M2 packages build + node smoke test (no PX4); BT ticks Arm→…; topics `/detections`, `/flight_api/*`, `/fmu/in/*` verified |
| 2026-07-28 | Main PC | `gz_x500_mono_cam` | yes | **M3 Mode A + Mode B both verified.** Mode A (manual RC + real YOLOv8n GPU inference via `dondron_yolo_venv`): flown manually via QGC, `/detections` confirmed live. Mode B (autonomous BT: perception+flight_api+state_machine): reached **TRACK** — `TargetAcquired` SUCCESS, `"TRACK: visual lock acquired (class 0)"`, held with repeated `"TRACK: maintaining visual lock"` over ~7s. See "M3 Mode B verification notes" below for method and caveats. |

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
- [x] **M3** — YOLO + sim target in Gazebo — **Mac slice A done 2026-07-03**; **Main PC camera bridge + YOLO GPU + sim target + Mode A/B verified 2026-07-28**
- [ ] **M4** — Task `20260625-l1gate`: real-life recognition + Line 1 go/no-go metrics
- [ ] **Later** — Task `20260625-l1diag`: diagnostics + rosbag triggers

## M3 Mode B verification notes (2026-07-28, Main PC)

- **Method that worked reliably:** the shipped `ExecuteSearchPattern` (forward+yaw weave, default `duration_s=5.0`) produces a real-world trajectory that's sensitive to per-boot PX4 EKF/controller dynamics and was **not** reliably reproducible run-to-run (same params gave final drone poses differing by >1m / >15° across clean SITL reboots) — chasing its exact endpoint to place the target was unreliable and not recommended for regression testing.
- **Robust regression method (recommended going forward):** use a scratch BT XML (`bt_xml_path` launch arg, now forwarded through `sil_public.launch.py` → `state_machine.launch.py`) that neutralizes `ExecuteSearchPattern` to a near no-op (`duration_s="1.0" forward_speed_mps="0.0" yaw_rate_radps="0.0"`), so only `ClimbToAltitude` (deterministic vertical climb) happens before the acquire check. Place `sim_target` ~4 m directly ahead of the drone's fresh spawn pose (`gz model -m x500_mono_cam_0 -p`, before arming) using `sim_target.launch.py x:=4.0 y:=0.0 yaw:=1.5708`. This passed the first time it was tried under this scheme.
- **Known real bug found (not fixed, out of scope for M3 verification):** `ClimbToAltitude` (`bt_nodes.cpp`) is **open-loop** — it publishes a fixed climb-rate velocity for a fixed duration (`altitude_m/climb_rate_mps + 1.0`) with no altitude feedback. Two effects observed: (1) if the drone is already flying, re-triggering the BT (e.g. relaunching just `state_machine_node`) climbs *again* on top of the current altitude — altitudes compounded to 6-7 m across repeated relaunches in the same continuous flight; (2) on at least 2 of 5 clean-boot attempts the commanded climb produced almost no actual altitude gain (PX4/Gazebo SITL flakiness, possibly host CPU load from repeated restarts) even though the BT logged `SUCCESS`. Recommend a future task: closed-loop altitude check (read `/fmu/out/vehicle_local_position_v1.z`) before declaring `ClimbToAltitude` success.
- Confirmed `bt_xml_path` launch arg (already declared in `state_machine.launch.py`) was **not** forwarded through `sil_public.launch.py` — fixed as part of this session (see files touched below).

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

- **M3 (Main PC SIL):** `84aa0be` (`feat(bringup)`: camera bridge, sim targets, BT XML override), `b6c8e6c` (`feat(perception)`: YOLOv8n GPU inference + detection HUD) on `main` — Mode A + Mode B verified 2026-07-28
- **M2:** `1abc10f` on `main` — `dondron_flight_api`, `dondron_state_machine`, `dondron_bringup` (`sil_public`); Main PC SIL TRACK verified 2026-07-02
- **M1:** `b11c4e7` on `main` — `dondron_perception` stub, `/detections` contract README
- **M0:** `88db8a7` on `main` — `dondron_description`, `dondron_bridge`, dev state

## CI / automated tests

- GitHub Actions: `.github/workflows/ci.yml` — colcon build/test + `scripts/check-public-boundary.sh`
- Local: skill `dondron-colcon-test`
- gtest: `dondron_flight_api` (`test_frame_transform`), `dondron_perception` (`test_monocular_range`)
- launch_testing: `dondron_perception` (`test_perception_launch`, `test_perception_inference_*_launch`)
- Agent evals: `.cursor/evals/` (manual)
