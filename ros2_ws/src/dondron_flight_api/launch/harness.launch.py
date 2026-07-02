"""Bench harness — publishes test setpoints to /flight_api/cmd_setpoint."""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'cmd_setpoint_topic',
            default_value='/flight_api/cmd_setpoint',
        ),
        Node(
            package='dondron_flight_api',
            executable='harness_node',
            name='flight_api_harness',
            output='screen',
            parameters=[{
                'cmd_setpoint_topic': LaunchConfiguration('cmd_setpoint_topic'),
                'publish_rate_hz': 10.0,
                'forward_speed_mps': 0.5,
                'yaw_rate_radps': 0.15,
                'enable_offboard_after_s': 2.0,
                'cmd_frame': 'body_frd',
            }],
        ),
    ])
