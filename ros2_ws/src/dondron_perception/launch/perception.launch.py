"""Launch dondron_perception stub node (M1 SIL integration)."""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'ros_domain_id',
            default_value='42',
            description='ROS_DOMAIN_ID (use 0 when co-located with PX4 SITL on same DDS domain)',
        ),
        DeclareLaunchArgument(
            'image_topic',
            default_value='/camera/image_raw',
            description=(
                'Camera image input. In Gazebo SIL, remap from the bridged Gazebo topic '
                '(see package README — gz_x500 default camera path).'
            ),
        ),
        DeclareLaunchArgument(
            'detections_topic',
            default_value='/detections',
            description='Detection2DArray output (Line 1 frozen contract).',
        ),
        DeclareLaunchArgument(
            'frame_id',
            default_value='camera_optical_frame',
            description='header.frame_id for /detections (matches dondron_description URDF).',
        ),
        Node(
            package='dondron_perception',
            executable='perception_node',
            name='perception_node',
            output='screen',
            parameters=[{
                'image_topic': LaunchConfiguration('image_topic'),
                'detections_topic': LaunchConfiguration('detections_topic'),
                'frame_id': LaunchConfiguration('frame_id'),
                'publish_rate_hz': 2.0,
            }],
        ),
    ])
