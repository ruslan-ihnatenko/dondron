# DonDron

Autonomous UAV R&D — ROS 2, PX4, Gazebo Harmonic, computer vision, BehaviorTree.CPP.

## Vault (tasks & architecture)

- **Nexus vault:** `~/Projects/nexus` → `01_Projects/Robotics/DonDron/`
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

## Layout

```
ros2_ws/src/          # ROS 2 packages (dondron_*)
simulation/worlds/      # Gazebo worlds
docker/               # Dev Container (future)
docs/                 # Build, deploy, run guides
```

## Main PC — quick start

```bash
source /opt/ros/jazzy/setup.bash
cd ~/PX4-Autopilot && make px4_sitl gz_x500   # SIL smoke test
```

## Repo

- **GitHub:** https://github.com/ruslan-ihnatenko/dondron
- **Local:** `~/Projects/dondron`
