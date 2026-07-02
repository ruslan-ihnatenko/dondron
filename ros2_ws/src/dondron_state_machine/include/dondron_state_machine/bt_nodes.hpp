#ifndef DONDRON_STATE_MACHINE__BT_NODES_HPP_
#define DONDRON_STATE_MACHINE__BT_NODES_HPP_

#include "behaviortree_cpp/bt_factory.h"
#include "dondron_state_machine/mission_context.hpp"

namespace dondron_state_machine
{

void register_bt_nodes(BT::BehaviorTreeFactory & factory);

}  // namespace dondron_state_machine

#endif  // DONDRON_STATE_MACHINE__BT_NODES_HPP_
