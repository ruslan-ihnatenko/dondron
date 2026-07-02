#include <array>
#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <string>

#include "dondron_flight_api/frame_transform.hpp"
#include "dondron_flight_api/msg/flight_api_status.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "px4_msgs/msg/offboard_control_mode.hpp"
#include "px4_msgs/msg/trajectory_setpoint.hpp"
#include "px4_msgs/msg/vehicle_attitude.hpp"
#include "px4_msgs/msg/vehicle_command.hpp"
#include "px4_msgs/msg/vehicle_status.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"

namespace dondron_flight_api
{

namespace
{
constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();
// PX4_CUSTOM_MAIN_MODE_OFFBOARD (MAVLink custom mode) — not NAVIGATION_STATE_OFFBOARD (14).
constexpr float kPx4MainModeOffboard = 6.0f;
constexpr uint32_t kMinSetpointsBeforeOffboard = 10;

std::string nav_state_name(uint8_t nav_state)
{
  using px4_msgs::msg::VehicleStatus;
  switch (nav_state) {
    case VehicleStatus::NAVIGATION_STATE_MANUAL: return "MANUAL";
    case VehicleStatus::NAVIGATION_STATE_ALTCTL: return "ALTCTL";
    case VehicleStatus::NAVIGATION_STATE_POSCTL: return "POSCTL";
    case VehicleStatus::NAVIGATION_STATE_AUTO_MISSION: return "AUTO_MISSION";
    case VehicleStatus::NAVIGATION_STATE_AUTO_LOITER: return "AUTO_LOITER";
    case VehicleStatus::NAVIGATION_STATE_AUTO_RTL: return "AUTO_RTL";
    case VehicleStatus::NAVIGATION_STATE_OFFBOARD: return "OFFBOARD";
    case VehicleStatus::NAVIGATION_STATE_AUTO_TAKEOFF: return "AUTO_TAKEOFF";
    case VehicleStatus::NAVIGATION_STATE_AUTO_LAND: return "AUTO_LAND";
    default: return "UNKNOWN";
  }
}
}  // namespace

class FlightApiNode : public rclcpp::Node
{
public:
  FlightApiNode()
  : Node("flight_api_node")
  {
    cmd_topic_ = declare_parameter<std::string>("cmd_setpoint_topic", "/flight_api/cmd_setpoint");
    status_topic_ = declare_parameter<std::string>("status_topic", "/flight_api/status");
    default_frame_ = declare_parameter<std::string>("default_cmd_frame", "body_frd");
    setpoint_rate_hz_ = declare_parameter<double>("setpoint_rate_hz", 20.0);
    cmd_timeout_s_ = declare_parameter<double>("cmd_timeout_s", 0.5);
    auto_offboard_ = declare_parameter<bool>("auto_request_offboard", true);
    takeoff_alt_m_ = declare_parameter<double>("takeoff_alt_m", 3.0);
    vehicle_status_topic_ = declare_parameter<std::string>(
      "vehicle_status_topic", "/fmu/out/vehicle_status_v4");

    cmd_sub_ = create_subscription<geometry_msgs::msg::TwistStamped>(
      cmd_topic_, rclcpp::QoS(10),
      [this](const geometry_msgs::msg::TwistStamped::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_cmd_ = *msg;
        has_cmd_ = true;
        last_cmd_time_ = now();
      });

    enable_offboard_sub_ = create_subscription<std_msgs::msg::Bool>(
      "/flight_api/enable_offboard", rclcpp::QoS(10),
      [this](const std_msgs::msg::Bool::SharedPtr msg) {
        offboard_requested_ = msg->data;
      });

    const auto px4_qos = rclcpp::SensorDataQoS();

    vehicle_status_sub_ = create_subscription<px4_msgs::msg::VehicleStatus>(
      vehicle_status_topic_, px4_qos,
      [this](const px4_msgs::msg::VehicleStatus::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_status_ = *msg;
        has_status_ = true;
      });

    vehicle_attitude_sub_ = create_subscription<px4_msgs::msg::VehicleAttitude>(
      "/fmu/out/vehicle_attitude", px4_qos,
      [this](const px4_msgs::msg::VehicleAttitude::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_attitude_ = *msg;
        has_attitude_ = true;
      });

    offboard_mode_pub_ = create_publisher<px4_msgs::msg::OffboardControlMode>(
      "/fmu/in/offboard_control_mode", rclcpp::QoS(10));
    trajectory_pub_ = create_publisher<px4_msgs::msg::TrajectorySetpoint>(
      "/fmu/in/trajectory_setpoint", rclcpp::QoS(10));
    vehicle_command_pub_ = create_publisher<px4_msgs::msg::VehicleCommand>(
      "/fmu/in/vehicle_command", rclcpp::QoS(10));
    status_pub_ = create_publisher<msg::FlightApiStatus>(status_topic_, rclcpp::QoS(10));

    const auto period_ms = static_cast<int64_t>(1000.0 / setpoint_rate_hz_);
    setpoint_timer_ = create_wall_timer(
      std::chrono::milliseconds(period_ms),
      std::bind(&FlightApiNode::on_setpoint_timer, this));

    status_timer_ = create_wall_timer(
      std::chrono::milliseconds(200),
      std::bind(&FlightApiNode::publish_status, this));

    RCLCPP_INFO(
      get_logger(),
      "Flight API: %s -> PX4 offboard (frame default=%s, rate=%.1f Hz)",
      cmd_topic_.c_str(), default_frame_.c_str(), setpoint_rate_hz_);
  }

private:
  void on_setpoint_timer()
  {
    geometry_msgs::msg::TwistStamped cmd;
    px4_msgs::msg::VehicleStatus status;
    px4_msgs::msg::VehicleAttitude attitude;
    bool have_cmd = false;
    bool have_status = false;
    bool have_attitude = false;
    bool offboard_req = false;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      have_cmd = has_cmd_;
      have_status = has_status_;
      have_attitude = has_attitude_;
      offboard_req = offboard_requested_;
      if (have_cmd) {
        cmd = latest_cmd_;
      }
      if (have_status) {
        status = latest_status_;
      }
      if (have_attitude) {
        attitude = latest_attitude_;
      }
    }

