#ifndef DONDRON_STATE_MACHINE__ALTITUDE_CONTROL_HPP_
#define DONDRON_STATE_MACHINE__ALTITUDE_CONTROL_HPP_

#include <algorithm>
#include <cmath>

namespace dondron_state_machine
{

/// Convert PX4 NED down-axis position to height above local origin (positive up).
inline double ned_z_to_altitude_m(double z_ned)
{
  return -z_ned;
}

enum class ClimbDecision
{
  Success,
  Running,
};

struct ClimbToAltitudeEval
{
  ClimbDecision decision{ClimbDecision::Running};
  double vz_ned{0.0};
};

/// Closed-loop climb/hold decision for ClimbToAltitude BT node.
///
/// Success when current altitude is at or above (target - tolerance), which
/// prevents compounding climbs on BT relaunch while already airborne.
/// Below that band, command NED vz (negative = climb up).
inline ClimbToAltitudeEval evaluate_climb_to_altitude(
  double current_altitude_m,
  double target_altitude_m,
  double tolerance_m,
  double climb_rate_mps)
{
  const double lower_band_m = target_altitude_m - tolerance_m;
  if (current_altitude_m >= lower_band_m) {
    return {ClimbDecision::Success, 0.0};
  }

  const double rate = std::max(climb_rate_mps, 0.1);
  return {ClimbDecision::Running, -rate};
}

}  // namespace dondron_state_machine

#endif  // DONDRON_STATE_MACHINE__ALTITUDE_CONTROL_HPP_
