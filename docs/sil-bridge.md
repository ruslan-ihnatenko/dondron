# SIL bridge — PX4, Gazebo, Micro-XRCE-DDS

Main PC native software-in-the-loop stack. Vault reference: `01_Projects/Robotics/DonDron/Tasks/setup-micro-xrce-dds.md`.

## Important: source ROS 2 first

**Always source ROS 2 before any PX4 build or sim launch on this machine.**

Gazebo Harmonic is installed via ROS 2 Jazzy vendor packages (`ros-jazzy-ros-gz`), not as a separate system install. Without ROS sourced, PX4 CMake cannot find `gz-sim` / `gz-transport`, the Gazebo bridge is disabled, and targets like `gz_x500` will not exist.

```bash
source /opt/ros/jazzy/setup.bash   # also in ~/.bashrc on Main PC
```

## Components

| Component | Path / command |
|-----------|----------------|
| ROS 2 Jazzy | `/opt/ros/jazzy` |
| PX4 Autopilot | `~/PX4-Autopilot` |
| Micro-XRCE-DDS Agent | `~/Micro-XRCE-DDS-Agent/build/MicroXRCEAgent` |
| px4_msgs (ROS 2) | `ros2_ws/src/px4_msgs` |

PX4 SITL starts the uXRCE-DDS **client** on UDP `127.0.0.1:8888`. The **agent** must be running first.

## Launch flow (three terminals)

### Terminal 1 — Micro-XRCE-DDS Agent

```bash
~/Micro-XRCE-DDS-Agent/build/MicroXRCEAgent udp4 -p 8888
```

Leave running. One agent per UDP port.

Optional system-wide install (requires sudo once):

```bash
cd ~/Micro-XRCE-DDS-Agent/build && sudo make install && sudo ldconfig /usr/local/lib/
# then: MicroXRCEAgent udp4 -p 8888
```

### Terminal 2 — PX4 SITL + Gazebo

```bash
source /opt/ros/jazzy/setup.bash
cd ~/PX4-Autopilot
make px4_sitl gz_x500              # GUI — M0–M2 smoke (no onboard camera)
# HEADLESS=1 make px4_sitl gz_x500  # headless smoke

# M3+ perception (monocular camera on airframe):
make px4_sitl gz_x500_mono_cam     # GUI
# HEADLESS=1 make px4_sitl gz_x500_mono_cam
```

| PX4 target | Camera | Use |
|------------|--------|-----|
| `gz_x500` | none | Agent/topic smoke, M2 TRACK with stub detections |
| `gz_x500_mono_cam` | forward mono | M3 YOLO + `/camera/image_raw` (see below) |

When the agent connects, PX4 logs lines like:

```text
INFO  [uxrce_dds_client] successfully created rt/fmu/out/sensor_combined data writer
```

Expected preflight warnings without QGroundControl: `No connection to the GCS`, `system power unavailable`.

### Terminal 3 — ROS 2

```bash
source /opt/ros/jazzy/setup.bash
source ~/Projects/dondron/ros2_ws/install/setup.bash
ros2 topic list | grep fmu
```

You should see topics such as `/fmu/out/sensor_combined`, `/fmu/out/vehicle_status`, etc.

Quick echo test:

```bash
ros2 topic echo /fmu/out/vehicle_status_v4 --once
```

## M3 perception stack (Main PC)

After T1 (agent) + T2 (`gz_x500_mono_cam`) are running:

```bash
export ROS_DOMAIN_ID=0
export ROS_LOG_DIR=/tmp/ros_log   # required for ros_gz_sim spawn
mkdir -p /tmp/ros_log

source /opt/ros/jazzy/setup.bash
source ~/Projects/dondron/ros2_ws/install/setup.bash

# Bridge Gazebo camera → frozen contract topics
ros2 launch dondron_bringup camera_bridge.launch.py

# Spawn sim target(s) — default red stop-sign at (5, 0)
ros2 launch dondron_bringup sim_target.launch.py
# Optional: extra sign sizes + orientation cubes
ros2 launch dondron_bringup spawn_orientation_props.launch.py
```

Verify camera + bridge:

```bash
ros2 topic hz /camera/image_raw
gz topic -l | grep -i camera   # if bridge fails — check model name x500_mono_cam_0
```

**YOLO venv (one-time Main PC setup):**

```bash
python3 -m venv --system-site-packages ~/dondron_yolo_venv
~/dondron_yolo_venv/bin/pip install ultralytics
```

**Mode A — manual flight + real detections** (no BT, no Offboard):

```bash
ros2 launch dondron_perception perception.launch.py use_stub:=false use_yolo:=true
ros2 run dondron_perception detection_visualizer.py   # optional HUD
ros2 run rqt_image_view rqt_image_view /detections/image_annotated
```

Arm and fly via QGroundControl (Position/Altitude — not Offboard).

**Mode B — autonomous TRACK with YOLO** (after Mode A works):

```bash
ros2 launch dondron_bringup sil_public.launch.py \
  ros_domain_id:=0 \
  image_topic:=/camera/image_raw \
  camera_info_topic:=/camera/camera_info \
  use_stub:=false \
  use_yolo:=true
```

Do not use manual sticks during Mode B. For repeatable Mode B regression (search-pattern drift), see `docs/dev_state.md` → **M3 Mode B verification notes** (`bt_xml_path` + scratch BT).

Vault ops guide: `01_Projects/Robotics/DonDron/Docs/notes/sil-m3-camera-view-and-manual-rc.md`.

## Build px4_msgs (after clone or PX4 upgrade)

Message definitions must match the PX4 firmware version. Clone `main` for current PX4 `main`/alpha; use a matching `release/X.Y` branch for stable PX4 releases.

```bash
source /opt/ros/jazzy/setup.bash
cd ~/Projects/dondron/ros2_ws
colcon build --symlink-install
source install/setup.bash
```

## Endpoints (SITL)

| Service | Endpoint |
|---------|----------|
| uXRCE-DDS | UDP `127.0.0.1:8888` |
| MAVLink (GCS) | UDP `14550` (remote), `18570` (local) |

## QGroundControl

Daily Build AppImage: `~/Applications/QGroundControl/`

```bash
~/Applications/QGroundControl/run-qgc.sh
```

With PX4 SITL running, QGC should auto-connect over MAVLink (UDP). The `Preflight Fail: No connection to the GCS` warning in PX4 should clear.

**One-time system setup (sudo):** GStreamer, FUSE, serial access — see [docs/main-pc-setup.md](main-pc-setup.md#qgroundcontrol).

## Related

- [docs/docker.md](docker.md) — container dev (Mac + Main PC)
- [docs/main-pc-setup.md](main-pc-setup.md) — QGC + Docker install (Mint 22)
- Vault: `01_Projects/Robotics/DonDron/Docs/mint22-dev-setup-notes.md`