    publish_offboard_mode();

    const bool cmd_fresh = have_cmd &&
      (now() - last_cmd_time_).seconds() <= cmd_timeout_s_;
    const bool armed = have_status &&
      status.arming_state == px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED;
    const bool in_offboard = have_status &&
      status.nav_state == px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD;

    if (auto_offboard_ && offboard_req && armed && !in_offboard &&
      setpoint_count_ >= kMinSetpointsBeforeOffboard)
    {
      request_offboard_mode();
    }

    float vn = 0.0f;
    float ve = 0.0f;
    float vd = 0.0f;
    float yawspeed = 0.0f;

    if (cmd_fresh) {
      const std::string frame = cmd.header.frame_id.empty() ? default_frame_ : cmd.header.frame_id;
      if (is_ned_frame(frame)) {
        vn = static_cast<float>(cmd.twist.linear.x);
        ve = static_cast<float>(cmd.twist.linear.y);
        vd = static_cast<float>(cmd.twist.linear.z);
      } else if (is_body_frd_frame(frame) && have_attitude) {
        const auto ned = body_frd_to_ned(
          static_cast<float>(cmd.twist.linear.x),
          static_cast<float>(cmd.twist.linear.y),
          static_cast<float>(cmd.twist.linear.z),
          {attitude.q[0], attitude.q[1], attitude.q[2], attitude.q[3]});
        vn = ned[0];
        ve = ned[1];
        vd = ned[2];
      } else {
        RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 5000,
          "Ignoring cmd_setpoint: unknown frame '%s' or missing attitude", frame.c_str());
      }
      yawspeed = static_cast<float>(cmd.twist.angular.z);
    }

