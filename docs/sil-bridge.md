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
make px4_sitl gz_x500              # GUI
# HEADLESS=1 make px4_sitl gz_x500  # headless
```

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
ros2 topic echo /fmu/out/vehicle_status --once
```

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
