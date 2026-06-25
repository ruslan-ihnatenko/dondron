"""Launch Micro-XRCE-DDS Agent for PX4 ↔ ROS 2 bridging."""

import os

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, SetEnvironmentVariable
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    default_agent = os.path.expanduser(
        '~/Micro-XRCE-DDS-Agent/build/MicroXRCEAgent'
    )

    agent_executable = LaunchConfiguration('agent_executable')
    port = LaunchConfiguration('port')
    ros_domain_id = LaunchConfiguration('ros_domain_id')

    return LaunchDescription([
        DeclareLaunchArgument(
            'agent_executable',
            default_value=default_agent,
            description='Path to MicroXRCEAgent binary',
        ),
        DeclareLaunchArgument(
            'port',
            default_value='8888',
            description='UDP port (must match PX4 uxrce_dds_client)',
        ),
        DeclareLaunchArgument(
            'ros_domain_id',
            default_value='42',
            description='ROS_DOMAIN_ID for all DonDron nodes on this machine',
        ),
        SetEnvironmentVariable('ROS_DOMAIN_ID', ros_domain_id),
        ExecuteProcess(
            cmd=[agent_executable, 'udp4', '-p', port],
            output='screen',
            shell=False,
        ),
    ])
