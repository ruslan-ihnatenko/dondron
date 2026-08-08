#ifndef DONDRON_STATE_MACHINE__ORBIT_CONTROL_HPP_
#define DONDRON_STATE_MACHINE__ORBIT_CONTROL_HPP_

#include <algorithm>
#include <cmath>

namespace dondron_state_machine
{

inline double wrap_pi(double angle_rad)
{
  while (angle_rad > M_PI) {
    angle_rad -= 2.0 * M_PI;
  }
  while (angle_rad < -M_PI) {
    angle_rad += 2.0 * M_PI;
  }
  return angle_rad;
}

struct OrbitVelocityNed
{
  double vx{0.0};
  double vy{0.0};
};

enum class OrbitPhase
{
  Expand,
  Orbit,
};

struct OrbitSetpointNed
{
  OrbitPhase phase{OrbitPhase::Orbit};
  double vx{0.0};
  double vy{0.0};
  double yawspeed{0.0};
};

/// CCW horizontal orbit tangent velocity in NED (north/east), m/s.
inline OrbitVelocityNed compute_orbit_tangent_velocity_ned(
  double x_ned,
  double y_ned,
  double center_x_ned,
  double center_y_ned,
  double tangential_speed_mps,
  double min_radius_m = 0.5)
{
  const double dx = x_ned - center_x_ned;
  const double dy = y_ned - center_y_ned;
  const double radius_m = std::hypot(dx, dy);
  const double speed = std::max(tangential_speed_mps, 0.1);

  if (radius_m < min_radius_m) {
    return {speed, 0.0};
  }

  const double scale = speed / radius_m;
  return {-dy * scale, dx * scale};
}

/// Orbit setpoint: expand to target radius, then CCW tangent + yaw nose toward center.
inline OrbitSetpointNed compute_orbit_setpoint_ned(
  double x_ned,
  double y_ned,
  double center_x_ned,
  double center_y_ned,
  double target_radius_m,
  double tangential_speed_mps,
  double current_heading_rad,
  double radius_tolerance_m = 1.0,
  double yaw_gain = 1.5,
  double max_yaw_rate_radps = 0.6)
{
  const double dx = x_ned - center_x_ned;
  const double dy = y_ned - center_y_ned;
  const double radius_m = std::hypot(dx, dy);
  const double speed = std::max(tangential_speed_mps, 0.1);

  if (radius_m < target_radius_m - radius_tolerance_m) {
    if (radius_m < 0.5) {
      return {OrbitPhase::Expand, speed, 0.0, 0.0};
    }
    const double scale = speed / radius_m;
    return {OrbitPhase::Expand, dx * scale, dy * scale, 0.0};
  }

  const auto tang = compute_orbit_tangent_velocity_ned(
    x_ned, y_ned, center_x_ned, center_y_ned, tangential_speed_mps);
  const double desired_heading = std::atan2(center_y_ned - y_ned, center_x_ned - x_ned);
  const double yaw_error = wrap_pi(desired_heading - current_heading_rad);
  const double yawspeed = std::clamp(
    yaw_gain * yaw_error, -max_yaw_rate_radps, max_yaw_rate_radps);

  return {OrbitPhase::Orbit, tang.vx, tang.vy, yawspeed};
}

}  // namespace dondron_state_machine

#endif  // DONDRON_STATE_MACHINE__ORBIT_CONTROL_HPP_
