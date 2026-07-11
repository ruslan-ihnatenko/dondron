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
            parameters=[{'pattern': 'target'}],
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(perception_share, 'launch', 'perception.launch.py')
            ),
            launch_arguments={'use_stub': 'false'}.items(),
        ),
        ReadyToTest(),
    ]), {}


class TestPerceptionInferenceDetect(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        rclpy.init()
        cls._node = rclpy.create_node('perception_inference_detect_test')
        cls._detection_msg = None

        def on_detections(msg: Detection2DArray):
            if msg.detections and cls._detection_msg is None:
                cls._detection_msg = msg

        cls._sub = cls._node.create_subscription(
            Detection2DArray, '/detections', on_detections, 10)

    @classmethod
    def tearDownClass(cls):
        cls._node.destroy_node()
        rclpy.shutdown()

    def test_non_empty_detection_with_range(self, proc_output):
        deadline = time.time() + 5.0
        while time.time() < deadline and self._detection_msg is None:
            rclpy.spin_once(self._node, timeout_sec=0.1)

        self.assertIsNotNone(
            self._detection_msg,
            'Expected non-empty /detections from synthetic target image')

        msg = self._detection_msg
        self.assertEqual(msg.header.frame_id, 'camera_optical_frame')
        self.assertEqual(len(msg.detections), 1)

        det = msg.detections[0]
        self.assertGreater(det.bbox.size_x, 0.0)
        self.assertEqual(len(det.results), 1)
        self.assertEqual(det.results[0].hypothesis.class_id, '0')
        self.assertGreaterEqual(det.results[0].hypothesis.score, 0.5)
        self.assertGreater(det.results[0].pose.pose.position.z, 0.0)
