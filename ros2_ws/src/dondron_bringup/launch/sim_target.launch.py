"""
Spawn a simple recognizable sim target into the running Gazebo world (Main PC SIL).

Spawns `models/sim_target/model.sdf` (this package) — a wrapper around the
Fuel "Stop Sign" mesh (OpenRobotics) that:

  - Matches a stock COCO class (`stop sign`, class id 11) so pretrained
    YOLOv8n can detect it without custom training (M3 first pass; a
    custom "target" class dataset is deferred to M4).
  - Overrides the mesh's legacy Ogre material (unsupported / renders flat
    black under gz-sim8 + Ogre2) with a plain SDF red material.
  - Scales the mesh 3.2x so its real width matches the dondron_perception
    `target_width_m` default (0.30 m) — the upstream mesh measures only
    ~0.095 m as authored (empirically calibrated, not trustworthy from the
    raw COLLADA transform alone).

Requires network access once to populate the local Fuel cache
(`~/.gz/fuel/fuel.gazebosim.org/...`) the first time this mesh URI is
resolved; offline afterwards.

Default pose: 5 m in front of the PX4 SITL home position (+X), sign face
rotated to point back at the vehicle (yaw = pi/2 — the mesh's face normal
is along its local Y axis, not X), sitting on the ground (z = 0).

Detection envelope measured with YOLOv8n pretrained weights at this scale
(camera_frame.png smoke tests, HFOV ~99.7 deg): confident "stop sign" hits
(score 0.75-0.86) at 3-5 m near boresight; score drops toward the 0.5
contract threshold at ~5 m + large off-axis angle; undetected by ~8 m.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'world', default_value='default',
            description='Gazebo world name to spawn into.',
        ),
        DeclareLaunchArgument(
            'name', default_value='sim_target',
            description='Entity name for the spawned target.',
        ),
        DeclareLaunchArgument(
            'model_file',
            default_value=os.path.join(
                get_package_share_directory('dondron_bringup'),
                'models', 'sim_target', 'model.sdf',
            ),
            description='Local SDF file to spawn (see package models/sim_target/).',
        ),
        DeclareLaunchArgument('x', default_value='5.0'),
        DeclareLaunchArgument('y', default_value='0.0'),
        DeclareLaunchArgument('z', default_value='0.0'),
        DeclareLaunchArgument(
            'yaw', default_value='1.5708',
            description=(
                'Radians. Default (pi/2) points the sign face back toward the '
                'SITL home pose — the mesh face normal is local +Y, not +X.'
            ),
        ),
        Node(
            package='ros_gz_sim',
            executable='create',
            name='spawn_sim_target',
            output='screen',
            arguments=[
                '-world', LaunchConfiguration('world'),
                '-name', LaunchConfiguration('name'),
                '-file', LaunchConfiguration('model_file'),
                '-x', LaunchConfiguration('x'),
                '-y', LaunchConfiguration('y'),
                '-z', LaunchConfiguration('z'),
                '-Y', LaunchConfiguration('yaw'),
            ],
        ),
    ])
