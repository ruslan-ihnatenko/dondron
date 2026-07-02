#ifndef DONDRON_FLIGHT_API__FRAME_TRANSFORM_HPP_
#define DONDRON_FLIGHT_API__FRAME_TRANSFORM_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace dondron_flight_api
{

inline std::array<float, 3> body_frd_to_ned(
  float vx_body, float vy_body, float vz_body,
  const std::array<float, 4> & q)
{
  // Hamilton quaternion q(w,x,y,z): rotate body FRD vector to NED
  const float w = q[0];
  const float x = q[1];
  const float y = q[2];
  const float z = q[3];

  const float ix = w * vx_body + y * vz_body - z * vy_body;
  const float iy = w * vy_body + z * vx_body - x * vz_body;
  const float iz = w * vz_body + x * vy_body - y * vx_body;
  const float iw = -x * vx_body - y * vy_body - z * vz_body;

  return {
    ix * w + iw * -x + iy * -z - iz * -y,
    iy * w + iw * -y + iz * -x - ix * -z,
    iz * w + iw * -z + ix * -y - iy * -x
  };
}

inline bool is_body_frd_frame(const std::string & frame_id)
{
  return frame_id == "body_frd" || frame_id == "base_link_frd";
}

inline bool is_ned_frame(const std::string & frame_id)
{
  return frame_id == "ned" || frame_id == "map" || frame_id.empty();
}

}  // namespace dondron_flight_api

#endif  // DONDRON_FLIGHT_API__FRAME_TRANSFORM_HPP_
