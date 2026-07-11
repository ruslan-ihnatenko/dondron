#include <gtest/gtest.h>

#include "dondron_perception/monocular_range.hpp"

namespace dondron_perception
{
namespace
{

TEST(MonocularRangeTest, KnownIntrinsicsAndBbox)
{
  // fx=554.25, target 0.30 m wide, bbox 80 px → range ≈ 2.078 m
  const double range = monocular_range_m(554.25, 0.30, 80.0);
  EXPECT_NEAR(range, 2.0784375, 1e-4);
}

TEST(MonocularRangeTest, InvalidBboxReturnsZero)
{
  EXPECT_DOUBLE_EQ(monocular_range_m(554.25, 0.30, 0.0), 0.0);
  EXPECT_DOUBLE_EQ(monocular_range_m(554.25, 0.30, -10.0), 0.0);
}

TEST(MonocularRangeTest, InvalidFocalLengthReturnsZero)
{
  EXPECT_DOUBLE_EQ(monocular_range_m(0.0, 0.30, 80.0), 0.0);
}

TEST(MonocularRangeTest, InvalidTargetWidthReturnsZero)
{
  EXPECT_DOUBLE_EQ(monocular_range_m(554.25, 0.0, 80.0), 0.0);
}

}  // namespace
}  // namespace dondron_perception
