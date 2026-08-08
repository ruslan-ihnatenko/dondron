"""
Spawn extra M3 SIL props for manual-flight spatial awareness (Main PC only).

Adds two more sim-target sizes (for range/detection testing at different
apparent target widths) plus three plain colored cubes that are NOT
recognition targets — just visual scale/distance references for the pilot
during manual orbit testing (Mode A). YOLO stays filtered to COCO class 11
("stop sign"), so the cubes never appear on /detections.

Assumes `sim_target` (the default ~1.0 m sign at x=5, y=0) is already spawned
via sim_target.launch.py. Run after the sim is up:

    ros2 launch dondron_bringup spawn_orientation_props.launch.py

Layout (local frame, meters, relative to drone spawn):

    sim_target        ~1.0 m sign   ( 5,  0) — spawned separately (existing)
    sim_target_small  ~0.5 m sign   ( 8,  0)
    sim_target_large  ~2.0 m sign   (12,  0)
    sim_target_extra_a ~1.0 m sign (22,  9) yaw ~73 deg
    sim_target_extra_b ~1.0 m sign (-4, 14) yaw ~195 deg
    sim_target_extra_c ~1.0 m sign (18,-16) yaw ~312 deg
    orientation_cube_small   0.3 m blue   ( 3,  3)
    orientation_cube_medium  0.6 m green  ( 3, -3)
    orientation_cube_large   1.2 m yellow ( 6,  6)
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def _model_path(name: str) -> str:
    return os.path.join(
        get_package_share_directory('dondron_bringup'), 'models', name, 'model.sdf')


# (entity name, model dir name, x, y, z, yaw)
_PROPS = [
    ('sim_target_small', 'sim_target_small', 8.0, 0.0, 0.0, 1.5708),
    ('sim_target_large', 'sim_target_large', 12.0, 0.0, 0.0, 1.5708),
    ('sim_target_extra_a', 'sim_target', 22.0, 9.0, 0.0, 1.2741),   # ~73 deg
    ('sim_target_extra_b', 'sim_target', -4.0, 14.0, 0.0, 3.4034),  # ~195 deg
    ('sim_target_extra_c', 'sim_target', 18.0, -16.0, 0.0, 5.4454),  # ~312 deg
    ('orientation_cube_small', 'orientation_cube_small', 3.0, 3.0, 0.15, 0.0),
    ('orientation_cube_medium', 'orientation_cube_medium', 3.0, -3.0, 0.3, 0.0),
    ('orientation_cube_large', 'orientation_cube_large', 6.0, 6.0, 0.6, 0.0),
]


def generate_launch_description():
    actions = []
    for entity_name, model_dir, x, y, z, yaw in _PROPS:
        actions.append(Node(
            package='ros_gz_sim',
            executable='create',
            name=f'spawn_{entity_name}',
            output='screen',
            arguments=[
                '-world', 'default',
                '-name', entity_name,
                '-file', _model_path(model_dir),
                '-x', str(x),
                '-y', str(y),
                '-z', str(z),
                '-Y', str(yaw),
            ],
        ))
    return LaunchDescription(actions)
