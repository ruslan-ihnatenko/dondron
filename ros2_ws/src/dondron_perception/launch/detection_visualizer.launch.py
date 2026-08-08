"""
Launch the /detections bounding-box overlay (debug HUD) — Line 1 Module A.

Standalone debug aid: overlays /detections boxes on /camera/image_raw and
republishes on /detections/image_annotated. View with rqt_image_view or
show_window:=true for a local OpenCV window.
Not part of the flight-critical path; safe to run alongside Mode A or Mode B.
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'image_topic', default_value='/camera/image_raw',
            description='Source camera image topic.',
        ),
        DeclareLaunchArgument(
            'detections_topic', default_value='/detections',
            description='Source Detection2DArray topic.',
        ),
        DeclareLaunchArgument(
            'output_topic', default_value='/detections/image_annotated',
            description='Annotated image output topic (view with rqt_image_view).',
        ),
        DeclareLaunchArgument(
            'show_window', default_value='false',
            description='Open local OpenCV imshow window (needs DISPLAY).',
        ),
        Node(
            package='dondron_perception',
            executable='detection_visualizer.py',
            name='detection_visualizer',
            output='screen',
            parameters=[{
                'image_topic': LaunchConfiguration('image_topic'),
                'detections_topic': LaunchConfiguration('detections_topic'),
                'output_topic': LaunchConfiguration('output_topic'),
                'show_window': LaunchConfiguration('show_window'),
            }],
        ),
    ])
