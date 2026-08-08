"""
Public Line 1 SIL stack — perception + flight_api + state_machine (through TRACK).

Does NOT include dondron_guidance_private or sil_full.

Prerequisites (see docs/sil-bridge.md):
  - Micro-XRCE agent + PX4 gz_x500 SITL running
  - Set ros_domain_id:=0 when sharing DDS domain with PX4 SITL

Camera: remaps Gazebo gz_x500 forward camera to /camera/image_raw (M1 contract).
M1 perception stub works without a live camera (timer-only synthetic /detections).
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    perception_share = get_package_share_directory('dondron_perception')
    flight_api_share = get_package_share_directory('dondron_flight_api')
    state_machine_share = get_package_share_directory('dondron_state_machine')

    gazebo_camera_topic = (
        '/world/default/model/x500_0/link/camera_link/sensor/imager/image'
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'ros_domain_id',
            default_value='42',
            description='ROS_DOMAIN_ID (use 0 with PX4 SITL on same host)',
        ),
        DeclareLaunchArgument(
            'image_topic',
            default_value=gazebo_camera_topic,
            description=(
                'Camera image source. Default assumes plain x500 with no separate ROS '
                'bridge. If a camera_bridge.launch.py (x500_mono_cam) is already running, '
                'override to /camera/image_raw instead.'
            ),
        ),
        DeclareLaunchArgument(
            'camera_info_topic',
            default_value='/camera/camera_info',
            description='CameraInfo for monocular range (use_yolo:=true / use_stub:=false).',
        ),
        DeclareLaunchArgument(
            'use_stub',
            default_value='true',
            description='Forwarded to perception.launch.py — false for real inference.',
        ),
        DeclareLaunchArgument(
            'use_yolo',
            default_value='false',
            description='Forwarded to perception.launch.py — true for YOLOv8n GPU inference.',
        ),
        DeclareLaunchArgument(
            'vehicle_status_topic',
            default_value='/fmu/out/vehicle_status_v4',
            description='PX4 bridged vehicle status (grep vehicle_status in ros2 topic list)',
        ),
        DeclareLaunchArgument(
            'bt_xml_path',
            default_value='',
            description=(
                'Forwarded to state_machine.launch.py — override BT XML path '
                '(default: package behavior_trees/mission.xml)'
            ),
        ),
        DeclareLaunchArgument(
            'search_pattern',
            default_value='weave',
            description='Forwarded to state_machine — weave (default) or orbit',
        ),
        SetEnvironmentVariable('ROS_DOMAIN_ID', LaunchConfiguration('ros_domain_id')),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(perception_share, 'launch', 'perception.launch.py')
            ),
            launch_arguments={
                'image_topic': LaunchConfiguration('image_topic'),
                'camera_info_topic': LaunchConfiguration('camera_info_topic'),
                'use_stub': LaunchConfiguration('use_stub'),
                'use_yolo': LaunchConfiguration('use_yolo'),
            }.items(),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(flight_api_share, 'launch', 'flight_api.launch.py')
            ),
            launch_arguments={
                'vehicle_status_topic': LaunchConfiguration('vehicle_status_topic'),
            }.items(),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(state_machine_share, 'launch', 'state_machine.launch.py')
            ),
            launch_arguments={
                'vehicle_status_topic': LaunchConfiguration('vehicle_status_topic'),
                'bt_xml_path': LaunchConfiguration('bt_xml_path'),
                'search_pattern': LaunchConfiguration('search_pattern'),
            }.items(),
        ),
    ])
