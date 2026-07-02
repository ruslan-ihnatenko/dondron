#include <cmath>
#include <memory>
#include <string>

#include "geometry_msgs/msg/twist_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"

namespace dondron_flight_api
{

class HarnessNode : public rclcpp::Node
{
public:
  HarnessNode()
  : Node("flight_api_harness")
  {
    cmd_topic_ = declare_parameter<std::string>("cmd_setpoint_topic", "/flight_api/cmd_setpoint");
    publish_rate_hz_ = declare_parameter<double>("publish_rate_hz", 10.0);
    forward_speed_mps_ = declare_parameter<double>("forward_speed_mps", 1.0);
    yaw_rate_radps_ = declare_parameter<double>("yaw_rate_radps", 0.2);
    enable_offboard_after_s_ = declare_parameter<double>("enable_offboard_after_s", 2.0);
    cmd_frame_ = declare_parameter<std::string>("cmd_frame", "body_frd");

    cmd_pub_ = create_publisher<geometry_msgs::msg::TwistStamped>(cmd_topic_, rclcpp::QoS(10));
    enable_offboard_pub_ = create_publisher<std_msgs::msg::Bool>(
      "/flight_api/enable_offboard", rclcpp::QoS(10));

    const auto period_ms = static_cast<int64_t>(1000.0 / publish_rate_hz_);
    timer_ = create_wall_timer(
      std::chrono::milliseconds(period_ms),
      std::bind(&HarnessNode::on_timer, this));

    start_time_ = now();
    RCLCPP_INFO(
      get_logger(),
      "Flight API harness publishing to %s at %.1f Hz (frame=%s)",
      cmd_topic_.c_str(), publish_rate_hz_, cmd_frame_.c_str());
  }

private:
  void on_timer()
  {
    const double elapsed = (now() - start_time_).seconds();
    if (elapsed >= enable_offboard_after_s_ && !offboard_enabled_) {
      std_msgs::msg::Bool enable;
      enable.data = true;
      enable_offboard_pub_->publish(enable);
      offboard_enabled_ = true;
      RCLCPP_INFO(get_logger(), "Harness requested offboard mode");
    }

    geometry_msgs::msg::TwistStamped cmd;
    cmd.header.stamp = now();
    cmd.header.frame_id = cmd_frame_;
    cmd.twist.linear.x = forward_speed_mps_;
    cmd.twist.linear.y = 0.0;
    cmd.twist.linear.z = 0.0;
    cmd.twist.angular.z = yaw_rate_radps_ * std::sin(elapsed * 0.5);
    cmd_pub_->publish(cmd);
  }

  std::string cmd_topic_;
  std::string cmd_frame_;
  double publish_rate_hz_{10.0};
  double forward_speed_mps_{1.0};
  double yaw_rate_radps_{0.2};
  double enable_offboard_after_s_{2.0};
  bool offboard_enabled_{false};
  rclcpp::Time start_time_;

  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr enable_offboard_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace dondron_flight_api

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<dondron_flight_api::HarnessNode>());
  rclcpp::shutdown();
  return 0;
}
