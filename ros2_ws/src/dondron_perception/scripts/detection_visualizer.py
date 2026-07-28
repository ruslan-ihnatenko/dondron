#!/usr/bin/env python3
"""
Detection bounding-box overlay (debug/HUD visualization) — Line 1 Module A.

Subscribes /camera/image_raw + /detections and republishes an annotated
image with the recognized-object bounding box, class_id, and score drawn
on top — a simple operator HUD for visually confirming object recognition
during manual flight testing (Mode A) or autonomous TRACK (Mode B).

This is a debug/visualization aid only: it does not feed back into flight
control and carries no guidance/intercept logic (Module C, private repo).

Run standalone:

    ros2 run dondron_perception detection_visualizer.py

Then view with:

    ros2 run rqt_image_view rqt_image_view /detections/image_annotated

Deliberately avoids cv_bridge: the system ROS Python env has picked up a
numpy/opencv ABI mismatch (see yolo_inference_node.py header for the same
issue), which breaks cv_bridge's dtype lookups. Converts sensor_msgs/Image
to/from numpy directly instead — only rgb8/bgr8/mono8 input is supported.
"""

import cv2
import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Image
from vision_msgs.msg import Detection2DArray


def image_to_bgr_numpy(msg: Image):
    arr = np.frombuffer(msg.data, dtype=np.uint8)
    if msg.encoding in ('rgb8', 'bgr8'):
        arr = arr.reshape(msg.height, msg.width, 3)
        if msg.encoding == 'rgb8':
            arr = arr[:, :, ::-1]
        return np.ascontiguousarray(arr)
    if msg.encoding == 'mono8':
        arr = arr.reshape(msg.height, msg.width)
        return np.ascontiguousarray(np.stack([arr, arr, arr], axis=-1))
    raise ValueError(f'Unsupported image encoding: {msg.encoding}')


def bgr_numpy_to_image_msg(frame, header) -> Image:
    msg = Image()
    msg.header = header
    msg.height, msg.width = frame.shape[:2]
    msg.encoding = 'bgr8'
    msg.is_bigendian = 0
    msg.step = msg.width * 3
    msg.data = np.ascontiguousarray(frame).tobytes()
    return msg


class DetectionVisualizer(Node):

    def __init__(self):
        super().__init__('detection_visualizer')

        self.declare_parameter('image_topic', '/camera/image_raw')
        self.declare_parameter('detections_topic', '/detections')
        self.declare_parameter('output_topic', '/detections/image_annotated')
        self.declare_parameter('box_color_bgr', [0, 255, 0])
        self.declare_parameter('stale_detections_timeout_sec', 1.0)

        image_topic = self.get_parameter('image_topic').value
        detections_topic = self.get_parameter('detections_topic').value
        output_topic = self.get_parameter('output_topic').value
        self._box_color = tuple(int(c) for c in self.get_parameter('box_color_bgr').value)
        self._stale_timeout = float(self.get_parameter('stale_detections_timeout_sec').value)

        self._latest_detections = None
        self._latest_detections_stamp = None

        self._image_pub = self.create_publisher(Image, output_topic, 10)
        self.create_subscription(
            Detection2DArray, detections_topic, self._on_detections, qos_profile_sensor_data)
        self.create_subscription(
            Image, image_topic, self._on_image, qos_profile_sensor_data)

        self.get_logger().info(
            f'Overlaying "{detections_topic}" boxes on "{image_topic}" -> "{output_topic}"')

    def _on_detections(self, msg: Detection2DArray):
        self._latest_detections = msg
        self._latest_detections_stamp = self.get_clock().now()

    def _on_image(self, msg: Image):
        try:
            frame = image_to_bgr_numpy(msg)
        except ValueError as ex:
            self.get_logger().warn(f'{ex}', throttle_duration_sec=5.0)
            return

        detections_fresh = (
            self._latest_detections is not None
            and (self.get_clock().now() - self._latest_detections_stamp).nanoseconds
            < self._stale_timeout * 1e9
        )
        if detections_fresh:
            for det in self._latest_detections.detections:
                self._draw_detection(frame, det)

        self._image_pub.publish(bgr_numpy_to_image_msg(frame, msg.header))

    def _draw_detection(self, frame, det):
        cx = det.bbox.center.position.x
        cy = det.bbox.center.position.y
        half_w = det.bbox.size_x / 2.0
        half_h = det.bbox.size_y / 2.0
        pt1 = (int(cx - half_w), int(cy - half_h))
        pt2 = (int(cx + half_w), int(cy + half_h))
        cv2.rectangle(frame, pt1, pt2, self._box_color, 2)

        label = 'object'
        if det.results:
            best = max(det.results, key=lambda r: r.hypothesis.score)
            label = f'class {best.hypothesis.class_id} ({best.hypothesis.score:.2f})'
        text_origin = (pt1[0], max(0, pt1[1] - 8))
        cv2.putText(
            frame, label, text_origin, cv2.FONT_HERSHEY_SIMPLEX, 0.6,
            self._box_color, 2, cv2.LINE_AA)


def main(args=None):
    rclpy.init(args=args)
    node = DetectionVisualizer()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
