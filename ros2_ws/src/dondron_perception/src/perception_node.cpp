#include <memory>
#include <mutex>
#include <string>

#include "rclcpp/rclcpp.hpp"
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
    detections_topic_ = declare_parameter<std::string>("detections_topic", "/detections");

    image_sub_ = create_subscription<sensor_msgs::msg::Image>(
      image_topic_, rclcpp::SensorDataQoS(),
      [this](const sensor_msgs::msg::Image::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(image_mutex_);
        latest_image_stamp_ = msg->header.stamp;
        has_image_ = true;
      });

    detections_pub_ = create_publisher<vision_msgs::msg::Detection2DArray>(
      detections_topic_, rclcpp::QoS(10));

    const auto period_ms = static_cast<int64_t>(1000.0 / publish_rate_hz_);
    timer_ = create_wall_timer(
      std::chrono::milliseconds(period_ms),
      std::bind(&PerceptionNode::publish_detections, this));

    RCLCPP_INFO(
      get_logger(),
      "Perception stub: %s -> %s at %.1f Hz (frame_id=%s)",
      image_topic_.c_str(), detections_topic_.c_str(), publish_rate_hz_, frame_id_.c_str());
  }

private:
  void publish_detections()
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

  std::string frame_id_;
  std::string image_topic_;
  std::string detections_topic_;
  std::string stub_class_id_;
  double publish_rate_hz_{2.0};
  double stub_score_{0.95};
  double stub_range_m_{10.0};
  double stub_bbox_center_x_{320.0};
  double stub_bbox_center_y_{240.0};
  double stub_bbox_size_x_{80.0};
  double stub_bbox_size_y_{60.0};

  std::mutex image_mutex_;
  rclcpp::Time latest_image_stamp_;
  bool has_image_{false};

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
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
