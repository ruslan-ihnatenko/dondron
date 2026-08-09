#include <chrono>
#include <cmath>
#include <memory>
#include <string>

#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/condition_node.h"
#include "dondron_state_machine/altitude_control.hpp"
#include "dondron_state_machine/bt_nodes.hpp"
#include "dondron_state_machine/orbit_control.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "px4_msgs/msg/vehicle_command.hpp"
#include "std_msgs/msg/bool.hpp"

namespace dondron_state_machine
{

namespace
{
MissionContext::Ptr get_context(const BT::TreeNode & node)
{
  auto ctx = node.config().blackboard->get<MissionContext::Ptr>("mission_context");
  return ctx;
}

void publish_vehicle_command(
  const rclcpp::Node::SharedPtr & ros_node,
  uint16_t command,
  float p1 = 0.0f, float p2 = 0.0f, float p3 = 0.0f, float p4 = 0.0f,
  float p5 = 0.0f, float p6 = 0.0f, float p7 = 0.0f)
{
  static rclcpp::Publisher<px4_msgs::msg::VehicleCommand>::SharedPtr pub;
  if (!pub) {
    pub = ros_node->create_publisher<px4_msgs::msg::VehicleCommand>(
      "/fmu/in/vehicle_command", rclcpp::QoS(10));
  }
  px4_msgs::msg::VehicleCommand msg;
  msg.timestamp = ros_node->now().nanoseconds() / 1000;
  msg.command = command;
  msg.param1 = p1;
  msg.param2 = p2;
  msg.param3 = p3;
  msg.param4 = p4;
  msg.param5 = p5;
  msg.param6 = p6;
  msg.param7 = p7;
  msg.target_system = 1;
  msg.target_component = 1;
  msg.source_system = 1;
  msg.source_component = 1;
  msg.from_external = true;
  pub->publish(msg);
}
}  // namespace

class ArmAction : public BT::StatefulActionNode
{
public:
  ArmAction(const std::string & name, const BT::NodeConfig & config)
  : BT::StatefulActionNode(name, config) {}

  static BT::PortsList providedPorts() {return {};}

  BT::NodeStatus onStart() override
  {
    start_ = std::chrono::steady_clock::now();
    auto ctx = get_context(*this);
    publish_vehicle_command(
      ctx->node, px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 1.0f);
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    auto ctx = get_context(*this);
    if (ctx->is_armed()) {
      return BT::NodeStatus::SUCCESS;
    }
    const auto elapsed = std::chrono::steady_clock::now() - start_;
    if (elapsed > std::chrono::seconds(10)) {
      RCLCPP_ERROR(
        ctx->node->get_logger(),
        "Arm timed out — pre_flight_checks_pass=%s gcs_connection_lost=%s "
        "(start QGroundControl or run `commander check` in PX4 shell)",
        ctx->pre_flight_checks_pass() ? "true" : "false",
        ctx->gcs_connection_lost() ? "true" : "false");
      return BT::NodeStatus::FAILURE;
    }
    publish_vehicle_command(
      ctx->node, px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 1.0f);
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override {}

private:
  std::chrono::steady_clock::time_point start_;
};

class TakeoffAction : public BT::StatefulActionNode
{
public:
  TakeoffAction(const std::string & name, const BT::NodeConfig & config)
  : BT::StatefulActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<double>("altitude_m", 3.0, "Takeoff altitude AMSL (m)")};
  }

