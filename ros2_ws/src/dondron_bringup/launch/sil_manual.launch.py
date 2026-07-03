"""
Manual SIL launch — Micro-XRCE agent only (no autonomy stack).

Prerequisites (external, see docs/sil-bridge.md):
  1. Micro-XRCE-DDS Agent (this launch can start it)
  2. PX4 SITL: source /opt/ros/jazzy/setup.bash && cd ~/PX4-Autopilot && make px4_sitl gz_x500

Use ros_domain_id:=0 when ROS nodes must share DDS with PX4 SITL on the same machine.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    bridge_share = get_package_share_directory('dondron_bridge')

    return LaunchDescription([
        DeclareLaunchArgument(
            'ros_domain_id',
            default_value='42',
            description='ROS_DOMAIN_ID for DonDron nodes (use 0 with PX4 SITL on same host)',
        ),
        DeclareLaunchArgument(
            'port',
            default_value='8888',
            description='uXRCE-DDS agent UDP port',
        ),
        DeclareLaunchArgument(
            'agent_executable',
            default_value=os.path.expanduser('~/Micro-XRCE-DDS-Agent/build/MicroXRCEAgent'),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(bridge_share, 'launch', 'agent.launch.py')
            ),
            launch_arguments={
                'ros_domain_id': LaunchConfiguration('ros_domain_id'),
                'port': LaunchConfiguration('port'),
                'agent_executable': LaunchConfiguration('agent_executable'),
            }.items(),
        ),
    ])
