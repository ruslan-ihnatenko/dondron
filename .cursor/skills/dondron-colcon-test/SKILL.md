---
name: dondron-colcon-test
description: Run DonDron colcon build, test, and public boundary check. Use before commit, after package changes, or when verifying CI locally.
---

# DonDron colcon test

Fast local verification (no PX4/Gazebo). Runs in Dev Container or Main PC with ROS Jazzy.

## Prerequisites

```bash
source /opt/ros/jazzy/setup.bash
cd ~/Projects/dondron/ros2_ws
```

`px4_msgs` must exist in `ros2_ws/src/px4_msgs` (clone per `docs/sil-bridge.md`).

## Full workspace (pre-commit / milestone)

```bash
source /opt/ros/jazzy/setup.bash
cd ~/Projects/dondron/ros2_ws

# Boundary + launch syntax (from repo root)
cd ~/Projects/dondron
./scripts/check-public-boundary.sh

cd ~/Projects/dondron/ros2_ws
colcon build --symlink-install \
  --packages-select \
    dondron_description dondron_bridge dondron_perception \
    dondron_flight_api dondron_state_machine dondron_bringup px4_msgs

source install/setup.bash
colcon test \
  --packages-select \
    dondron_description dondron_bridge dondron_perception \
    dondron_flight_api dondron_state_machine dondron_bringup
colcon test-result --verbose
```

## Single package (during development)

```bash
colcon build --symlink-install --packages-select dondron_perception
source install/setup.bash
colcon test --packages-select dondron_perception
colcon test-result --verbose
```

Replace package name as needed.

## What runs where

| Check | Tier | Needs PX4? |
|-------|------|------------|
| `check-public-boundary.sh` | 1 | No |
| `colcon build` | 1 | No |
| `ament_lint` | 1 | No |
| `test_frame_transform` (gtest) | 2 | No |
| `test_perception_launch` | 3 | No |
| SIL smoke | 4 | Yes — skill `dondron-sil-smoke-test` |

## Pass criteria

- [ ] Boundary script exits 0
- [ ] `colcon build` succeeds
- [ ] `colcon test-result` shows 0 failures

## On failure

| Symptom | Action |
|---------|--------|
| `px4_msgs` not found | `git clone https://github.com/PX4/px4_msgs.git ros2_ws/src/px4_msgs` |
| Boundary fail | Read script output; check `flight_api` / `mission.xml` / private imports |
| Launch test timeout | Ensure no stale ROS daemons; retry with `ROS_DOMAIN_ID=99` |
| flake8 on test imports | Match order in existing `launch/*.py` files |

## CI mirror

GitHub Actions workflow: `.github/workflows/ci.yml` — same build + test + boundary on push/PR.
