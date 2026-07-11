"""Launch dondron_perception node (M1 stub or M3 inference contract)."""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'ros_domain_id',
            default_value='42',
            description='ROS_DOMAIN_ID (use 0 when co-located with PX4 SITL on same DDS domain)',
        ),
        DeclareLaunchArgument(
            'use_stub',
            default_value='true',
            description='M1 timer stub (true) or M3 image-callback inference (false).',
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
            'camera_info_topic',
            default_value='/camera/camera_info',
            description='CameraInfo for monocular range (M3+).',
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
        DeclareLaunchArgument(
            'publish_rate_hz',
            default_value='2.0',
            description='Stub publish rate (use_stub:=true only).',
        ),
        DeclareLaunchArgument(
            'score_threshold',
            default_value='0.5',
            description='Minimum detection score (use_stub:=false).',
        ),
        DeclareLaunchArgument(
            'target_width_m',
            default_value='0.30',
            description='Known physical width of class-0 target for monocular range.',
        ),
        DeclareLaunchArgument(
            'target_class_id',
            default_value='0',
            description='class_id string for inference detections.',
        ),
        DeclareLaunchArgument(
            'brightness_threshold',
            default_value='200',
            description='CPU blob placeholder threshold 0-255 (Mac/CI; YOLO on Main PC).',
        ),
        Node(
            package='dondron_perception',
            executable='perception_node',
            name='perception_node',
            output='screen',
            parameters=[{
                'use_stub': ParameterValue(LaunchConfiguration('use_stub'), value_type=bool),
                'image_topic': LaunchConfiguration('image_topic'),
                'camera_info_topic': LaunchConfiguration('camera_info_topic'),
                'detections_topic': LaunchConfiguration('detections_topic'),
                'frame_id': LaunchConfiguration('frame_id'),
                'publish_rate_hz': ParameterValue(
                    LaunchConfiguration('publish_rate_hz'), value_type=float),
                'score_threshold': ParameterValue(
                    LaunchConfiguration('score_threshold'), value_type=float),
                'target_width_m': ParameterValue(
                    LaunchConfiguration('target_width_m'), value_type=float),
                'target_class_id': ParameterValue(
                    LaunchConfiguration('target_class_id'), value_type=str),
                'brightness_threshold': ParameterValue(
                    LaunchConfiguration('brightness_threshold'), value_type=int),
            }],
        ),
    ])
