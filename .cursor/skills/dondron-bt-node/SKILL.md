---
name: dondron-bt-node
description: Add BehaviorTree.CPP v4 nodes for dondron_state_machine. Use for BT actions, conditions, subtrees, or state machine logic through TRACK.
---

# BehaviorTree node (public states)

Package: `dondron_state_machine`  
Library: **BehaviorTree.CPP v4**

Vault: blueprint §4.4

## Public state graph

```
IDLE → ARM → TAKEOFF → SEARCH → ACQUIRE → TRACK
```

**TRACK** is the public terminal autonomous state. Do not add ENGAGE or intercept actions here.

## DISENGAGE pattern

- PX4 `COM_RC_OVERRIDE` triggers mode change at FC level — not BT-initiated
- BT **Condition** node: monitor `/fmu/out/vehicle_status` (or bridged equivalent) for Offboard exit
- On fail: run DISENGAGE → RTL → LAND subtree

## Adding a node

### 1. Register in BT factory

```cpp
factory.registerNodeType<MyAction>("MyAction");
```

### 2. XML tree snippet

```xml
<root BTCPP_format="4">
  <BehaviorTree ID="SearchAndTrack">
    <Sequence>
      <Action ID="ExecuteSearchPattern"/>
      <Condition ID="TargetAcquired"/>  <!-- reads /detections -->
      <Action ID="TrackTarget"/>
    </Sequence>
  </BehaviorTree>
</root>
```

### 3. Action node conventions

- Inherit `BT::SyncActionNode` or `BT::StatefulActionNode`
- Use `rclcpp::Node` shared via blackboard or node wrapper
- Return `NodeStatus::SUCCESS` / `FAILURE` / `RUNNING` correctly

### 4. Conditions on perception

- `TargetAcquired` may subscribe to `/detections` — **read only**
- No setpoint output from BT perception conditions

## Root structure (sketch)

```
ReactiveSequence
├── Condition: RC_Override_Not_Active?
└── Sequence: Arm → Takeoff → SearchAndTrack → RTL
```

## Public boundary

- No ENGAGE subtree
- No detection→setpoint mapping in action nodes
- Trackable BT work → vault task, not inline TODOs

See `.cursor/rules/state-machine.mdc`.
