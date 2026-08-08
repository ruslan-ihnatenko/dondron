#include <gtest/gtest.h>

#include "dondron_state_machine/orbit_control.hpp"

namespace dondron_state_machine
{
namespace
{

TEST(OrbitControlTest, EastPointMovesNorthOnCcwOrbit)
{
  // NED: x=north, y=east. Point 7 m north of origin -> tangent is east.
  const auto vel = compute_orbit_tangent_velocity_ned(7.0, 0.0, 0.0, 0.0, 1.0);
  EXPECT_NEAR(vel.vx, 0.0, 1e-6);
  EXPECT_NEAR(vel.vy, 1.0, 1e-6);
}

TEST(OrbitControlTest, NorthPointMovesWestOnCcwOrbit)
{
  // Point 7 m east of origin -> tangent is south (negative north).
  const auto vel = compute_orbit_tangent_velocity_ned(0.0, 7.0, 0.0, 0.0, 2.0);
  EXPECT_NEAR(vel.vx, -2.0, 1e-6);
  EXPECT_NEAR(vel.vy, 0.0, 1e-6);
}

TEST(OrbitControlTest, NearCenterDefaultsNorthExpand)
{
  const auto vel = compute_orbit_tangent_velocity_ned(0.1, 0.0, 0.0, 0.0, 0.8);
  EXPECT_NEAR(vel.vx, 0.8, 1e-6);
  EXPECT_NEAR(vel.vy, 0.0, 1e-6);
}

TEST(OrbitControlTest, ExpandPhaseFliesOutward)
{
  const auto sp = compute_orbit_setpoint_ned(2.0, 0.0, 0.0, 0.0, 10.0, 1.0, 0.0, 1.0);
  EXPECT_EQ(sp.phase, OrbitPhase::Expand);
  EXPECT_NEAR(sp.vx, 1.0, 1e-6);
  EXPECT_NEAR(sp.vy, 0.0, 1e-6);
}

TEST(OrbitControlTest, OrbitPhaseYawTowardCenter)
{
  const auto sp = compute_orbit_setpoint_ned(10.0, 0.0, 0.0, 0.0, 10.0, 1.0, 0.0, 1.0);
  EXPECT_EQ(sp.phase, OrbitPhase::Orbit);
  EXPECT_NEAR(sp.vx, 0.0, 1e-6);
  EXPECT_NEAR(sp.vy, 1.0, 1e-6);
  EXPECT_GT(sp.yawspeed, 0.0);  // north of center: turn south (positive yaw from heading 0)
}

TEST(OrbitControlTest, WrapPi)
{
  EXPECT_NEAR(wrap_pi(3.5), 3.5 - 2.0 * M_PI, 1e-6);
}

}  // namespace
}  // namespace dondron_state_machine

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
