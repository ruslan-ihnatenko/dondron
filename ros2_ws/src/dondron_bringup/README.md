# dondron_bringup

Line 1 launch composition — public stack only. No `sil_full.launch.py`, no `dondron_guidance_private`.

## Launch modes

| Launch | Purpose |
|--------|---------|
| `sil_manual.launch.py` | Micro-XRCE agent + env docs; manual / bench (no autonomy) |
| `sil_public.launch.py` | Perception + flight_api + state_machine through TRACK |
| `camera_bridge.launch.py` | Gazebo `x500_mono_cam` → `/camera/image_raw` + `/camera/camera_info` |
| `sim_target.launch.py` | Spawn default M3 sim target (red stop sign) |
| `spawn_orientation_props.launch.py` | Extra sign sizes + non-target orientation cubes (Mode A spatial refs) |

## SIL prerequisites

See [docs/sil-bridge.md](../../docs/sil-bridge.md):

1. **T1** — Micro-XRCE agent: `MicroXRCEAgent udp4 -p 8888`
2. **T2** — PX4 SITL:
   - M0–M2 smoke / stub TRACK: `make px4_sitl gz_x500`
   - **M3+ camera + YOLO:** `make px4_sitl gz_x500_mono_cam` (plain `gz_x500` has **no** camera)
3. **QGroundControl** (recommended): clears GCS preflight / arm block in SITL
4. **`ROS_DOMAIN_ID=0`** when PX4 SITL and ROS nodes share the same host

```bash
export ROS_DOMAIN_ID=0
export ROS_LOG_DIR=/tmp/ros_log && mkdir -p /tmp/ros_log   # before ros_gz_sim spawn
```

## camera_bridge.launch.py

Bridges gz-transport topics from `x500_mono_cam_0` to the frozen perception contract:

| ROS topic | Default gz source |
|-----------|-------------------|
| `/camera/image_raw` | `.../x500_mono_cam_0/.../camera/image` |
| `/camera/camera_info` | `.../x500_mono_cam_0/.../camera/camera_info` |

```bash
ros2 launch dondron_bringup camera_bridge.launch.py
```

If world/model name differs, override `gz_image_topic` / `gz_camera_info_topic` (check with `gz topic -l | grep -i camera` while sim runs).

## sim_target.launch.py

Spawns a static red stop-sign mesh (Fuel model + PBR material override) for YOLO class 11 ("stop sign") → contract class `"0"`.

```bash
ros2 launch dondron_bringup sim_target.launch.py
# Override pose:
ros2 launch dondron_bringup sim_target.launch.py x:=4.0 y:=0.0 yaw:=1.5708
```

Models live in `models/sim_target/` (plus `sim_target_small`, `sim_target_large`, `orientation_cube_*` for props launch).

## sil_manual.launch.py

Starts the uXRCE-DDS agent (includes `dondron_bridge/agent.launch.py`).

```bash
export ROS_DOMAIN_ID=0
ros2 launch dondron_bringup sil_manual.launch.py ros_domain_id:=0
```

Use with QGC for manual flight; add `camera_bridge` + `perception.launch.py use_yolo:=true` for Mode A recognition testing (no `state_machine`).

## sil_public.launch.py

Public recognition + autonomy stack:

- `dondron_perception` — stub, blob inference, or **YOLO GPU** (`use_yolo`)
- `dondron_flight_api` — Offboard setpoint bridge
- `dondron_state_machine` — BT through TRACK

```bash
source /opt/ros/jazzy/setup.bash
source ~/Projects/dondron/ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=0

# M2 stub TRACK (no camera):
ros2 launch dondron_bringup sil_public.launch.py ros_domain_id:=0

# M3 Mode B — real YOLO (camera_bridge must already be running):
ros2 launch dondron_bringup sil_public.launch.py \
  ros_domain_id:=0 \
  image_topic:=/camera/image_raw \
  camera_info_topic:=/camera/camera_info \
  use_stub:=false \
  use_yolo:=true
```

| Argument | Default | Notes |
|----------|---------|-------|
| `ros_domain_id` | `42` | Use **`0`** with PX4 SITL on same host |
| `image_topic` | gz `x500_0` imager path | Override to **`/camera/image_raw`** when using `camera_bridge` |
| `camera_info_topic` | `/camera/camera_info` | Required for monocular range with YOLO |
| `use_stub` | `true` | `false` for real inference |
| `use_yolo` | `false` | `true` for Main PC YOLOv8n GPU path |
| `bt_xml_path` | `""` | Optional scratch BT XML (for Mode B regression tuning) |
| `vehicle_status_topic` | `/fmu/out/vehicle_status_v4` | Match bridged PX4 topic |

**Mode B:** do not fly manually while Offboard is active. See vault ops note `sil-m3-camera-view-and-manual-rc`.

## SIL bench — PX4 params (T2 `pxh>`)

On a **fresh** SITL session, **after QGC connects** and before autonomous launch:

```text
param set NAV_DLL_ACT 0
param set COM_LOW_BAT_ACT 0
param set COM_ARM_BAT_MIN -1
param set CBRK_SUPPLY_CHK 894281
```

Wait ~20 s after PX4 boot, then `commander check` — expect `Preflight check: OK`.

`CBRK_SUPPLY_CHK` / `NAV_DLL_ACT` are **SIL bench only** — not for field hardware without review.

## Build

```bash
cd ~/Projects/dondron/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select dondron_bringup
source install/setup.bash
```

## Public boundary

- No `dondron_guidance_private` import
- No `sil_full.launch.py` in this repo
