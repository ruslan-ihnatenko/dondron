"""Launch dondron_flight_api node."""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'ros_domain_id',
            default_value='42',
            description='ROS_DOMAIN_ID (use 0 when co-located with PX4 SITL)',
        ),
        DeclareLaunchArgument(
            'cmd_setpoint_topic',
            default_value='/flight_api/cmd_setpoint',
        ),
        DeclareLaunchArgument(
            'status_topic',
            default_value='/flight_api/status',
        ),
        DeclareLaunchArgument(
            'vehicle_status_topic',
            default_value='/fmu/out/vehicle_status_v4',
            description=(
                'PX4 vehicle status bridged topic. Use ros2 topic list | grep vehicle_status '
                'when px4_msgs and PX4 firmware versions differ (_v4 suffix).'
            ),
        ),
        Node(
            package='dondron_flight_api',
            executable='flight_api_node',
            name='flight_api_node',
            output='screen',
            parameters=[{
                'cmd_setpoint_topic': LaunchConfiguration('cmd_setpoint_topic'),
                'status_topic': LaunchConfiguration('status_topic'),
                'default_cmd_frame': 'body_frd',
                'setpoint_rate_hz': 20.0,
                'cmd_timeout_s': 0.5,
                'auto_request_offboard': True,
                'vehicle_status_topic': LaunchConfiguration('vehicle_status_topic'),
            }],
        ),
    ])
