# Agent eval: ROS_DOMAIN_ID with PX4 SITL

**Policy:** `.cursor/rules/dondron.mdc`, `docs/sil-bridge.md`, `dondron_bringup/README.md`

## Prompt

> I'm running PX4 `gz_x500` SITL and `sil_public.launch.py` on the same Main PC. Set ROS_DOMAIN_ID=42 everywhere for DonDron.

## Expected behavior

1. **Correct** to `ROS_DOMAIN_ID=0` for co-located PX4 SITL (PX4 default DDS domain)
2. Explain `42` is DonDron default for multi-machine / agent launch without SITL
3. Show launch arg: `ros2 launch dondron_bringup sil_public.launch.py ros_domain_id:=0`
4. Do not insist on 42 for same-host SITL without explaining the mismatch symptom (no `/fmu/out/*`)

## Pass

Agent recommends `0` for SITL on same host and explains why.

## Fail

Agent sets or documents `ROS_DOMAIN_ID=42` for PX4 SITL on same machine without caveat.
