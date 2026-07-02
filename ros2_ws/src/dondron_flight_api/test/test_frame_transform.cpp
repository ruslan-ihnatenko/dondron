#include <cmath>
#include <gtest/gtest.h>

#include "dondron_flight_api/frame_transform.hpp"

namespace dondron_flight_api
{
namespace
{

TEST(FrameTransformTest, IdentityQuaternionMapsBodyXToNorth)
{
  const std::array<float, 4> q{1.0f, 0.0f, 0.0f, 0.0f};
  const auto ned = body_frd_to_ned(1.0f, 0.0f, 0.0f, q);
  EXPECT_NEAR(ned[0], 1.0f, 1e-5f);
  EXPECT_NEAR(ned[1], 0.0f, 1e-5f);
  EXPECT_NEAR(ned[2], 0.0f, 1e-5f);
}

TEST(FrameTransformTest, ZeroVelocityStaysZero)
{
  const std::array<float, 4> q{0.7071f, 0.0f, 0.0f, 0.7071f};
  const auto ned = body_frd_to_ned(0.0f, 0.0f, 0.0f, q);
  EXPECT_NEAR(ned[0], 0.0f, 1e-4f);
  EXPECT_NEAR(ned[1], 0.0f, 1e-4f);
  EXPECT_NEAR(ned[2], 0.0f, 1e-4f);
}

TEST(FrameTransformTest, FrameIdHelpers)
{
  EXPECT_TRUE(is_body_frd_frame("body_frd"));
  EXPECT_TRUE(is_body_frd_frame("base_link_frd"));
  EXPECT_FALSE(is_body_frd_frame("ned"));

  EXPECT_TRUE(is_ned_frame("ned"));
  EXPECT_TRUE(is_ned_frame("map"));
  EXPECT_TRUE(is_ned_frame(""));
  EXPECT_FALSE(is_ned_frame("body_frd"));
}

}  // namespace
}  // namespace dondron_flight_api