  BT::NodeStatus onStart() override
  {
    start_ = std::chrono::steady_clock::now();
    altitude_m_ = getInput<double>("altitude_m").value_or(3.0);
    auto ctx = get_context(*this);
    publish_vehicle_command(
      ctx->node, px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_TAKEOFF,
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, static_cast<float>(altitude_m_));
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    const auto elapsed = std::chrono::steady_clock::now() - start_;
    if (elapsed > std::chrono::seconds(15)) {
      return BT::NodeStatus::SUCCESS;
    }
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override {}

private:
  std::chrono::steady_clock::time_point start_;
  double altitude_m_{3.0};
};

class ClimbToAltitudeAction : public BT::StatefulActionNode
{
public:
  ClimbToAltitudeAction(const std::string & name, const BT::NodeConfig & config)
  : BT::StatefulActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<double>("altitude_m", 3.0, "Target height above local origin (m, positive up)"),
      BT::InputPort<double>("climb_rate_mps", 1.0, "Upward speed (m/s)"),
      BT::InputPort<double>("tolerance_m", 0.3, "Success band below target altitude (m)"),
      BT::InputPort<double>("timeout_s", 30.0, "Max time to reach target altitude (s)"),
    };
  }

  BT::NodeStatus onStart() override
  {
    start_ = std::chrono::steady_clock::now();
    altitude_m_ = getInput<double>("altitude_m").value_or(3.0);
    climb_rate_mps_ = getInput<double>("climb_rate_mps").value_or(1.0);
    tolerance_m_ = getInput<double>("tolerance_m").value_or(0.3);
    timeout_s_ = getInput<double>("timeout_s").value_or(30.0);
    auto ctx = get_context(*this);
    static rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr pub;
    if (!pub) {
      pub = ctx->node->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/flight_api/cmd_setpoint", rclcpp::QoS(10));
    }
    cmd_pub_ = pub;

    if (ctx->has_altitude()) {
      const auto eval = evaluate_climb_to_altitude(
        ctx->altitude_m(), altitude_m_, tolerance_m_, climb_rate_mps_);
      if (eval.decision == ClimbDecision::Success) {
        publish_hold(ctx);
        RCLCPP_INFO(
          ctx->node->get_logger(),
          "ClimbToAltitude: already at %.2f m (target %.2f m) — skipping climb",
          ctx->altitude_m(), altitude_m_);
        return BT::NodeStatus::SUCCESS;
      }
    }

    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    auto ctx = get_context(*this);
    const auto elapsed = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - start_).count();

    if (elapsed > timeout_s_) {
      if (ctx->has_altitude()) {
        RCLCPP_ERROR(
          ctx->node->get_logger(),
          "ClimbToAltitude timed out at %.2f m (target %.2f m, tolerance %.2f m)",
          ctx->altitude_m(), altitude_m_, tolerance_m_);
      } else {
        RCLCPP_ERROR(
          ctx->node->get_logger(),
          "ClimbToAltitude timed out — no valid /fmu/out/vehicle_local_position_v1");
      }
      publish_hold(ctx);
      return BT::NodeStatus::FAILURE;
    }

    if (!ctx->has_altitude()) {
      RCLCPP_WARN_THROTTLE(
        ctx->node->get_logger(), *ctx->node->get_clock(), 2000,
        "ClimbToAltitude waiting for valid local position...");
      return BT::NodeStatus::RUNNING;
    }

    const double current_altitude_m = ctx->altitude_m();
    const auto eval = evaluate_climb_to_altitude(
      current_altitude_m, altitude_m_, tolerance_m_, climb_rate_mps_);

    geometry_msgs::msg::TwistStamped cmd;
    cmd.header.stamp = ctx->node->now();
    cmd.header.frame_id = "ned";
    cmd.twist.linear.z = eval.vz_ned;
    cmd_pub_->publish(cmd);

    if (eval.decision == ClimbDecision::Success) {
      publish_hold(ctx);
      RCLCPP_INFO(
        ctx->node->get_logger(),
        "ClimbToAltitude reached %.2f m (target %.2f m)",
        current_altitude_m, altitude_m_);
      return BT::NodeStatus::SUCCESS;
    }

    RCLCPP_INFO_THROTTLE(
      ctx->node->get_logger(), *ctx->node->get_clock(), 2000,
      "ClimbToAltitude climbing: %.2f m -> %.2f m",
      current_altitude_m, altitude_m_);
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override
  {
    if (cmd_pub_) {
      publish_hold(get_context(*this));
    }
  }

private:
  void publish_hold(const MissionContext::Ptr & ctx)
  {
    geometry_msgs::msg::TwistStamped hold;
    hold.header.stamp = ctx->node->now();
    hold.header.frame_id = "ned";
    cmd_pub_->publish(hold);
  }

  std::chrono::steady_clock::time_point start_;
  double altitude_m_{3.0};
  double climb_rate_mps_{1.0};
  double tolerance_m_{0.3};
  double timeout_s_{30.0};
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_pub_;
};

class EnableOffboardAction : public BT::SyncActionNode
{
public:
  EnableOffboardAction(const std::string & name, const BT::NodeConfig & config)
  : BT::SyncActionNode(name, config) {}

  static BT::PortsList providedPorts() {return {};}

  BT::NodeStatus tick() override
  {
    auto ctx = get_context(*this);
    static rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr pub;
    if (!pub) {
      pub = ctx->node->create_publisher<std_msgs::msg::Bool>(
        "/flight_api/enable_offboard", rclcpp::QoS(10));
    }
    std_msgs::msg::Bool msg;
    msg.data = true;
    pub->publish(msg);
    return BT::NodeStatus::SUCCESS;
  }
};

