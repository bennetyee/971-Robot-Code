#include "y2022/vision/target_estimator.h"

#include "absl/strings/str_format.h"
#include "aos/time/time.h"
#include "ceres/ceres.h"
#include "frc971/control_loops/quaternion_utils.h"
#include "frc971/vision/geometry.h"
#include "opencv2/core/core.hpp"
#include "opencv2/core/eigen.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "y2022/constants.h"

DEFINE_bool(freeze_roll, false, "If true, don't solve for roll");
DEFINE_bool(freeze_pitch, false, "If true, don't solve for pitch");
DEFINE_bool(freeze_yaw, false, "If true, don't solve for yaw");
DEFINE_bool(freeze_camera_height, true,
            "If true, don't solve for camera height");
DEFINE_bool(freeze_angle_to_camera, true,
            "If true, don't solve for polar angle to camera");

DEFINE_uint64(max_solver_iterations, 200,
              "Maximum number of iterations for the ceres solver");
DEFINE_bool(solver_output, false,
            "If true, log the solver progress and results");
DEFINE_bool(draw_projected_hub, true,
            "If true, draw the projected hub when drawing an estimate");

namespace y2022::vision {

namespace {

constexpr size_t kNumPiecesOfTape = 16;
// Width and height of a piece of reflective tape
constexpr double kTapePieceWidth = 0.13;
constexpr double kTapePieceHeight = 0.05;
// Height of the center of the tape (m)
constexpr double kTapeCenterHeight = 2.58 + (kTapePieceHeight / 2);
// Horizontal distance from tape to center of hub (m)
constexpr double kUpperHubRadius = 1.36 / 2;

std::vector<cv::Point3d> ComputeTapePoints() {
  std::vector<cv::Point3d> tape_points;

  constexpr size_t kNumVisiblePiecesOfTape = 5;
  for (size_t i = 0; i < kNumVisiblePiecesOfTape; i++) {
    // The center piece of tape is at 0 rad, so the angle indices are offset
    // by the number of pieces of tape on each side of it
    const double theta_index =
        static_cast<double>(i) - ((kNumVisiblePiecesOfTape - 1) / 2);
    // The polar angle is a multiple of the angle between tape centers
    double theta = theta_index * ((2.0 * M_PI) / kNumPiecesOfTape);
    tape_points.emplace_back(kUpperHubRadius * std::cos(theta),
                             kUpperHubRadius * std::sin(theta),
                             kTapeCenterHeight);
  }

  return tape_points;
}

std::array<cv::Point3d, 4> ComputeMiddleTapePiecePoints() {
  std::array<cv::Point3d, 4> tape_piece_points;

  // Angle that each piece of tape occupies on the hub
  constexpr double kTapePieceAngle =
      (kTapePieceWidth / (2.0 * M_PI * kUpperHubRadius)) * (2.0 * M_PI);

  constexpr double kThetaTapeLeft = -kTapePieceAngle / 2.0;
  constexpr double kThetaTapeRight = kTapePieceAngle / 2.0;

  constexpr double kTapeTopHeight =
      kTapeCenterHeight + (kTapePieceHeight / 2.0);
  constexpr double kTapeBottomHeight =
      kTapeCenterHeight - (kTapePieceHeight / 2.0);

  tape_piece_points[0] = {kUpperHubRadius * std::cos(kThetaTapeLeft),
                          kUpperHubRadius * std::sin(kThetaTapeLeft),
                          kTapeTopHeight};
  tape_piece_points[1] = {kUpperHubRadius * std::cos(kThetaTapeRight),
                          kUpperHubRadius * std::sin(kThetaTapeRight),
                          kTapeTopHeight};

  tape_piece_points[2] = {kUpperHubRadius * std::cos(kThetaTapeRight),
                          kUpperHubRadius * std::sin(kThetaTapeRight),
                          kTapeBottomHeight};
  tape_piece_points[3] = {kUpperHubRadius * std::cos(kThetaTapeLeft),
                          kUpperHubRadius * std::sin(kThetaTapeLeft),
                          kTapeBottomHeight};

  return tape_piece_points;
}

}  // namespace

const std::vector<cv::Point3d> TargetEstimator::kTapePoints =
    ComputeTapePoints();
const std::array<cv::Point3d, 4> TargetEstimator::kMiddleTapePiecePoints =
    ComputeMiddleTapePiecePoints();

namespace {
constexpr double kDefaultDistance = 3.0;
constexpr double kDefaultYaw = M_PI;
constexpr double kDefaultAngleToCamera = 0.0;
}  // namespace

TargetEstimator::TargetEstimator(cv::Mat intrinsics, cv::Mat extrinsics)
    : blob_stats_(),
      middle_blob_index_(0),
      max_blob_area_(0.0),
      image_(std::nullopt),
      roll_(0.0),
      pitch_(0.0),
      yaw_(kDefaultYaw),
      distance_(kDefaultDistance),
      angle_to_camera_(kDefaultAngleToCamera),
      // Seed camera height
      camera_height_(extrinsics.at<double>(2, 3) +
                     constants::Values::kImuHeight()) {
  cv::cv2eigen(intrinsics, intrinsics_);
  cv::cv2eigen(extrinsics, extrinsics_);
}

namespace {
void SetBoundsOrFreeze(double *param, bool freeze, double min, double max,
                       ceres::Problem *problem) {
  if (freeze) {
    problem->SetParameterBlockConstant(param);
  } else {
    problem->SetParameterLowerBound(param, 0, min);
    problem->SetParameterUpperBound(param, 0, max);
  }
}

// With X, Y, Z being hub axes and x, y, z being camera axes,
// x = -Y, y = -Z, z = X
const Eigen::Matrix3d kHubToCameraAxes =
    (Eigen::Matrix3d() << 0.0, -1.0, 0.0, 0.0, 0.0, -1.0, 1.0, 0.0, 0.0)
        .finished();

}  // namespace

void TargetEstimator::Solve(
    const std::vector<BlobDetector::BlobStats> &blob_stats,
    std::optional<cv::Mat> image) {
  auto start = aos::monotonic_clock::now();

  blob_stats_ = blob_stats;
  image_ = image;

  // Do nothing if no blobs were detected
  if (blob_stats_.size() == 0) {
    confidence_ = 0.0;
    return;
  }

  CHECK_GE(blob_stats_.size(), 3) << "Expected at least 3 blobs";

  const auto circle = frc971::vision::Circle::Fit({blob_stats_[0].centroid,
                                                   blob_stats_[1].centroid,
                                                   blob_stats_[2].centroid});
  CHECK(circle.has_value());

  max_blob_area_ = 0.0;

  // Find the middle blob, which is the one with the angle closest to the
  // average
  double theta_avg = 0.0;
  for (const auto &stats : blob_stats_) {
    theta_avg += circle->AngleOf(stats.centroid);

    if (stats.area > max_blob_area_) {
      max_blob_area_ = stats.area;
    }
  }
  theta_avg /= blob_stats_.size();

  double min_diff = std::numeric_limits<double>::infinity();
  for (auto it = blob_stats_.begin(); it < blob_stats_.end(); it++) {
    const double diff = std::abs(circle->AngleOf(it->centroid) - theta_avg);
    if (diff < min_diff) {
      min_diff = diff;
      middle_blob_index_ = it - blob_stats_.begin();
    }
  }

  ceres::Problem problem;

  // x and y differences between projected centroids and blob centroids, as well
  // as width and height differences between middle projected piece and the
  // detected blob
  const size_t num_residuals = (blob_stats_.size() * 2) + 2;

  // Set up the only cost function (also known as residual). This uses
  // auto-differentiation to obtain the derivative (jacobian).
  ceres::CostFunction *cost_function =
      new ceres::AutoDiffCostFunction<TargetEstimator, ceres::DYNAMIC, 1, 1, 1,
                                      1, 1, 1>(this, num_residuals,
                                               ceres::DO_NOT_TAKE_OWNERSHIP);

  // TODO(milind): add loss function when we get more noisy data
  problem.AddResidualBlock(cost_function, new ceres::HuberLoss(2.0), &roll_,
                           &pitch_, &yaw_, &distance_, &angle_to_camera_,
                           &camera_height_);

  // Compute the estimated rotation of the camera using the robot rotation.
  const Eigen::Matrix3d extrinsics_rot =
      Eigen::Affine3d(extrinsics_).rotation() * kHubToCameraAxes;
  // asin returns a pitch in [-pi/2, pi/2] so this will be the correct euler
  // angles.
  const double pitch_seed = -std::asin(extrinsics_rot(2, 0));
  const double roll_seed =
      std::atan2(extrinsics_rot(2, 1) / std::cos(pitch_seed),
                 extrinsics_rot(2, 2) / std::cos(pitch_seed));

  // TODO(milind): seed with localizer output as well

  // If we didn't solve well last time, seed everything at the defaults so we
  // don't get stuck in a bad state.
  // Copied from localizer.cc
  constexpr double kMinConfidence = 0.75;
  if (confidence_ < kMinConfidence) {
    roll_ = roll_seed;
    pitch_ = pitch_seed;
    yaw_ = kDefaultYaw;
    distance_ = kDefaultDistance;
    angle_to_camera_ = kDefaultAngleToCamera;
    camera_height_ = extrinsics_(2, 3) + constants::Values::kImuHeight();
  }

  // Constrain the rotation to be around the localizer's, otherwise there can be
  // multiple solutions. There shouldn't be too much roll or pitch
  if (FLAGS_freeze_roll) {
    roll_ = roll_seed;
  }
  constexpr double kMaxRollDelta = 0.1;
  SetBoundsOrFreeze(&roll_, FLAGS_freeze_roll, roll_seed - kMaxRollDelta,
                    roll_seed + kMaxRollDelta, &problem);

  if (FLAGS_freeze_pitch) {
    pitch_ = pitch_seed;
  }
  constexpr double kMaxPitchDelta = 0.15;
  SetBoundsOrFreeze(&pitch_, FLAGS_freeze_pitch, pitch_seed - kMaxPitchDelta,
                    pitch_seed + kMaxPitchDelta, &problem);
  // Constrain the yaw to where the target would be visible
  constexpr double kMaxYawDelta = M_PI / 4.0;
  SetBoundsOrFreeze(&yaw_, FLAGS_freeze_yaw, M_PI - kMaxYawDelta,
                    M_PI + kMaxYawDelta, &problem);

  constexpr double kMaxHeightDelta = 0.1;
  SetBoundsOrFreeze(&camera_height_, FLAGS_freeze_camera_height,
                    camera_height_ - kMaxHeightDelta,
                    camera_height_ + kMaxHeightDelta, &problem);

  // Distances shouldn't be too close to the target or too far
  constexpr double kMinDistance = 1.0;
  constexpr double kMaxDistance = 10.0;
  SetBoundsOrFreeze(&distance_, false, kMinDistance, kMaxDistance, &problem);

  // Keep the angle between +/- half of the angle between piece of tape
  constexpr double kMaxAngle = ((2.0 * M_PI) / kNumPiecesOfTape) / 2.0;
  SetBoundsOrFreeze(&angle_to_camera_, FLAGS_freeze_angle_to_camera, -kMaxAngle,
                    kMaxAngle, &problem);

  ceres::Solver::Options options;
  options.minimizer_progress_to_stdout = FLAGS_solver_output;
  options.gradient_tolerance = 1e-12;
  options.function_tolerance = 1e-16;
  options.parameter_tolerance = 1e-12;
  options.max_num_iterations = FLAGS_max_solver_iterations;
  ceres::Solver::Summary summary;
  ceres::Solve(options, &problem, &summary);

  auto end = aos::monotonic_clock::now();
  VLOG(1) << "Target estimation elapsed time: "
          << std::chrono::duration<double, std::milli>(end - start).count()
          << " ms";

  // For computing the confidence, find the standard deviation in pixels.
  std::vector<double> residual(num_residuals);
  (*this)(&roll_, &pitch_, &yaw_, &distance_, &angle_to_camera_,
          &camera_height_, residual.data());
  double std_dev = 0.0;
  for (auto it = residual.begin(); it < residual.end() - 2; it++) {
    std_dev += std::pow(*it, 2);
  }
  std_dev /= num_residuals - 2;
  std_dev = std::sqrt(std_dev);

  // Use a sigmoid to convert the deviation into a confidence for the
  // localizer. Fit a sigmoid to the points of (0, 1) and two other
  // reasonable deviation-confidence combinations using
  // https://www.desmos.com/calculator/ha6fh9yw44
  constexpr double kSigmoidCapacity = 1.065;
  // Stretch the sigmoid out correctly.
  // Currently, good estimates have deviations of 1 or less pixels.
  constexpr double kSigmoidScalar = 0.06496;
  constexpr double kSigmoidGrowthRate = -0.6221;
  confidence_ =
      kSigmoidCapacity /
      (1.0 + kSigmoidScalar * std::exp(-kSigmoidGrowthRate * std_dev));

  if (FLAGS_solver_output) {
    LOG(INFO) << summary.FullReport();

    LOG(INFO) << "roll: " << roll_;
    LOG(INFO) << "pitch: " << pitch_;
    LOG(INFO) << "yaw: " << yaw_;
    LOG(INFO) << "angle to target (based on yaw): " << angle_to_target();
    LOG(INFO) << "angle to camera (polar): " << angle_to_camera_;
    LOG(INFO) << "distance (polar): " << distance_;
    LOG(INFO) << "camera height: " << camera_height_;
    LOG(INFO) << "standard deviation (px): " << std_dev;
    LOG(INFO) << "confidence: " << confidence_;
  }
}

namespace {

// Hacks to extract a double from a scalar, which is either a ceres jet or a
// double. Only used for debugging and displaying.
template <typename S>
double ScalarToDouble(S s) {
  const double *ptr = reinterpret_cast<double *>(&s);
  return *ptr;
}

template <typename S>
cv::Point2d ScalarPointToDouble(cv::Point_<S> p) {
  return cv::Point2d(ScalarToDouble(p.x), ScalarToDouble(p.y));
}

}  // namespace

template <typename S>
bool TargetEstimator::operator()(const S *const roll, const S *const pitch,
                                 const S *const yaw, const S *const distance,
                                 const S *const theta,
                                 const S *const camera_height,
                                 S *residual) const {
  const auto H_hub_camera = ComputeHubCameraTransform(
      *roll, *pitch, *yaw, *distance, *theta, *camera_height);

  // Project tape points
  std::vector<cv::Point_<S>> tape_points_proj;
  for (cv::Point3d tape_point_hub : kTapePoints) {
    tape_points_proj.emplace_back(ProjectToImage(tape_point_hub, H_hub_camera));
    VLOG(2) << "Projected tape point: "
            << ScalarPointToDouble(
                   tape_points_proj[tape_points_proj.size() - 1]);
  }

  // Find the rectangle bounding the projected piece of tape
  std::array<cv::Point_<S>, 4> middle_tape_piece_points_proj;
  for (auto tape_piece_it = kMiddleTapePiecePoints.begin();
       tape_piece_it < kMiddleTapePiecePoints.end(); tape_piece_it++) {
    middle_tape_piece_points_proj[tape_piece_it -
                                  kMiddleTapePiecePoints.begin()] =
        ProjectToImage(*tape_piece_it, H_hub_camera);
  }

  // Now, find the closest tape for each blob.
  // We don't normally see tape without matching blobs in the center.  So we
  // want to compress any gaps in the matched tape blobs.  This makes it so it
  // doesn't want to make the goal super small and skip tape blobs.  The
  // resulting accuracy is then pretty good.

  // Mapping from tape index to blob index.
  std::vector<std::pair<size_t, size_t>> tape_indices;
  for (size_t i = 0; i < blob_stats_.size(); i++) {
    tape_indices.emplace_back(ClosestTape(i, tape_points_proj), i);
    VLOG(2) << "Tape indices were " << tape_indices.back().first;
  }

  std::sort(
      tape_indices.begin(), tape_indices.end(),
      [](const std::pair<size_t, size_t> &a,
         const std::pair<size_t, size_t> &b) { return a.first < b.first; });

  std::optional<size_t> middle_tape_index = std::nullopt;
  for (size_t i = 0; i < tape_indices.size(); ++i) {
    if (tape_indices[i].second == middle_blob_index_) {
      middle_tape_index = i;
    }
  }
  CHECK(middle_tape_index.has_value()) << "Failed to find middle tape";

  if (VLOG_IS_ON(2)) {
    LOG(INFO) << "Middle tape is " << *middle_tape_index << ", blob "
              << middle_blob_index_;
    for (size_t i = 0; i < tape_indices.size(); ++i) {
      const auto distance = DistanceFromTapeIndex(
          tape_indices[i].second, tape_indices[i].first, tape_points_proj);
      LOG(INFO) << "Blob index " << tape_indices[i].second << " maps to "
                << tape_indices[i].first << " distance " << distance.x << " "
                << distance.y;
    }
  }

  {
    size_t offset = 0;
    for (size_t i = *middle_tape_index + 1; i < tape_indices.size(); ++i) {
      tape_indices[i].first -= offset;

      if (tape_indices[i].first > tape_indices[i - 1].first + 1) {
        offset += tape_indices[i].first - (tape_indices[i - 1].first + 1);
        VLOG(2) << "Offset now " << offset;
        tape_indices[i].first = tape_indices[i - 1].first + 1;
      }
    }
  }

  if (VLOG_IS_ON(2)) {
    LOG(INFO) << "Middle tape is " << *middle_tape_index << ", blob "
              << middle_blob_index_;
    for (size_t i = 0; i < tape_indices.size(); ++i) {
      const auto distance = DistanceFromTapeIndex(
          tape_indices[i].second, tape_indices[i].first, tape_points_proj);
      LOG(INFO) << "Blob index " << tape_indices[i].second << " maps to "
                << tape_indices[i].first << " distance " << distance.x << " "
                << distance.y;
    }
  }

  {
    size_t offset = 0;
    for (size_t i = *middle_tape_index; i > 0; --i) {
      tape_indices[i - 1].first -= offset;

      if (tape_indices[i - 1].first + 1 < tape_indices[i].first) {
        VLOG(2) << "Too big a gap. " << tape_indices[i].first << " and "
                << tape_indices[i - 1].first;

        offset += tape_indices[i].first - (tape_indices[i - 1].first + 1);
        tape_indices[i - 1].first = tape_indices[i].first - 1;
        VLOG(2) << "Offset now " << offset;
      }
    }
  }

  if (VLOG_IS_ON(2)) {
    LOG(INFO) << "Middle tape is " << *middle_tape_index << ", blob "
              << middle_blob_index_;
    for (size_t i = 0; i < tape_indices.size(); ++i) {
      const auto distance = DistanceFromTapeIndex(
          tape_indices[i].second, tape_indices[i].first, tape_points_proj);
      LOG(INFO) << "Blob index " << tape_indices[i].second << " maps to "
                << tape_indices[i].first << " distance " << distance.x << " "
                << distance.y;
    }
  }

  for (size_t i = 0; i < tape_indices.size(); ++i) {
    const auto distance = DistanceFromTapeIndex(
        tape_indices[i].second, tape_indices[i].first, tape_points_proj);
    // Scale the distance based on the blob area: larger blobs have less noise.
    const S distance_scalar =
        S(blob_stats_[tape_indices[i].second].area / max_blob_area_);
    VLOG(2) << "Blob index " << tape_indices[i].second << " maps to "
            << tape_indices[i].first << " distance " << distance.x << " "
            << distance.y << " distance scalar "
            << ScalarToDouble(distance_scalar);

    // Set the residual to the (x, y) distance of the centroid from the
    // matched projected piece of tape
    residual[i * 2] = distance_scalar * distance.x;
    residual[(i * 2) + 1] = distance_scalar * distance.y;
  }

  // Penalize based on the difference between the size of the projected piece of
  // tape and that of the detected blobs.
  const S middle_tape_piece_width = ceres::hypot(
      middle_tape_piece_points_proj[2].x - middle_tape_piece_points_proj[3].x,
      middle_tape_piece_points_proj[2].y - middle_tape_piece_points_proj[3].y);
  const S middle_tape_piece_height = ceres::hypot(
      middle_tape_piece_points_proj[1].x - middle_tape_piece_points_proj[2].x,
      middle_tape_piece_points_proj[1].y - middle_tape_piece_points_proj[2].y);

  constexpr double kCenterBlobSizeScalar = 0.1;
  residual[blob_stats_.size() * 2] =
      kCenterBlobSizeScalar *
      (middle_tape_piece_width -
       static_cast<S>(blob_stats_[middle_blob_index_].size.width));
  residual[(blob_stats_.size() * 2) + 1] =
      kCenterBlobSizeScalar *
      (middle_tape_piece_height -
       static_cast<S>(blob_stats_[middle_blob_index_].size.height));

  if (image_.has_value()) {
    // Draw the current stage of the solving
    cv::Mat image = image_->clone();
    std::vector<cv::Point2d> tape_points_proj_double;
    for (auto point : tape_points_proj) {
      tape_points_proj_double.emplace_back(ScalarPointToDouble(point));
    }
    DrawProjectedHub(tape_points_proj_double, image);
    cv::imshow("image", image);
    cv::waitKey(10);
  }

  return true;
}

template <typename S>
Eigen::Transform<S, 3, Eigen::Affine>
TargetEstimator::ComputeHubCameraTransform(S roll, S pitch, S yaw, S distance,
                                           S theta, S camera_height) const {
  using Vector3s = Eigen::Matrix<S, 3, 1>;
  using Affine3s = Eigen::Transform<S, 3, Eigen::Affine>;

  Eigen::AngleAxis<S> roll_angle(roll, Vector3s::UnitX());
  Eigen::AngleAxis<S> pitch_angle(pitch, Vector3s::UnitY());
  Eigen::AngleAxis<S> yaw_angle(yaw, Vector3s::UnitZ());
  // Construct the rotation and translation of the camera in the hub's frame
  Eigen::Quaternion<S> R_camera_hub = yaw_angle * pitch_angle * roll_angle;
  Vector3s T_camera_hub(distance * ceres::cos(theta),
                        distance * ceres::sin(theta), camera_height);

  Affine3s H_camera_hub = Eigen::Translation<S, 3>(T_camera_hub) * R_camera_hub;
  Affine3s H_hub_camera = H_camera_hub.inverse();

  return H_hub_camera;
}

template <typename S>
cv::Point_<S> TargetEstimator::ProjectToImage(
    cv::Point3d tape_point_hub,
    const Eigen::Transform<S, 3, Eigen::Affine> &H_hub_camera) const {
  using Vector3s = Eigen::Matrix<S, 3, 1>;

  const Vector3s tape_point_hub_eigen =
      Vector3s(S(tape_point_hub.x), S(tape_point_hub.y), S(tape_point_hub.z));
  // Project the 3d tape point onto the image using the transformation and
  // intrinsics
  const Vector3s tape_point_proj =
      intrinsics_ * (kHubToCameraAxes * (H_hub_camera * tape_point_hub_eigen));

  // Normalize the projected point
  return {tape_point_proj.x() / tape_point_proj.z(),
          tape_point_proj.y() / tape_point_proj.z()};
}

namespace {
template <typename S>
cv::Point_<S> Distance(cv::Point p, cv::Point_<S> q) {
  return cv::Point_<S>(S(p.x) - q.x, S(p.y) - q.y);
}

template <typename S>
bool Less(cv::Point_<S> distance_1, cv::Point_<S> distance_2) {
  return (ceres::pow(distance_1.x, 2) + ceres::pow(distance_1.y, 2) <
          ceres::pow(distance_2.x, 2) + ceres::pow(distance_2.y, 2));
}
}  // namespace

template <typename S>
cv::Point_<S> TargetEstimator::DistanceFromTapeIndex(
    size_t blob_index, size_t tape_index,
    const std::vector<cv::Point_<S>> &tape_points) const {
  return Distance(blob_stats_[blob_index].centroid, tape_points[tape_index]);
}

template <typename S>
size_t TargetEstimator::ClosestTape(
    size_t blob_index, const std::vector<cv::Point_<S>> &tape_points) const {
  auto distance = cv::Point_<S>(std::numeric_limits<S>::infinity(),
                                std::numeric_limits<S>::infinity());
  std::optional<size_t> final_match = std::nullopt;
  if (blob_index == middle_blob_index_) {
    // Fix the middle blob so the solver can't go too far off
    final_match = tape_points.size() / 2;
    distance = DistanceFromTapeIndex(blob_index, *final_match, tape_points);
  } else {
    // Give the other blob_stats some freedom in case some are split into pieces
    for (auto it = tape_points.begin(); it < tape_points.end(); it++) {
      const size_t tape_index = std::distance(tape_points.begin(), it);
      const auto current_distance =
          DistanceFromTapeIndex(blob_index, tape_index, tape_points);
      if ((tape_index != (tape_points.size() / 2)) &&
          Less(current_distance, distance)) {
        final_match = tape_index;
        distance = current_distance;
      }
    }
  }

  CHECK(final_match.has_value());
  VLOG(2) << "Matched index " << blob_index << " to " << *final_match
          << " distance " << distance.x << " " << distance.y;

  return *final_match;
}

void TargetEstimator::DrawProjectedHub(
    const std::vector<cv::Point2d> &tape_points_proj,
    cv::Mat view_image) const {
  for (size_t i = 0; i < tape_points_proj.size() - 1; i++) {
    cv::line(view_image, ScalarPointToDouble(tape_points_proj[i]),
             ScalarPointToDouble(tape_points_proj[i + 1]),
             cv::Scalar(255, 255, 255));
    cv::circle(view_image, ScalarPointToDouble(tape_points_proj[i]), 2,
               cv::Scalar(255, 20, 147), cv::FILLED);
    cv::circle(view_image, ScalarPointToDouble(tape_points_proj[i + 1]), 2,
               cv::Scalar(255, 20, 147), cv::FILLED);
  }
}

void TargetEstimator::DrawEstimate(cv::Mat view_image) const {
  if (FLAGS_draw_projected_hub) {
    // Draw projected hub
    const auto H_hub_camera = ComputeHubCameraTransform(
        roll_, pitch_, yaw_, distance_, angle_to_camera_, camera_height_);
    std::vector<cv::Point2d> tape_points_proj;
    for (cv::Point3d tape_point_hub : kTapePoints) {
      tape_points_proj.emplace_back(
          ProjectToImage(tape_point_hub, H_hub_camera));
    }
    DrawProjectedHub(tape_points_proj, view_image);
  }

  constexpr int kTextX = 10;
  int text_y = 0;
  constexpr int kTextSpacing = 25;

  const auto kTextColor = cv::Scalar(0, 255, 255);
  constexpr double kFontScale = 0.6;

  cv::putText(view_image,
              absl::StrFormat("Distance: %.3f m (%.3f in)", distance_,
                              distance_ / 0.0254),
              cv::Point(kTextX, text_y += kTextSpacing),
              cv::FONT_HERSHEY_DUPLEX, kFontScale, kTextColor, 2);
  cv::putText(view_image,
              absl::StrFormat("Angle to target: %.3f", angle_to_target()),
              cv::Point(kTextX, text_y += kTextSpacing),
              cv::FONT_HERSHEY_DUPLEX, kFontScale, kTextColor, 2);
  cv::putText(view_image,
              absl::StrFormat("Angle to camera: %.3f", angle_to_camera_),
              cv::Point(kTextX, text_y += kTextSpacing),
              cv::FONT_HERSHEY_DUPLEX, kFontScale, kTextColor, 2);

  cv::putText(view_image,
              absl::StrFormat("Roll: %.3f, pitch: %.3f, yaw: %.3f", roll_,
                              pitch_, yaw_),
              cv::Point(kTextX, text_y += kTextSpacing),
              cv::FONT_HERSHEY_DUPLEX, kFontScale, kTextColor, 2);

  cv::putText(view_image, absl::StrFormat("Confidence: %.3f", confidence_),
              cv::Point(kTextX, text_y += kTextSpacing),
              cv::FONT_HERSHEY_DUPLEX, kFontScale, kTextColor, 2);
}

}  // namespace y2022::vision
