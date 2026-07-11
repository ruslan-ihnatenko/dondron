#!/usr/bin/env python3
"""Synthetic camera publisher for perception inference launch tests."""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CameraInfo, Image


class SyntheticCameraPublisher(Node):
    """Publishes paired Image + CameraInfo for contract tests (no live camera)."""

    def __init__(self):
        super().__init__('synthetic_camera_publisher')
        self.declare_parameter('pattern', 'target')
        self.declare_parameter('width', 640)
        self.declare_parameter('height', 480)
        self.declare_parameter('frame_id', 'camera_optical_frame')
        self.declare_parameter('publish_hz', 10.0)
        self.declare_parameter('focal_length_px', 554.25)

        self._pattern = self.get_parameter('pattern').get_parameter_value().string_value
        self._width = self.get_parameter('width').get_parameter_value().integer_value
        self._height = self.get_parameter('height').get_parameter_value().integer_value
        self._frame_id = self.get_parameter('frame_id').get_parameter_value().string_value
        hz = self.get_parameter('publish_hz').get_parameter_value().double_value
        self._focal = self.get_parameter('focal_length_px').get_parameter_value().double_value

        self._image_pub = self.create_publisher(Image, '/camera/image_raw', 10)
        self._info_pub = self.create_publisher(CameraInfo, '/camera/camera_info', 10)
        self._timer = self.create_timer(1.0 / hz, self._publish)

    def _publish(self):
        stamp = self.get_clock().now().to_msg()
        image = Image()
        image.header.stamp = stamp
        image.header.frame_id = self._frame_id
        image.height = self._height
        image.width = self._width
        image.encoding = 'mono8'
        image.is_bigendian = 0
        image.step = self._width
        pixels = bytearray(self._width * self._height)

        if self._pattern == 'target':
            # Bright rectangle centered — detected by blob placeholder on Mac/CI.
            cx, cy = self._width // 2, self._height // 2
            half_w, half_h = 40, 30
            for y in range(self._height):
                for x in range(self._width):
                    if (abs(x - cx) <= half_w and abs(y - cy) <= half_h):
                        pixels[y * self._width + x] = 255
        # else: empty (all zeros) → inference publishes detections: []

        image.data = list(pixels)

        info = CameraInfo()
        info.header = image.header
        info.height = self._height
        info.width = self._width
        info.distortion_model = 'plumb_bob'
        info.d = [0.0, 0.0, 0.0, 0.0, 0.0]
        info.k = [
            self._focal, 0.0, self._width / 2.0,
            0.0, self._focal, self._height / 2.0,
            0.0, 0.0, 1.0,
        ]
        info.r = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
        info.p = [
            self._focal, 0.0, self._width / 2.0, 0.0,
            0.0, self._focal, self._height / 2.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
        ]

        self._image_pub.publish(image)
        self._info_pub.publish(info)


def main(args=None):
    rclpy.init(args=args)
    node = SyntheticCameraPublisher()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