class ExecuteSearchPatternAction : public BT::StatefulActionNode
{
public:
  ExecuteSearchPatternAction(const std::string & name, const BT::NodeConfig & config)
  : BT::StatefulActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<double>("duration_s", 5.0, "Search leg duration"),
      BT::InputPort<double>("forward_speed_mps", 0.5, "Forward speed"),
      BT::InputPort<double>("yaw_rate_radps", 0.3, "Yaw rate for search pattern"),
    };
  }

  BT::NodeStatus onStart() override
  {
    start_ = std::chrono::steady_clock::now();
    duration_s_ = getInput<double>("duration_s").value_or(5.0);
    forward_speed_ = getInput<double>("forward_speed_mps").value_or(0.5);
    yaw_rate_ = getInput<double>("yaw_rate_radps").value_or(0.3);
    auto ctx = get_context(*this);
    static rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr pub;
    if (!pub) {
      pub = ctx->node->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/flight_api/cmd_setpoint", rclcpp::QoS(10));
    }
    cmd_pub_ = pub;
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    const auto elapsed = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - start_).count();

    geometry_msgs::msg::TwistStamped cmd;
    cmd.header.stamp = get_context(*this)->node->now();
    cmd.header.frame_id = "body_frd";
    cmd.twist.linear.x = forward_speed_;
    cmd.twist.angular.z = yaw_rate_ * std::sin(elapsed);
    cmd_pub_->publish(cmd);

    if (elapsed >= duration_s_) {
      return BT::NodeStatus::SUCCESS;
    }
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override {}

private:
  std::chrono::steady_clock::time_point start_;
  double duration_s_{5.0};
  double forward_speed_{0.5};
  double yaw_rate_{0.3};
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_pub_;
};

class ExecuteOrbitSearchAction : public BT::StatefulActionNode
{
public:
  ExecuteOrbitSearchAction(const std::string & name, const BT::NodeConfig & config)
  : BT::StatefulActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<double>("radius_m", 10.0, "Target orbit radius (m)"),
      BT::InputPort<double>("tangential_speed_mps", 0.5, "Orbit speed (m/s)"),
      BT::InputPort<double>("duration_s", 60.0, "Orbit duration (s)"),
      BT::InputPort<double>("center_x_m", 0.0, "Orbit center north offset (m, NED)"),
      BT::InputPort<double>("center_y_m", 0.0, "Orbit center east offset (m, NED)"),
      BT::InputPort<double>("radius_tolerance_m", 1.0, "Band to start tangential orbit (m)"),
      BT::InputPort<double>("timeout_s", 30.0, "Max wait for valid local position (s)"),
    };
  }

  BT::NodeStatus onStart() override
  {
    start_ = std::chrono::steady_clock::now();
    radius_m_ = getInput<double>("radius_m").value_or(10.0);
    tangential_speed_mps_ = getInput<double>("tangential_speed_mps").value_or(0.5);
    duration_s_ = getInput<double>("duration_s").value_or(60.0);
    center_x_m_ = getInput<double>("center_x_m").value_or(0.0);
    center_y_m_ = getInput<double>("center_y_m").value_or(0.0);
    radius_tolerance_m_ = getInput<double>("radius_tolerance_m").value_or(1.0);
    timeout_s_ = getInput<double>("timeout_s").value_or(30.0);
    auto ctx = get_context(*this);
    static rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr pub;
    if (!pub) {
      pub = ctx->node->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/flight_api/cmd_setpoint", rclcpp::QoS(10));
    }
    cmd_pub_ = pub;
    RCLCPP_INFO(
      ctx->node->get_logger(),
      "ExecuteOrbitSearch: radius %.1f m, speed %.2f m/s, duration %.0f s, center (%.1f N, %.1f E)",
      radius_m_, tangential_speed_mps_, duration_s_, center_x_m_, center_y_m_);
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    auto ctx = get_context(*this);
    const auto elapsed = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - start_).count();

    if (elapsed > timeout_s_ && !ctx->has_horizontal_position()) {
      RCLCPP_ERROR(
        ctx->node->get_logger(),
        "ExecuteOrbitSearch timed out — no valid horizontal local position");
      publish_hold(ctx);
      return BT::NodeStatus::FAILURE;
    }

    if (elapsed >= duration_s_) {
      publish_hold(ctx);
      RCLCPP_INFO(
        ctx->node->get_logger(), "ExecuteOrbitSearch completed %.0f s orbit",
        duration_s_);
      return BT::NodeStatus::SUCCESS;
    }

    if (!ctx->has_horizontal_position()) {
      RCLCPP_WARN_THROTTLE(
        ctx->node->get_logger(), *ctx->node->get_clock(), 2000,
        "ExecuteOrbitSearch waiting for valid local position xy...");
      return BT::NodeStatus::RUNNING;
    }

    const double heading = ctx->has_heading() ? ctx->heading_rad() : 0.0;
    const auto setpoint = compute_orbit_setpoint_ned(
      ctx->position_x_m(), ctx->position_y_m(),
      center_x_m_, center_y_m_, radius_m_, tangential_speed_mps_, heading,
      radius_tolerance_m_);

    geometry_msgs::msg::TwistStamped cmd;
    cmd.header.stamp = ctx->node->now();
    cmd.header.frame_id = "ned";
    cmd.twist.linear.x = setpoint.vx;
    cmd.twist.linear.y = setpoint.vy;
    cmd.twist.linear.z = 0.0;
    cmd.twist.angular.z = setpoint.yawspeed;
    cmd_pub_->publish(cmd);

    if (setpoint.phase == OrbitPhase::Expand) {
      RCLCPP_INFO_THROTTLE(
        ctx->node->get_logger(), *ctx->node->get_clock(), 3000,
        "ExecuteOrbitSearch expanding to radius %.1f m (now %.1f m)",
        radius_m_, std::hypot(
          ctx->position_x_m() - center_x_m_, ctx->position_y_m() - center_y_m_));
    } else {
      RCLCPP_INFO_THROTTLE(
        ctx->node->get_logger(), *ctx->node->get_clock(), 3000,
        "ExecuteOrbitSearch orbiting at (%.1f N, %.1f E), yaw->center",
        ctx->position_x_m(), ctx->position_y_m());
    }
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override
  {
    if (cmd_pub_) {
      publish_hold(get_context(*this));
    }
  }

