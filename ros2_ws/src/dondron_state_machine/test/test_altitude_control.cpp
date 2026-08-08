#include <gtest/gtest.h>

#include "dondron_state_machine/altitude_control.hpp"

namespace dondron_state_machine
{
namespace
{

TEST(AltitudeControlTest, NedZToAltitude)
{
  EXPECT_NEAR(ned_z_to_altitude_m(0.0), 0.0, 1e-9);
  EXPECT_NEAR(ned_z_to_altitude_m(-3.0), 3.0, 1e-9);
  EXPECT_NEAR(ned_z_to_altitude_m(2.5), -2.5, 1e-9);
}

TEST(AltitudeControlTest, SuccessWithinToleranceBand)
{
  const auto eval = evaluate_climb_to_altitude(2.8, 3.0, 0.3, 1.0);
  EXPECT_EQ(eval.decision, ClimbDecision::Success);
  EXPECT_NEAR(eval.vz_ned, 0.0, 1e-9);
}

TEST(AltitudeControlTest, SuccessWhenAlreadyAboveTarget)
{
  const auto eval = evaluate_climb_to_altitude(6.0, 3.0, 0.3, 1.0);
  EXPECT_EQ(eval.decision, ClimbDecision::Success);
  EXPECT_NEAR(eval.vz_ned, 0.0, 1e-9);
}

TEST(AltitudeControlTest, ClimbWhenBelowTarget)
{
  const auto eval = evaluate_climb_to_altitude(1.0, 3.0, 0.3, 1.0);
  EXPECT_EQ(eval.decision, ClimbDecision::Running);
  EXPECT_NEAR(eval.vz_ned, -1.0, 1e-9);
}

TEST(AltitudeControlTest, ClimbJustBelowLowerBand)
{
  const auto eval = evaluate_climb_to_altitude(2.69, 3.0, 0.3, 0.8);
  EXPECT_EQ(eval.decision, ClimbDecision::Running);
  EXPECT_NEAR(eval.vz_ned, -0.8, 1e-9);
}

}  // namespace
}  // namespace dondron_state_machine

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
