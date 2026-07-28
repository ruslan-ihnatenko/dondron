"""Launch dondron_perception node (M1 stub, M3 blob inference, or M3 YOLO GPU)."""

import os

from ament_index_python.packages import get_package_prefix
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    yolo_script = os.path.join(
        get_package_prefix('dondron_perception'), 'lib', 'dondron_perception',
        'yolo_inference_node.py',
    )

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
            'use_yolo',
            default_value='false',
            description=(
                'Main PC only: use real YOLOv8n GPU inference (scripts/yolo_inference_node.py) '
                'instead of the C++ perception_node. Overrides use_stub when true. '
                'Requires the venv in yolo_python (see script header docstring).'
            ),
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
            description='class_id string for inference detections (frozen contract output id).',
        ),
        DeclareLaunchArgument(
            'brightness_threshold',
            default_value='200',
            description='CPU blob placeholder threshold 0-255 (Mac/CI; YOLO on Main PC).',
        ),
        DeclareLaunchArgument(
            'model_path',
            default_value='yolov8n.pt',
            description='Ultralytics model path/name (use_yolo:=true only).',
        ),
        DeclareLaunchArgument(
            'yolo_source_class_id',
            default_value='11',
            description=(
                'COCO class id the pretrained model must match (use_yolo:=true only). '
                'Default 11 = "stop sign", matching dondron_bringup models/sim_target.'
            ),
        ),
        DeclareLaunchArgument(
            'yolo_python',
            default_value=os.path.expanduser('~/dondron_yolo_venv/bin/python3'),
            description=(
                'Python interpreter with ultralytics installed (use_yolo:=true only). '
                'See scripts/yolo_inference_node.py header for venv setup.'
            ),
        ),
        DeclareLaunchArgument(
            'device',
            default_value='auto',
            description='torch device override, e.g. cuda:0 or cpu ("auto" = auto-detect).',
        ),
        Node(
            package='dondron_perception',
            executable='perception_node',
            name='perception_node',
            output='screen',
            condition=UnlessCondition(LaunchConfiguration('use_yolo')),
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
        ExecuteProcess(
            condition=IfCondition(LaunchConfiguration('use_yolo')),
            name='yolo_inference_node',
            output='screen',
            cmd=[
                LaunchConfiguration('yolo_python'),
                yolo_script,
                '--ros-args',
                '-p', ['image_topic:=', LaunchConfiguration('image_topic')],
                '-p', ['camera_info_topic:=', LaunchConfiguration('camera_info_topic')],
                '-p', ['detections_topic:=', LaunchConfiguration('detections_topic')],
                '-p', ['frame_id:=', LaunchConfiguration('frame_id')],
                '-p', ['model_path:=', LaunchConfiguration('model_path')],
                '-p', ['score_threshold:=', LaunchConfiguration('score_threshold')],
                '-p', ['yolo_source_class_id:=', LaunchConfiguration('yolo_source_class_id')],
                # Quoted so ROS's YAML parameter parser keeps this a string even
                # when the value looks numeric (e.g. "0").
                '-p', ['target_class_id:="', LaunchConfiguration('target_class_id'), '"'],
                '-p', ['target_width_m:=', LaunchConfiguration('target_width_m')],
                '-p', ['device:=', LaunchConfiguration('device')],
            ],
        ),
    ])
