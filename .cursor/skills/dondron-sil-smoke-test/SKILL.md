---
name: dondron-sil-smoke-test
description: Run Main PC SIL smoke test — Micro-XRCE agent, PX4 gz_x500, ROS 2 topic checks. Use when testing SIL, PX4 bridge, or verifying sim stack.
---

# SIL smoke test (Main PC)

**Machine:** Main PC native only. Do not run full SIL on Mac Dev Container.

Reference: `docs/sil-bridge.md`

## Prerequisites

- ROS 2 Jazzy sourced: `source /opt/ros/jazzy/setup.bash`
- `~/PX4-Autopilot` built with Gazebo bridge (`gz_x500` target exists)
- `~/Micro-XRCE-DDS-Agent` built

## Three terminals

### Terminal 1 — Agent (start first)

```bash
~/Micro-XRCE-DDS-Agent/build/MicroXRCEAgent udp4 -p 8888
```

Leave running.

### Terminal 2 — PX4 SITL + Gazebo

```bash
source /opt/ros/jazzy/setup.bash
cd ~/PX4-Autopilot
make px4_sitl gz_x500
# HEADLESS=1 make px4_sitl gz_x500   # no GUI
```

**Success indicators in PX4 log:**

```text
INFO  [uxrce_dds_client] successfully created rt/fmu/out/sensor_combined data writer
```

Expected without QGC: `No connection to the GCS`, `system power unavailable`.

### Terminal 3 — ROS 2 verification

```bash
source /opt/ros/jazzy/setup.bash
export ROS_DOMAIN_ID=42
# If dondron workspace built:
# source ~/Projects/dondron/ros2_ws/install/setup.bash

ros2 topic list | grep fmu
ros2 topic echo /fmu/out/vehicle_status --once
```

## Pass criteria

- [ ] Agent running on UDP 8888
- [ ] PX4 SITL starts Gazebo with x500
- [ ] uXRCE client creates `/fmu/out/*` writers
- [ ] ROS 2 sees bridged PX4 topics

## On failure

| Symptom | Check |
|---------|-------|
| `gz_x500` target missing | Source ROS before `make px4_sitl` |
| No `/fmu/out/*` topics | Agent started before PX4? Port 8888 free? |
| ROS sees no topics | `ROS_DOMAIN_ID` mismatch; source workspace overlay |

## After test

Update `docs/dev_state.md` → Last known SIL status table.
