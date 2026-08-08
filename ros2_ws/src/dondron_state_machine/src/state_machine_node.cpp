#include <ament_index_cpp/get_package_share_directory.hpp>
#include <chrono>
#include <memory>
#include <string>

#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_cpp/loggers/bt_cout_logger.h"
#include "dondron_state_machine/bt_nodes.hpp"
#include "dondron_state_machine/mission_context.hpp"
#include "rclcpp/rclcpp.hpp"

namespace dondron_state_machine
{

class StateMachineNode : public rclcpp::Node
{
public:
  StateMachineNode()
  : Node("state_machine_node")
  {
    bt_xml_path_ = declare_parameter<std::string>("bt_xml_path", "");
    tick_rate_hz_ = declare_parameter<double>("tick_rate_hz", 10.0);
    search_pattern_ = declare_parameter<std::string>("search_pattern", "weave");

    if (bt_xml_path_.empty()) {
      bt_xml_path_ = ament_index_cpp::get_package_share_directory("dondron_state_machine") +
        "/behavior_trees/mission.xml";
    }
  }

  void init_mission()
  {
    mission_context_ = std::make_shared<MissionContext>(shared_from_this());

    register_bt_nodes(factory_);
    factory_.registerBehaviorTreeFromFile(bt_xml_path_);
    tree_ = factory_.createTree("MissionRoot");
    tree_.rootBlackboard()->set("mission_context", mission_context_);
    tree_.rootBlackboard()->set("search_pattern", search_pattern_);

    logger_ = std::make_unique<BT::StdCoutLogger>(tree_);

    const auto period_ms = static_cast<int64_t>(1000.0 / tick_rate_hz_);
    tick_timer_ = create_wall_timer(
      std::chrono::milliseconds(period_ms),
      std::bind(&StateMachineNode::on_tick, this));

    RCLCPP_INFO(
      get_logger(), "State machine loaded BT from %s (search_pattern=%s)",
      bt_xml_path_.c_str(), search_pattern_.c_str());
  }

private:
  void on_tick()
  {
    if (mission_halted_) {
      return;
    }
    const auto status = tree_.tickOnce();
    if (status == BT::NodeStatus::SUCCESS) {
      RCLCPP_INFO(get_logger(), "Mission BT completed SUCCESS");
      mission_halted_ = true;
      tick_timer_->cancel();
    } else if (status == BT::NodeStatus::FAILURE) {
      RCLCPP_ERROR(
        get_logger(),
        "Mission BT FAILURE — halted (no RTL). Fix the failing step and relaunch.");
      mission_halted_ = true;
      tick_timer_->cancel();
    }
  }

  std::string bt_xml_path_;
  std::string search_pattern_{"weave"};
  double tick_rate_hz_{10.0};
  bool mission_halted_{false};
  MissionContext::Ptr mission_context_;
  BT::BehaviorTreeFactory factory_;
  BT::Tree tree_;
  std::unique_ptr<BT::StdCoutLogger> logger_;
  rclcpp::TimerBase::SharedPtr tick_timer_;
};

}  // namespace dondron_state_machine

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<dondron_state_machine::StateMachineNode>();
  node->init_mission();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
