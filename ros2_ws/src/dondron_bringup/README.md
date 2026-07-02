# dondron_bringup

Line 1 launch composition — public stack only. No `sil_full.launch.py`, no `dondron_guidance_private`.

## Launch modes

| Launch | Purpose |
|--------|---------|
| `sil_manual.launch.py` | Micro-XRCE agent + env docs; manual / bench (no autonomy) |
| `sil_public.launch.py` | Perception + flight_api + state_machine through TRACK |

## sil_manual.launch.py

Starts the uXRCE-DDS agent (includes `dondron_bridge/agent.launch.py`).

**External prerequisites** — see [docs/sil-bridge.md](../../docs/sil-bridge.md):

1. Agent (T1) or `MicroXRCEAgent udp4 -p 8888`
2. PX4 SITL (T2): `source /opt/ros/jazzy/setup.bash && cd ~/PX4-Autopilot && make px4_sitl gz_x500`
3. **QGroundControl recommended (T5 or background):** clears `gcs_connection_lost` / preflight arm block in SITL

```bash
export ROS_DOMAIN_ID=0   # match PX4 SITL when on same machine
ros2 launch dondron_bringup sil_manual.launch.py ros_domain_id:=0
```

## sil_public.launch.py

One-command public recognition stack:

- `dondron_perception` — `/detections` stub
- `dondron_flight_api` — Offboard setpoint bridge
- `dondron_state_machine` — BT through TRACK

```bash
source /opt/ros/jazzy/setup.bash
source ~/Projects/dondron/ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=0   # with PX4 SITL on same host

# With agent + PX4 already running:
ros2 launch dondron_bringup sil_public.launch.py ros_domain_id:=0
```

**Camera remap:** `image_topic` defaults to gz_x500 Gazebo camera path; override if your world/model name differs:

```bash
ros2 launch dondron_bringup sil_public.launch.py \
  ros_domain_id:=0 \
  image_topic:=/world/default/model/x500_0/link/camera_link/sensor/imager/image
```

M1 perception stub publishes synthetic `/detections` without a live camera.

## SIL bench — PX4 params (T2 `pxh>`)

On a **fresh** `make px4_sitl gz_x500` session, **after QGC connects (T5)** and before `sil_public` launch:

```text
param set NAV_DLL_ACT 0
param set COM_LOW_BAT_ACT 0
param set COM_ARM_BAT_MIN -1
param set CBRK_SUPPLY_CHK 894281
```

Wait ~20 s after PX4 boot, then:

```text
commander check
```

Expect `Preflight check: OK`. If still `FAILED`, read the red items in QGC **Setup → Safety** (or PX4 log lines starting with `Preflight Fail:`). Do **not** launch `sil_public` until OK or you have confirmed QGC shows ready to arm.

`CBRK_SUPPLY_CHK` / `NAV_DLL_ACT` are **SIL bench only** — not for field hardware without review.

## Build

```bash
cd ~/Projects/dondron/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install
source install/setup.bash
```

## Public boundary

- No `dondron_guidance_private` import
- No `sil_full.launch.py` in this repo
