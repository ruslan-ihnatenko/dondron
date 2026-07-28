#!/usr/bin/env python3
"""
YOLOv8 inference node (Main PC GPU path) — M3.

Alternative to the C++ `perception_node` blob placeholder when `use_yolo:=true`
(see `dondron_perception/launch/perception.launch.py`). Publishes /detections
per the frozen contract; subscribes /camera/image_raw + /camera/camera_info.

Requires `ultralytics` + `torch` (Main PC only — not installed in CI / the Mac
Dev Container). To avoid numpy/opencv ABI conflicts with the system ROS
Python environment, install into an isolated virtualenv with access to the
ROS site-packages:

    python3 -m venv --system-site-packages ~/dondron_yolo_venv
    ~/dondron_yolo_venv/bin/pip install ultralytics

`dondron_bringup` launches this script with that venv's interpreter when
`use_yolo:=true` (see camera_bridge / perception.launch.py `yolo_python` arg).
Manual run:

    ~/dondron_yolo_venv/bin/python3 \
      $(ros2 pkg prefix dondron_perception)/lib/dondron_perception/yolo_inference_node.py \
      --ros-args -p use_yolo:=true

Deliberately avoids cv_bridge: converts sensor_msgs/Image to a numpy array
directly, so this node's numpy/opencv versions (inside the venv) never have
to match the system ROS install's ABI.
"""

import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import CameraInfo, Image
from vision_msgs.msg import Detection2D, Detection2DArray, ObjectHypothesisWithPose


def image_to_numpy(msg: Image):
    """Convert sensor_msgs/Image to an HxWx3 BGR uint8 array (no cv_bridge)."""
    arr = np.frombuffer(msg.data, dtype=np.uint8)
    if msg.encoding in ('rgb8', 'bgr8'):
        arr = arr.reshape(msg.height, msg.width, 3)
        if msg.encoding == 'rgb8':
            arr = arr[:, :, ::-1]  # RGB -> BGR (ultralytics/opencv convention)
        return np.ascontiguousarray(arr)
    if msg.encoding == 'mono8':
        arr = arr.reshape(msg.height, msg.width)
        return np.ascontiguousarray(np.stack([arr, arr, arr], axis=-1))
    raise ValueError(f'Unsupported image encoding: {msg.encoding}')


def monocular_range_m(
        focal_length_px: float, target_width_m: float, bbox_width_px: float) -> float:
    if bbox_width_px <= 0.0 or focal_length_px <= 0.0 or target_width_m <= 0.0:
        return 0.0
    return (focal_length_px * target_width_m) / bbox_width_px


class YoloInferenceNode(Node):

    def __init__(self):
        super().__init__('yolo_inference_node')

        self.declare_parameter('image_topic', '/camera/image_raw')
        self.declare_parameter('camera_info_topic', '/camera/camera_info')
        self.declare_parameter('detections_topic', '/detections')
        self.declare_parameter('frame_id', 'camera_optical_frame')
        self.declare_parameter('model_path', 'yolov8n.pt')
        self.declare_parameter('score_threshold', 0.5)
        self.declare_parameter(
            'yolo_source_class_id', 11,
            # COCO index detected by the pretrained model. Default 11 = "stop
            # sign" — matches the M3 SIL sim target (see dondron_bringup
            # models/sim_target/). Swap when spawning a different COCO prop.
        )
        self.declare_parameter('target_class_id', '0')  # frozen contract output id
        self.declare_parameter('target_width_m', 0.30)
        self.declare_parameter('default_focal_length_px', 554.25)
        self.declare_parameter('device', 'auto')  # 'auto' = cuda:0 if available, else cpu

        self._image_topic = self.get_parameter('image_topic').value
        self._camera_info_topic = self.get_parameter('camera_info_topic').value
        self._detections_topic = self.get_parameter('detections_topic').value
        self._frame_id = self.get_parameter('frame_id').value
        model_path = self.get_parameter('model_path').value
        self._score_threshold = float(self.get_parameter('score_threshold').value)
        self._yolo_source_class_id = int(self.get_parameter('yolo_source_class_id').value)
        self._target_class_id = str(self.get_parameter('target_class_id').value)
        self._target_width_m = float(self.get_parameter('target_width_m').value)
        self._default_focal_length_px = float(
            self.get_parameter('default_focal_length_px').value)

        self._focal_length_px = 0.0
        self._has_camera_info = False

        device_param = self.get_parameter('device').value
        self._device = self._auto_device() if device_param == 'auto' else device_param

        self.get_logger().info(
            f'Loading YOLO model "{model_path}" on device "{self._device}"...')
        from ultralytics import YOLO  # deferred: heavy import, Main PC only
        self._model = YOLO(model_path)
        source_name = self._model.names.get(self._yolo_source_class_id, '?')
        self.get_logger().info(
            f'Model loaded. Filtering COCO class {self._yolo_source_class_id} '
            f'("{source_name}") -> contract class_id "{self._target_class_id}", '
            f'score_threshold={self._score_threshold}, target_width_m={self._target_width_m}')

        self._detections_pub = self.create_publisher(
            Detection2DArray, self._detections_topic, 10)
        self._camera_info_sub = self.create_subscription(
            CameraInfo, self._camera_info_topic, self._on_camera_info,
            qos_profile_sensor_data)
        self._image_sub = self.create_subscription(
            Image, self._image_topic, self._on_image, qos_profile_sensor_data)

    @staticmethod
    def _auto_device() -> str:
        try:
            import torch
            return 'cuda:0' if torch.cuda.is_available() else 'cpu'
        except ImportError:
            return 'cpu'

    def _on_camera_info(self, msg: CameraInfo):
        if msg.k[0] > 0.0:
            self._focal_length_px = msg.k[0]
            self._has_camera_info = True

    def _on_image(self, msg: Image):
        out = Detection2DArray()
        out.header.stamp = msg.header.stamp
        out.header.frame_id = self._frame_id

        try:
            frame = image_to_numpy(msg)
        except ValueError as ex:
            self.get_logger().warn(f'{ex}', throttle_duration_sec=5.0)
            self._detections_pub.publish(out)
            return

        focal_px = self._focal_length_px if self._has_camera_info else (
            self._default_focal_length_px)

        results = self._model.predict(
            frame, device=self._device, verbose=False, conf=0.05,
            classes=[self._yolo_source_class_id])
        boxes = results[0].boxes

        # Publish every box above threshold, not just the highest-confidence
        # one — multiple targets (e.g. several signs) can be in frame at once.
        for box in boxes:
            conf = float(box.conf[0])
            if conf < self._score_threshold:
                continue
            cx, cy, w, h = box.xywh[0].tolist()

            det = Detection2D()
            det.bbox.center.position.x = cx
            det.bbox.center.position.y = cy
            det.bbox.size_x = w
            det.bbox.size_y = h

            result = ObjectHypothesisWithPose()
            result.hypothesis.class_id = self._target_class_id
            result.hypothesis.score = conf
            result.pose.pose.position.z = monocular_range_m(
                focal_px, self._target_width_m, w)

            det.results.append(result)
            out.detections.append(det)

        self._detections_pub.publish(out)


def main(args=None):
    rclpy.init(args=args)
    node = YoloInferenceNode()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
