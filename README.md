# DonDron

Autonomous UAV R&D — ROS 2, PX4, Gazebo Harmonic, computer vision, BehaviorTree.CPP.

## Vault (tasks & architecture)

- **Nexus vault:** `~/projects/nexus` → `01_Projects/Robotics/DonDron/`
- **Blueprint:** `hybrid-architecture-blueprint.md` in vault (primary reference — not duplicated here)
- **Algorithms reference:** `uav-architecture-master-plan.md` in vault
- **Task board:** `01_Projects/Robotics/DonDron/Tasks/_board.md`

## Stack

| Layer | Technology |
|-------|------------|
| Middleware | ROS 2 **Jazzy** |
| Autopilot | PX4 (upstream at `~/PX4-Autopilot`) |
| Simulator | Gazebo Harmonic |
| Bridge | Micro-XRCE-DDS |

## Agent workflow

- **Guardrails:** [AGENTS.md](AGENTS.md)
- **Dev state:** [docs/dev_state.md](docs/dev_state.md) — update at session end
- **Cursor rules:** `.cursor/rules/` — stack conventions, public/legal boundary, git workflow (`github.mdc`)
- **Cursor skills:** `.cursor/skills/` — package scaffold, SIL smoke test, session handoff, **git commit**

## Layout

```
ros2_ws/src/          # ROS 2 packages (dondron_*)
simulation/worlds/    # Gazebo worlds
docker/               # Dev Container image + compose
.devcontainer/        # Cursor / VS Code Remote Containers
docs/                 # Build, deploy, run guides
```

## MacBook — ROS 2 development (Docker)

ROS 2 Jazzy is not installed natively on macOS. Use the Dev Container:

1. Install and start **Docker Desktop**
2. Open this repo in Cursor → **Dev Containers: Reopen in Container**

Or from a terminal:

```bash
docker compose -f docker/docker-compose.yml build
docker compose -f docker/docker-compose.yml run --rm dev
```

Full details: [docs/docker.md](docs/docker.md)

## Main PC — quick start (native SIL)

**Always source ROS 2 before any PX4 build or sim launch on this machine.** Gazebo Harmonic comes from ROS 2 Jazzy vendor packages (`ros-jazzy-ros-gz`), not a separate system install. Without ROS sourced, PX4 cannot find Gazebo CMake packages and sim targets like `gz_x500` will not be built.

```bash
source /opt/ros/jazzy/setup.bash   # also in ~/.bashrc on Main PC
cd ~/PX4-Autopilot && make px4_sitl gz_x500   # SIL smoke test
```

PX4 ↔ ROS 2 bridge (Micro-XRCE-DDS): [docs/sil-bridge.md](docs/sil-bridge.md)

## Repo

- **GitHub:** https://github.com/ruslan-ihnatenko/dondron
- **Local:** `~/projects/dondron`
