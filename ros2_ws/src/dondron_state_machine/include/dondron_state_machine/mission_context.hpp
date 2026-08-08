#ifndef DONDRON_STATE_MACHINE__MISSION_CONTEXT_HPP_
#define DONDRON_STATE_MACHINE__MISSION_CONTEXT_HPP_

#include <memory>
#include <mutex>
#include <string>

#include "px4_msgs/msg/vehicle_local_position.hpp"
#include "px4_msgs/msg/vehicle_status.hpp"
#include "rclcpp/rclcpp.hpp"
#include "vision_msgs/msg/detection2_d_array.hpp"

#include "dondron_state_machine/altitude_control.hpp"

namespace dondron_state_machine
{

struct MissionContext
{
  using Ptr = std::shared_ptr<MissionContext>;

  explicit MissionContext(const rclcpp::Node::SharedPtr & node)
  : node(node)
  {
    const auto vehicle_status_topic = node->declare_parameter<std::string>(
      "vehicle_status_topic", "/fmu/out/vehicle_status_v4");
    const auto local_position_topic = node->declare_parameter<std::string>(
      "vehicle_local_position_topic", "/fmu/out/vehicle_local_position_v1");

    detections_sub_ = node->create_subscription<vision_msgs::msg::Detection2DArray>(
      "/detections", rclcpp::QoS(10),
      [this](const vision_msgs::msg::Detection2DArray::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_detections_ = *msg;
        has_detections_ = true;
      });

    status_sub_ = node->create_subscription<px4_msgs::msg::VehicleStatus>(
      vehicle_status_topic, rclcpp::SensorDataQoS(),
      [this](const px4_msgs::msg::VehicleStatus::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_status_ = *msg;
        has_status_ = true;
      });

    local_position_sub_ = node->create_subscription<px4_msgs::msg::VehicleLocalPosition>(
      local_position_topic, rclcpp::SensorDataQoS(),
      [this](const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_local_position_ = *msg;
        has_local_position_ = true;
      });
  }

  bool target_acquired(const std::string & target_class_id, double min_score) const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_detections_) {
      return false;
    }
    for (const auto & det : latest_detections_.detections) {
      for (const auto & result : det.results) {
        if (result.hypothesis.class_id == target_class_id &&
          result.hypothesis.score >= min_score)
        {
          return true;
        }
      }
    }
    return false;
  }

  bool detections_stable(
    const std::string & target_class_id,
    double min_score,
    int required_consecutive) const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_detections_) {
      stable_count_ = 0;
      return false;
    }
    bool found = false;
    for (const auto & det : latest_detections_.detections) {
      for (const auto & result : det.results) {
        if (result.hypothesis.class_id == target_class_id &&
          result.hypothesis.score >= min_score)
        {
          found = true;
          break;
        }
      }
      if (found) {
        break;
      }
    }
    if (found) {
      ++stable_count_;
    } else {
      stable_count_ = 0;
    }
    return stable_count_ >= required_consecutive;
  }

  void reset_track_stability() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stable_count_ = 0;
  }

  bool rc_override_not_active() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_status_) {
      return true;
    }
    if (latest_status_.nav_state ==
      px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD)
    {
      ever_was_offboard_ = true;
      return true;
    }
    // Only fail after Offboard was active and PX4 left it (RC override / mode change).
    return !ever_was_offboard_;
  }

  bool is_armed() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_status_) {
      return false;
    }
    return latest_status_.arming_state ==
           px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED;
  }

  bool is_offboard() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_status_) {
      return false;
    }
    return latest_status_.nav_state ==
           px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD;
  }

  bool pre_flight_checks_pass() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return has_status_ && latest_status_.pre_flight_checks_pass;
  }

  bool gcs_connection_lost() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return has_status_ && latest_status_.gcs_connection_lost;
  }

  bool has_altitude() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return has_local_position_ && latest_local_position_.z_valid;
  }

  double altitude_m() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return ned_z_to_altitude_m(latest_local_position_.z);
  }

  bool has_horizontal_position() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return has_local_position_ && latest_local_position_.xy_valid;
  }

  double position_x_m() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_local_position_.x;
  }

  double position_y_m() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_local_position_.y;
  }

  bool has_heading() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return has_local_position_ && latest_local_position_.heading_good_for_control;
  }

  double heading_rad() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_local_position_.heading;
  }

  rclcpp::Node::SharedPtr node;

private:
  mutable std::mutex mutex_;
  vision_msgs::msg::Detection2DArray latest_detections_;
  px4_msgs::msg::VehicleStatus latest_status_;
  px4_msgs::msg::VehicleLocalPosition latest_local_position_;
  bool has_detections_{false};
  bool has_status_{false};
  bool has_local_position_{false};
  mutable bool ever_was_offboard_{false};
  mutable int stable_count_{0};

  rclcpp::Subscription<vision_msgs::msg::Detection2DArray>::SharedPtr detections_sub_;
  rclcpp::Subscription<px4_msgs::msg::VehicleStatus>::SharedPtr status_sub_;
  rclcpp::Subscription<px4_msgs::msg::VehicleLocalPosition>::SharedPtr local_position_sub_;
};

}  // namespace dondron_state_machine

#endif  // DONDRON_STATE_MACHINE__MISSION_CONTEXT_HPP_
