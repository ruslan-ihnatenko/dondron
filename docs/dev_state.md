# DonDron — Development State

Code-repo session state (like UpAndDown `experiment_state_*.md`). Update at session end; vault task gets summary + branch/commit only.

## Last known SIL status

| Date | Machine | PX4 target | Agent connected | Notes |
|------|---------|------------|-----------------|-------|
| 2026-06-25 | Main PC | `gz_x500` | — | SIL stack ready per vault CONTEXT; packages not yet scaffolded |

## Package progress

- [ ] `dondron_description` — URDF/SDF, meshes
- [ ] `dondron_bringup` — launch files, system params
- [ ] `dondron_bridge` — Micro-XRCE-DDS agent config
- [ ] `dondron_perception` — `/detections` publisher
- [ ] `dondron_flight_api` — generic setpoint interface
- [ ] `dondron_state_machine` — BT through TRACK
- [ ] `dondron_diagnostics` — health monitoring, rosbag triggers

## Pending work

- [ ] Scaffold first ROS 2 package (`dondron_description` or `dondron_perception`)
- [ ] Optional: `docker compose build` smoke test on Main PC

## Known issues

- `ros2_ws/src/` is empty (`.gitkeep` only) — no colcon build yet
- Mac Dev Container: node dev only; full SIL on Main PC
