#ifndef DONDRON_PERCEPTION__SIMPLE_BLOB_DETECTOR_HPP_
#define DONDRON_PERCEPTION__SIMPLE_BLOB_DETECTOR_HPP_

#include <algorithm>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

namespace dondron_perception
{

struct BlobDetection
{
  double center_x{0.0};
  double center_y{0.0};
  double size_x{0.0};
  double size_y{0.0};
  double score{0.0};
};

/// CPU placeholder detector for Mac/CI — finds the largest bright blob.
/// Replaced by YOLO on Main PC; sufficient for synthetic-image contract tests.
inline bool detect_largest_bright_blob(
  const cv::Mat & gray, int brightness_threshold, BlobDetection & out)
{
  if (gray.empty()) {
    return false;
  }

  cv::Mat mask;
  cv::threshold(gray, mask, brightness_threshold, 255, cv::THRESH_BINARY);

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

  double best_area = 0.0;
  cv::Rect best_rect;
  for (const auto & contour : contours) {
    const auto rect = cv::boundingRect(contour);
    const double area = static_cast<double>(rect.area());
    if (area > best_area) {
      best_area = area;
      best_rect = rect;
    }
  }

  if (best_area <= 0.0) {
    return false;
  }

  cv::Mat roi = gray(best_rect);
  const double mean_val = cv::mean(roi)[0];
  const double score = std::min(1.0, mean_val / 255.0);

  out.center_x = best_rect.x + best_rect.width * 0.5;
  out.center_y = best_rect.y + best_rect.height * 0.5;
  out.size_x = static_cast<double>(best_rect.width);
  out.size_y = static_cast<double>(best_rect.height);
  out.score = score;
  return true;
}

}  // namespace dondron_perception

#endif  // DONDRON_PERCEPTION__SIMPLE_BLOB_DETECTOR_HPP_
