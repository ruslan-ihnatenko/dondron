#ifndef DONDRON_PERCEPTION__MONOCULAR_RANGE_HPP_
#define DONDRON_PERCEPTION__MONOCULAR_RANGE_HPP_

namespace dondron_perception
{

/// Monocular range from bbox width (Phase 1 contract — see package README).
/// Returns 0.0 when bbox_width_px <= 0 (invalid).
inline double monocular_range_m(
  double focal_length_px, double target_width_m, double bbox_width_px)
{
  if (bbox_width_px <= 0.0 || focal_length_px <= 0.0 || target_width_m <= 0.0) {
    return 0.0;
  }
  return (focal_length_px * target_width_m) / bbox_width_px;
}

}  // namespace dondron_perception

#endif  // DONDRON_PERCEPTION__MONOCULAR_RANGE_HPP_