private:
  void publish_hold(const MissionContext::Ptr & ctx)
  {
    geometry_msgs::msg::TwistStamped hold;
    hold.header.stamp = ctx->node->now();
    hold.header.frame_id = "ned";
    cmd_pub_->publish(hold);
  }

  std::chrono::steady_clock::time_point start_;
  double radius_m_{10.0};
  double tangential_speed_mps_{0.5};
  double duration_s_{60.0};
  double center_x_m_{0.0};
  double center_y_m_{0.0};
  double radius_tolerance_m_{1.0};
  double timeout_s_{30.0};
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_pub_;
};

class TargetAcquiredCondition : public BT::ConditionNode
{
public:
  TargetAcquiredCondition(const std::string & name, const BT::NodeConfig & config)
  : BT::ConditionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<std::string>("target_class_id", "0", "Target class id string"),
      BT::InputPort<double>("min_score", 0.5, "Minimum detection score"),
    };
  }

  BT::NodeStatus tick() override
  {
    const auto class_id = getInput<std::string>("target_class_id").value_or("0");
    const auto min_score = getInput<double>("min_score").value_or(0.5);
    return get_context(*this)->target_acquired(class_id, min_score) ?
           BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
  }
};

class TrackTargetAction : public BT::StatefulActionNode
{
public:
  TrackTargetAction(const std::string & name, const BT::NodeConfig & config)
  : BT::StatefulActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<std::string>("target_class_id", "0", "Target class id string"),
      BT::InputPort<double>("min_score", 0.5, "Minimum detection score"),
      BT::InputPort<int>("stable_ticks", 3, "Consecutive stable detections to enter TRACK"),
      BT::InputPort<int>("lost_ticks", 5, "Consecutive misses before FAILURE"),
    };
  }

  BT::NodeStatus onStart() override
  {
    class_id_ = getInput<std::string>("target_class_id").value_or("0");
    min_score_ = getInput<double>("min_score").value_or(0.5);
    stable_ticks_ = getInput<int>("stable_ticks").value_or(3);
    lost_ticks_ = getInput<int>("lost_ticks").value_or(5);
    get_context(*this)->reset_track_stability();
    in_track_ = false;
    miss_count_ = 0;
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    auto ctx = get_context(*this);
    const bool acquired = ctx->target_acquired(class_id_, min_score_);

    if (!in_track_) {
      if (ctx->detections_stable(class_id_, min_score_, stable_ticks_)) {
        in_track_ = true;
        RCLCPP_INFO(ctx->node->get_logger(), "TRACK: visual lock acquired (class %s)",
            class_id_.c_str());
      }
      return BT::NodeStatus::RUNNING;
    }

    if (acquired) {
      miss_count_ = 0;
      RCLCPP_INFO_THROTTLE(
        ctx->node->get_logger(), *ctx->node->get_clock(), 2000,
        "TRACK: maintaining visual lock");
      return BT::NodeStatus::RUNNING;
    }

    ++miss_count_;
    if (miss_count_ >= lost_ticks_) {
      RCLCPP_WARN(ctx->node->get_logger(), "TRACK: target lost");
      return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override {}

private:
  std::string class_id_{"0"};
  double min_score_{0.5};
  int stable_ticks_{3};
  int lost_ticks_{5};
  bool in_track_{false};
  int miss_count_{0};
};

class RCOverrideNotActiveCondition : public BT::ConditionNode
{
public:
  RCOverrideNotActiveCondition(const std::string & name, const BT::NodeConfig & config)
  : BT::ConditionNode(name, config) {}

  static BT::PortsList providedPorts() {return {};}

  BT::NodeStatus tick() override
  {
    return get_context(*this)->rc_override_not_active() ?
           BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
  }
};

class RTLAction : public BT::SyncActionNode
{
public:
  RTLAction(const std::string & name, const BT::NodeConfig & config)
  : BT::SyncActionNode(name, config) {}

  static BT::PortsList providedPorts() {return {};}

  BT::NodeStatus tick() override
  {
    auto ctx = get_context(*this);
    publish_vehicle_command(
      ctx->node, px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_RETURN_TO_LAUNCH);
    RCLCPP_WARN(ctx->node->get_logger(), "DISENGAGE: RTL commanded");
    return BT::NodeStatus::SUCCESS;
  }
};

class LandAction : public BT::SyncActionNode
{
public:
  LandAction(const std::string & name, const BT::NodeConfig & config)
  : BT::SyncActionNode(name, config) {}

  static BT::PortsList providedPorts() {return {};}

  BT::NodeStatus tick() override
  {
    auto ctx = get_context(*this);
    publish_vehicle_command(
      ctx->node, px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND);
    RCLCPP_WARN(ctx->node->get_logger(), "DISENGAGE: LAND commanded");
    return BT::NodeStatus::SUCCESS;
  }
};

class WaitOffboardAction : public BT::StatefulActionNode
{
public:
  WaitOffboardAction(const std::string & name, const BT::NodeConfig & config)
  : BT::StatefulActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<double>("timeout_s", 10.0, "Wait timeout")};
  }

  BT::NodeStatus onStart() override
  {
    start_ = std::chrono::steady_clock::now();
    timeout_s_ = getInput<double>("timeout_s").value_or(10.0);
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    if (get_context(*this)->is_offboard()) {
      return BT::NodeStatus::SUCCESS;
    }
    const auto elapsed = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - start_).count();
    if (elapsed > timeout_s_) {
      RCLCPP_ERROR(
        get_context(*this)->node->get_logger(),
        "WaitOffboard timed out — check /fmu/in/trajectory_setpoint hz and "
        "VEHICLE_CMD_DO_SET_MODE param2=6 (offboard main mode)");
      return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override {}

private:
  std::chrono::steady_clock::time_point start_;
  double timeout_s_{10.0};
};

void register_bt_nodes(BT::BehaviorTreeFactory & factory)
{
  factory.registerNodeType<ArmAction>("Arm");
  factory.registerNodeType<TakeoffAction>("Takeoff");
  factory.registerNodeType<ClimbToAltitudeAction>("ClimbToAltitude");
  factory.registerNodeType<EnableOffboardAction>("EnableOffboard");
  factory.registerNodeType<WaitOffboardAction>("WaitOffboard");
  factory.registerNodeType<ExecuteSearchPatternAction>("ExecuteSearchPattern");
  factory.registerNodeType<ExecuteOrbitSearchAction>("ExecuteOrbitSearch");
  factory.registerNodeType<TargetAcquiredCondition>("TargetAcquired");
  factory.registerNodeType<TrackTargetAction>("TrackTarget");
  factory.registerNodeType<RCOverrideNotActiveCondition>("RC_Override_Not_Active");
  factory.registerNodeType<RTLAction>("RTL");
  factory.registerNodeType<LandAction>("Land");
}

}  // namespace dondron_state_machine
