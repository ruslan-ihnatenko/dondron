# Docker dev environment

Reproducible ROS 2 Jazzy workspace for **macOS** (Docker Desktop) and **Ubuntu** (Docker Engine). Matches the Main PC native stack without installing Ubuntu on the MacBook.

## Prerequisites

### MacBook Air M2

1. [Docker Desktop for Mac](https://docs.docker.com/desktop/setup/install/mac-install/) (Apple Silicon)
2. Start Docker Desktop before building or opening the Dev Container
3. Optional: XQuartz if you need GUI tools (RViz) from the container — expect sluggish performance on Air

### Main PC (Ubuntu 24.04)

- Docker Engine + Compose plugin (already installed per vault CONTEXT)
- For GPU vision later: [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)

## Quick start

From the repo root:

```bash
# Build the image (first time or after Dockerfile changes)
docker compose -f docker/docker-compose.yml build

# Interactive dev shell
docker compose -f docker/docker-compose.yml run --rm dev

# Inside the container
ros2 doctor
colcon build
```

## Cursor Dev Container (recommended on Mac)

1. Open `~/projects/dondron` in Cursor
2. Command palette → **Dev Containers: Reopen in Container**
3. Cursor builds the image and opens a terminal with ROS 2 sourced

The workspace folder inside the container is `/workspace/ros2_ws`.

## What's in the image

| Component | Notes |
|-----------|-------|
| Base | `osrf/ros:jazzy-desktop` (linux/amd64; Rosetta emulation on Apple Silicon) |
| Build | colcon, rosdep, vcstool, cmake |
| Sim bridge | `ros-jazzy-ros-gz` meta-package |

## Layout

```
docker/
├── Dockerfile
├── docker-compose.yml
└── entrypoint.sh      # sources ROS + workspace overlay
ros2_ws/               # mounted into container at /workspace/ros2_ws
```

Build artifacts (`build/`, `install/`, `log/`) stay on the host under `ros2_ws/` and are gitignored.

## Limits on Mac

| Workload | Mac (Docker) | Main PC (native) |
|----------|--------------|------------------|
| Write/build ROS 2 nodes | ✅ | ✅ |
| Full SIL (Gazebo + PX4) | ⚠️ slow / optional | ✅ primary |
| GPU vision (CUDA) | ❌ | ✅ RTX 4070 |
| HIL (Pixhawk USB) | ⚠️ USB passthrough awkward | ✅ preferred |

Heavy simulation stays on the Main PC. Use the Mac for node development, launch files, and BehaviorTree logic.

## Troubleshooting

**`Cannot connect to the Docker daemon`**

Start Docker Desktop and wait until the whale icon is steady.

**Build fails on Apple Silicon**

The upstream `osrf/ros:jazzy-desktop` image is **amd64-only**. Docker Desktop runs it under Rosetta emulation — this is expected and fine for writing/building nodes. `uname -m` inside the container will show `x86_64`. For full-speed native builds, use the Main PC (Ubuntu 24.04).

**`rosdep` errors for new packages**

Inside the container:

```bash
cd /workspace/ros2_ws
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
```

## Related

- Vault task: `01_Projects/Robotics/DonDron/Tasks/setup-docker-dev-environment.md`
- Main PC SIL: `make px4_sitl gz_x500` in `~/PX4-Autopilot` (native Ubuntu only)
