#include "y2019/control_loops/drivetrain/event_loop_localizer.h"

#include <functional>

namespace y2019 {
namespace control_loops {
namespace drivetrain {
constexpr size_t EventLoopLocalizer::kMaxTargetsPerFrame;

::std::array<EventLoopLocalizer::Camera, constants::Values::kNumCameras>
MakeCameras(EventLoopLocalizer::Pose *pose) {
  constants::Field field;
  ::std::array<EventLoopLocalizer::Camera, constants::Values::kNumCameras>
      cameras;
  for (size_t ii = 0; ii < constants::Values::kNumCameras; ++ii) {
    constants::Values::CameraCalibration camera_info =
        constants::GetValues().cameras[ii];
    EventLoopLocalizer::Pose camera_pose = camera_info.pose.Rebase(pose);
    cameras[ii] = EventLoopLocalizer::Camera(
        camera_pose, camera_info.fov,
        constants::GetValues().camera_noise_parameters, field.targets(),
        field.obstacles());
  }
  return cameras;
}

EventLoopLocalizer::EventLoopLocalizer(
    const ::frc971::control_loops::drivetrain::DrivetrainConfig<double>
        &dt_config,
    ::aos::EventLoop *event_loop)
    : event_loop_(event_loop),
      cameras_(MakeCameras(&robot_pose_)),
      localizer_(dt_config, &robot_pose_) {
  localizer_.ResetInitialState(::aos::monotonic_clock::now(),
                               Localizer::State::Zero(), localizer_.P());
  frame_fetcher_ = event_loop_->MakeFetcher<CameraFrame>(
      ".y2019.control_loops.drivetrain.camera_frames");
  target_selector_.set_has_target(false);
}

void EventLoopLocalizer::Reset(const Localizer::State &state) {
  localizer_.ResetInitialState(::aos::monotonic_clock::now(), state,
                               localizer_.P());
}

void EventLoopLocalizer::Update(
    const ::Eigen::Matrix<double, 2, 1> &U,
    ::aos::monotonic_clock::time_point now, double left_encoder,
    double right_encoder, double gyro_rate,
    double /*longitudinal_accelerometer*/) {
  localizer_.UpdateEncodersAndGyro(left_encoder, right_encoder, gyro_rate, U,
                                   now);
  while (frame_fetcher_.FetchNext()) {
    HandleFrame(*frame_fetcher_.get());
  }
}

void EventLoopLocalizer::HandleFrame(const CameraFrame &frame) {
  // We need to construct TargetView's and pass them to the localizer:
  ::aos::SizedArray<TargetView, kMaxTargetsPerFrame> views;
  for (int ii = 0; ii < frame.num_targets; ++ii) {
    TargetView view;
    view.reading.heading = frame.targets[ii].heading;
    view.reading.distance = frame.targets[ii].distance;
    view.reading.skew = frame.targets[ii].skew;
    view.reading.height = frame.targets[ii].height;
    cameras_[frame.camera].PopulateNoise(&view);
    views.push_back(view);
  }
  ::aos::monotonic_clock::time_point t(
      ::std::chrono::nanoseconds(frame.timestamp));
  localizer_.UpdateTargets(cameras_[frame.camera], views, t);
}

}  // namespace drivetrain
}  // namespace control_loops
}  // namespace y2019