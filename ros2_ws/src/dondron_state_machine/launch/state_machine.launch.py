"""Launch dondron_state_machine BT node."""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'bt_xml_path',
            default_value='',
            description='Override BT XML path (default: package behavior_trees/mission.xml)',
        ),
        DeclareLaunchArgument(
            'tick_rate_hz',
            default_value='10.0',
        ),
        DeclareLaunchArgument(
            'vehicle_status_topic',
            default_value='/fmu/out/vehicle_status_v4',
            description='PX4 vehicle status topic (see flight_api.launch.py)',
        ),
        DeclareLaunchArgument(
            'vehicle_local_position_topic',
            default_value='/fmu/out/vehicle_local_position_v1',
            description='PX4 local position topic for closed-loop ClimbToAltitude',
        ),
        DeclareLaunchArgument(
            'search_pattern',
            default_value='weave',
            description='Search leg: weave (default) or orbit (circular closed-loop)',
        ),
        Node(
            package='dondron_state_machine',
            executable='state_machine_node',
            name='state_machine_node',
            output='screen',
            parameters=[{
                'bt_xml_path': LaunchConfiguration('bt_xml_path'),
                'tick_rate_hz': LaunchConfiguration('tick_rate_hz'),
                'vehicle_status_topic': LaunchConfiguration('vehicle_status_topic'),
                'vehicle_local_position_topic': LaunchConfiguration(
                    'vehicle_local_position_topic'),
                'search_pattern': LaunchConfiguration('search_pattern'),
            }],
        ),
    ])
