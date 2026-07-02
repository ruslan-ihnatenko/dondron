#include <chrono>
#include <cmath>
#include <memory>
#include <string>

#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/condition_node.h"
#include "dondron_state_machine/bt_nodes.hpp"
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
      BT::InputPort<double>("altitude_m", 3.0, "Climb height (m)"),
      BT::InputPort<double>("climb_rate_mps", 1.0, "Upward speed (m/s)"),
    };
  }

  BT::NodeStatus onStart() override
  {
    start_ = std::chrono::steady_clock::now();
    altitude_m_ = getInput<double>("altitude_m").value_or(3.0);
    climb_rate_mps_ = getInput<double>("climb_rate_mps").value_or(1.0);
    duration_s_ = altitude_m_ / std::max(climb_rate_mps_, 0.1) + 1.0;
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
    cmd.header.frame_id = "ned";
    cmd.twist.linear.z = -static_cast<double>(climb_rate_mps_);
    cmd_pub_->publish(cmd);

    if (elapsed >= duration_s_) {
      geometry_msgs::msg::TwistStamped hold;
      hold.header.stamp = get_context(*this)->node->now();
      hold.header.frame_id = "ned";
      cmd_pub_->publish(hold);
      return BT::NodeStatus::SUCCESS;
    }
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override {}

private:
  std::chrono::steady_clock::time_point start_;
  double altitude_m_{3.0};
  double climb_rate_mps_{1.0};
  double duration_s_{4.0};
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
        RCLCPP_INFO(ctx->node->get_logger(), "TRACK: visual lock acquired (class %s)", class_id_.c_str());
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
  factory.registerNodeType<TargetAcquiredCondition>("TargetAcquired");
  factory.registerNodeType<TrackTargetAction>("TrackTarget");
  factory.registerNodeType<RCOverrideNotActiveCondition>("RC_Override_Not_Active");
  factory.registerNodeType<RTLAction>("RTL");
  factory.registerNodeType<LandAction>("Land");
}

}  // namespace dondron_state_machine
