"""
Gazebo -> ROS 2 camera bridge (Main PC SIL only).

Bridges the gz-transport image + camera_info topics published by the
`x500_mono_cam` PX4 Gazebo model onto the frozen dondron_perception
contract topics (`/camera/image_raw`, `/camera/camera_info`).

Requires the `gz_x500_mono_cam` PX4 target (plain `gz_x500` has no camera):

  make px4_sitl gz_x500_mono_cam

Verify gz-side topic names while the sim is running (world/model name
may differ from the default single-vehicle instance):

  gz topic -l | grep -i camera
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    default_gz_image_topic = (
        '/world/default/model/x500_mono_cam_0/link/camera_link/sensor/camera/image'
    )
    default_gz_camera_info_topic = (
        '/world/default/model/x500_mono_cam_0/link/camera_link/sensor/camera/camera_info'
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'gz_image_topic',
            default_value=default_gz_image_topic,
            description=(
                'Source gz-transport image topic. Override if world/model name '
                'differs from x500_mono_cam_0 (check with `gz topic -l`).'
            ),
        ),
        DeclareLaunchArgument(
            'gz_camera_info_topic',
            default_value=default_gz_camera_info_topic,
            description='Source gz-transport camera_info topic (paired with gz_image_topic).',
        ),
        DeclareLaunchArgument(
            'image_topic',
            default_value='/camera/image_raw',
            description='Destination ROS topic (dondron_perception contract).',
        ),
        DeclareLaunchArgument(
            'camera_info_topic',
            default_value='/camera/camera_info',
            description='Destination ROS topic (dondron_perception contract).',
        ),
        Node(
            package='ros_gz_bridge',
            executable='parameter_bridge',
            name='camera_gz_bridge',
            output='screen',
            arguments=[
                [
                    LaunchConfiguration('gz_image_topic'),
                    '@sensor_msgs/msg/Image[gz.msgs.Image',
                ],
                [
                    LaunchConfiguration('gz_camera_info_topic'),
                    '@sensor_msgs/msg/CameraInfo[gz.msgs.CameraInfo',
                ],
            ],
            remappings=[
                (LaunchConfiguration('gz_image_topic'), LaunchConfiguration('image_topic')),
                (
                    LaunchConfiguration('gz_camera_info_topic'),
                    LaunchConfiguration('camera_info_topic'),
                ),
            ],
        ),
    ])
