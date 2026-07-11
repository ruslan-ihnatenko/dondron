#!/usr/bin/env python3
"""
Optional YOLOv8 inference node (Main PC GPU path).

Not started by default — enable via launch arg `use_yolo:=true` when
ultralytics + torch are installed. CPU-only on Mac Dev Container is possible
but slow; keep `use_stub:=true` for CI.

Usage (Main PC, after pip install ultralytics):
  ros2 run dondron_perception yolo_inference_node.py

Publishes /detections per frozen contract; subscribes /camera/image_raw.
"""

import rclpy
from rclpy.node import Node


class YoloInferenceNode(Node):
    """Scaffold — wire Ultralytics YOLOv8n in a follow-up Main PC session."""

    def __init__(self):
        super().__init__('yolo_inference_node')
        self.declare_parameter('model_path', 'yolov8n.pt')
        self.declare_parameter('score_threshold', 0.5)
        self.declare_parameter('target_class_id', '0')
        self.declare_parameter('target_width_m', 0.30)
        self.get_logger().warn(
            'yolo_inference_node is a scaffold only — install ultralytics on Main PC '
            'and implement inference callback before production use.')

    def run(self):
        rclpy.spin(self)


def main(args=None):
    rclpy.init(args=args)
    node = YoloInferenceNode()
    try:
        node.run()
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
