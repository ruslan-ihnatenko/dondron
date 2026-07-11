import os
import time
import unittest

from ament_index_python.packages import get_package_share_directory
import launch
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_testing.actions import ReadyToTest
import pytest
import rclpy
from vision_msgs.msg import Detection2DArray


@pytest.mark.launch_test
def generate_test_description():
    perception_share = get_package_share_directory('dondron_perception')
    return launch.LaunchDescription([
        Node(
            package='dondron_perception',
            executable='synthetic_camera_publisher.py',
            name='synthetic_camera_publisher',
            output='screen',
            parameters=[{'pattern': 'empty'}],
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(perception_share, 'launch', 'perception.launch.py')
            ),
            launch_arguments={'use_stub': 'false'}.items(),
        ),
        ReadyToTest(),
    ]), {}


class TestPerceptionInferenceEmpty(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        rclpy.init()
        cls._node = rclpy.create_node('perception_inference_empty_test')
        cls._messages = []

        def on_detections(msg: Detection2DArray):
            cls._messages.append(msg)

        cls._sub = cls._node.create_subscription(
            Detection2DArray, '/detections', on_detections, 10)

    @classmethod
    def tearDownClass(cls):
        cls._node.destroy_node()
        rclpy.shutdown()

    def test_empty_detections_and_frame_id(self, proc_output):
        deadline = time.time() + 5.0
        while time.time() < deadline and len(self._messages) < 3:
            rclpy.spin_once(self._node, timeout_sec=0.1)

        self.assertGreaterEqual(
            len(self._messages), 1,
            'Expected /detections from inference mode within 5s')

        for msg in self._messages:
            self.assertEqual(msg.header.frame_id, 'camera_optical_frame')
            self.assertEqual(len(msg.detections), 0)
