import os
import time
import unittest

from ament_index_python.packages import get_package_share_directory
import launch
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_testing.actions import ReadyToTest
import pytest
import rclpy
from vision_msgs.msg import Detection2DArray


@pytest.mark.launch_test
def generate_test_description():
    perception_share = get_package_share_directory('dondron_perception')
    return launch.LaunchDescription([
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(perception_share, 'launch', 'perception.launch.py')
            ),
        ),
        ReadyToTest(),
    ]), {}


class TestPerceptionLaunch(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        rclpy.init()
        cls._node = rclpy.create_node('perception_launch_test_listener')
        cls._received = False

        def on_detections(_msg: Detection2DArray):
            cls._received = True

        cls._sub = cls._node.create_subscription(
            Detection2DArray, '/detections', on_detections, 10)

    @classmethod
    def tearDownClass(cls):
        cls._node.destroy_node()
        rclpy.shutdown()

    def test_detections_publishes_within_timeout(self, proc_output):
        deadline = time.time() + 5.0
        while time.time() < deadline and not self._received:
            rclpy.spin_once(self._node, timeout_sec=0.1)
        self.assertTrue(
            self._received,
            'Expected /detections message from perception_node within 5s')