    publish_trajectory_setpoint(vn, ve, vd, yawspeed);
    ++setpoint_count_;
  }

  void publish_offboard_mode()
  {
    px4_msgs::msg::OffboardControlMode msg;
    msg.timestamp = now().nanoseconds() / 1000;
    msg.position = false;
    msg.velocity = true;
    msg.acceleration = false;
    msg.attitude = false;
    msg.body_rate = false;
    offboard_mode_pub_->publish(msg);
  }

  void publish_trajectory_setpoint(float vn, float ve, float vd, float yawspeed)
  {
    px4_msgs::msg::TrajectorySetpoint msg;
    msg.timestamp = now().nanoseconds() / 1000;
    msg.position = {kNaN, kNaN, kNaN};
    msg.velocity = {vn, ve, vd};
    msg.acceleration = {kNaN, kNaN, kNaN};
    msg.yaw = kNaN;
    msg.yawspeed = yawspeed;
    trajectory_pub_->publish(msg);
  }

  void request_offboard_mode()
  {
    const auto now_us = now().nanoseconds() / 1000;
    px4_msgs::msg::VehicleCommand cmd;
    cmd.timestamp = now_us;
    cmd.command = px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE;
    cmd.param1 = 1.0f;
    cmd.param2 = kPx4MainModeOffboard;
    cmd.target_system = 1;
    cmd.target_component = 1;
    cmd.source_system = 1;
    cmd.source_component = 1;
    cmd.from_external = true;
    vehicle_command_pub_->publish(cmd);
  }

  void publish_status()
  {
    msg::FlightApiStatus out;
    out.header.stamp = now();
    out.header.frame_id = "map";

    std::lock_guard<std::mutex> lock(mutex_);
    if (has_status_) {
      out.armed = latest_status_.arming_state ==
        px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED;
      out.offboard_active = latest_status_.nav_state ==
        px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD;
      out.px4_nav_state = latest_status_.nav_state;
      out.nav_state_name = nav_state_name(latest_status_.nav_state);
    }
    status_pub_->publish(out);
  }

  std::string cmd_topic_;
  std::string status_topic_;
  std::string vehicle_status_topic_;
  std::string default_frame_;
  double setpoint_rate_hz_{20.0};
  double cmd_timeout_s_{0.5};
  double takeoff_alt_m_{3.0};
  bool auto_offboard_{true};
  bool offboard_requested_{false};
  uint32_t setpoint_count_{0};

  std::mutex mutex_;
  geometry_msgs::msg::TwistStamped latest_cmd_;
  px4_msgs::msg::VehicleStatus latest_status_;
  px4_msgs::msg::VehicleAttitude latest_attitude_;
  bool has_cmd_{false};
  bool has_status_{false};
  bool has_attitude_{false};
  rclcpp::Time last_cmd_time_;

  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_sub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr enable_offboard_sub_;
  rclcpp::Subscription<px4_msgs::msg::VehicleStatus>::SharedPtr vehicle_status_sub_;
  rclcpp::Subscription<px4_msgs::msg::VehicleAttitude>::SharedPtr vehicle_attitude_sub_;
  rclcpp::Publisher<px4_msgs::msg::OffboardControlMode>::SharedPtr offboard_mode_pub_;
  rclcpp::Publisher<px4_msgs::msg::TrajectorySetpoint>::SharedPtr trajectory_pub_;
  rclcpp::Publisher<px4_msgs::msg::VehicleCommand>::SharedPtr vehicle_command_pub_;
  rclcpp::Publisher<msg::FlightApiStatus>::SharedPtr status_pub_;
  rclcpp::TimerBase::SharedPtr setpoint_timer_;
  rclcpp::TimerBase::SharedPtr status_timer_;
};

}  // namespace dondron_flight_api

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<dondron_flight_api::FlightApiNode>());
  rclcpp::shutdown();
  return 0;
}
