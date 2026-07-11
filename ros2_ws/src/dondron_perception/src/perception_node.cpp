#include <memory>
#include <mutex>
#include <string>

#include <cv_bridge/cv_bridge.hpp>
#include "dondron_perception/monocular_range.hpp"
#include "dondron_perception/simple_blob_detector.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "vision_msgs/msg/detection2_d.hpp"
#include "vision_msgs/msg/detection2_d_array.hpp"
#include "vision_msgs/msg/object_hypothesis_with_pose.hpp"

namespace dondron_perception
{

class PerceptionNode : public rclcpp::Node
{
public:
  PerceptionNode()
  : Node("perception_node")
  {
    use_stub_ = declare_parameter<bool>("use_stub", true);
    frame_id_ = declare_parameter<std::string>("frame_id", "camera_optical_frame");
    publish_rate_hz_ = declare_parameter<double>("publish_rate_hz", 2.0);
    stub_class_id_ = declare_parameter<std::string>("stub_class_id", "0");
    stub_score_ = declare_parameter<double>("stub_score", 0.95);
    stub_range_m_ = declare_parameter<double>("stub_range_m", 10.0);
    stub_bbox_center_x_ = declare_parameter<double>("stub_bbox_center_x", 320.0);
    stub_bbox_center_y_ = declare_parameter<double>("stub_bbox_center_y", 240.0);
    stub_bbox_size_x_ = declare_parameter<double>("stub_bbox_size_x", 80.0);
    stub_bbox_size_y_ = declare_parameter<double>("stub_bbox_size_y", 60.0);
    image_topic_ = declare_parameter<std::string>("image_topic", "/camera/image_raw");
    camera_info_topic_ = declare_parameter<std::string>(
      "camera_info_topic", "/camera/camera_info");
    detections_topic_ = declare_parameter<std::string>("detections_topic", "/detections");
    score_threshold_ = declare_parameter<double>("score_threshold", 0.5);
    target_width_m_ = declare_parameter<double>("target_width_m", 0.30);
    target_class_id_ = declare_parameter<std::string>("target_class_id", "0");
    brightness_threshold_ = declare_parameter<int>("brightness_threshold", 200);
    default_focal_length_px_ = declare_parameter<double>("default_focal_length_px", 554.25);

    image_sub_ = create_subscription<sensor_msgs::msg::Image>(
      image_topic_, rclcpp::SensorDataQoS(),
      std::bind(&PerceptionNode::on_image, this, std::placeholders::_1));

    camera_info_sub_ = create_subscription<sensor_msgs::msg::CameraInfo>(
      camera_info_topic_, rclcpp::SensorDataQoS(),
      [this](const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(camera_info_mutex_);
        focal_length_px_ = msg->k[0];
        has_camera_info_ = focal_length_px_ > 0.0;
      });

    detections_pub_ = create_publisher<vision_msgs::msg::Detection2DArray>(
      detections_topic_, rclcpp::QoS(10));

    if (use_stub_) {
      const auto period_ms = static_cast<int64_t>(1000.0 / publish_rate_hz_);
      timer_ = create_wall_timer(
        std::chrono::milliseconds(period_ms),
        std::bind(&PerceptionNode::publish_stub_detections, this));
      RCLCPP_INFO(
        get_logger(),
        "Perception stub: %s -> %s at %.1f Hz (frame_id=%s)",
        image_topic_.c_str(), detections_topic_.c_str(), publish_rate_hz_, frame_id_.c_str());
    } else {
      RCLCPP_INFO(
        get_logger(),
        "Perception inference: %s + %s -> %s (frame_id=%s, score_threshold=%.2f)",
        image_topic_.c_str(), camera_info_topic_.c_str(), detections_topic_.c_str(),
        frame_id_.c_str(), score_threshold_);
    }
  }

private:
  void on_image(const sensor_msgs::msg::Image::SharedPtr msg)
  {
    {
      std::lock_guard<std::mutex> lock(image_mutex_);
      latest_image_stamp_ = msg->header.stamp;
      has_image_ = true;
    }

    if (use_stub_) {
      return;
    }

    publish_inference_detections(msg);
  }

  void publish_stub_detections()
  {
    vision_msgs::msg::Detection2DArray msg;
    {
      std::lock_guard<std::mutex> lock(image_mutex_);
      msg.header.stamp = has_image_ ? latest_image_stamp_ : now();
    }
    msg.header.frame_id = frame_id_;

    vision_msgs::msg::Detection2D det;
    det.bbox.center.position.x = stub_bbox_center_x_;
    det.bbox.center.position.y = stub_bbox_center_y_;
    det.bbox.size_x = stub_bbox_size_x_;
    det.bbox.size_y = stub_bbox_size_y_;

    vision_msgs::msg::ObjectHypothesisWithPose result;
    result.hypothesis.class_id = stub_class_id_;
    result.hypothesis.score = stub_score_;
    result.pose.pose.position.z = stub_range_m_;

    det.results.push_back(result);
    msg.detections.push_back(det);

    detections_pub_->publish(msg);
  }

  void publish_inference_detections(const sensor_msgs::msg::Image::SharedPtr & image_msg)
  {
    vision_msgs::msg::Detection2DArray msg;
    msg.header.stamp = image_msg->header.stamp;
    msg.header.frame_id = frame_id_;

    double focal_px = default_focal_length_px_;
    {
      std::lock_guard<std::mutex> lock(camera_info_mutex_);
      if (has_camera_info_) {
        focal_px = focal_length_px_;
      }
    }

    try {
      const auto cv_ptr = cv_bridge::toCvShare(image_msg, "mono8");
      BlobDetection blob;
      if (detect_largest_bright_blob(cv_ptr->image, brightness_threshold_, blob) &&
        blob.score >= score_threshold_)
      {
        vision_msgs::msg::Detection2D det;
        det.bbox.center.position.x = blob.center_x;
        det.bbox.center.position.y = blob.center_y;
        det.bbox.size_x = blob.size_x;
        det.bbox.size_y = blob.size_y;

        vision_msgs::msg::ObjectHypothesisWithPose result;
        result.hypothesis.class_id = target_class_id_;
        result.hypothesis.score = blob.score;
        result.pose.pose.position.z = monocular_range_m(
          focal_px, target_width_m_, blob.size_x);

        det.results.push_back(result);
        msg.detections.push_back(det);
      }
    } catch (const cv_bridge::Exception & ex) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 5000,
        "cv_bridge conversion failed: %s", ex.what());
    }

    detections_pub_->publish(msg);
  }

  bool use_stub_{true};
  std::string frame_id_;
  std::string image_topic_;
  std::string camera_info_topic_;
  std::string detections_topic_;
  std::string stub_class_id_;
  std::string target_class_id_;
  double publish_rate_hz_{2.0};
  double stub_score_{0.95};
  double stub_range_m_{10.0};
  double stub_bbox_center_x_{320.0};
  double stub_bbox_center_y_{240.0};
  double stub_bbox_size_x_{80.0};
  double stub_bbox_size_y_{60.0};
  double score_threshold_{0.5};
  double target_width_m_{0.30};
  double default_focal_length_px_{554.25};
  int brightness_threshold_{200};

  std::mutex image_mutex_;
  std::mutex camera_info_mutex_;
  rclcpp::Time latest_image_stamp_;
  bool has_image_{false};
  double focal_length_px_{0.0};
  bool has_camera_info_{false};

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr camera_info_sub_;
  rclcpp::Publisher<vision_msgs::msg::Detection2DArray>::SharedPtr detections_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace dondron_perception

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<dondron_perception::PerceptionNode>());
  rclcpp::shutdown();
  return 0;
}
