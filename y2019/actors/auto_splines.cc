#include "y2019/actors/auto_splines.h"

#include "frc971/control_loops/control_loops.q.h"

namespace y2019 {
namespace actors {

::frc971::MultiSpline AutonomousSplines::HPToNearRocket() {
  ::frc971::MultiSpline spline;
  ::frc971::Constraint longitudinal_constraint;
  ::frc971::Constraint lateral_constraint;
  ::frc971::Constraint voltage_constraint;

  longitudinal_constraint.constraint_type = 1;
  longitudinal_constraint.value = 1.0;

  lateral_constraint.constraint_type = 2;
  lateral_constraint.value = 1.0;

  voltage_constraint.constraint_type = 3;
  voltage_constraint.value = 12.0;

  spline.spline_count = 1;
  spline.spline_x = {{0.4, 1.0, 3.0, 4.0, 4.5, 5.05}};
  spline.spline_y = {{3.4, 3.4, 3.4, 3.0, 3.0, 3.5}};
  spline.constraints = {
      {longitudinal_constraint, lateral_constraint, voltage_constraint}};
  return spline;
}

::frc971::MultiSpline AutonomousSplines::BasicSSpline() {
  ::frc971::MultiSpline spline;
  ::frc971::Constraint longitudinal_constraint;
  ::frc971::Constraint lateral_constraint;
  ::frc971::Constraint voltage_constraint;

  longitudinal_constraint.constraint_type = 1;
  longitudinal_constraint.value = 1.0;

  lateral_constraint.constraint_type = 2;
  lateral_constraint.value = 1.0;

  voltage_constraint.constraint_type = 3;
  voltage_constraint.value = 6.0;

  spline.spline_count = 1;
  const float startx = 0.4;
  const float starty = 3.4;
  spline.spline_x = {{0.0f + startx, 0.6f + startx, 0.6f + startx,
                      0.4f + startx, 0.4f + startx, 1.0f + startx}};
  spline.spline_y = {{starty - 0.0f, starty - 0.0f, starty - 0.3f,
                      starty - 0.7f, starty - 1.0f, starty - 1.0f}};
  spline.constraints = {
      {longitudinal_constraint, lateral_constraint, voltage_constraint}};
  return spline;
}

::frc971::MultiSpline AutonomousSplines::StraightLine() {
  ::frc971::MultiSpline spline;
  ::frc971::Constraint contraints;

  contraints.constraint_type = 0;
  contraints.value = 0.0;
  contraints.start_distance = 0.0;
  contraints.end_distance = 0.0;

  spline.spline_count = 1;
  spline.spline_x = {{-12.3, -11.9, -11.5, -11.1, -10.6, -10.0}};
  spline.spline_y = {{1.25, 1.25, 1.25, 1.25, 1.25, 1.25}};
  spline.constraints = {{contraints}};
  return spline;
}

}  // namespace actors
}  // namespace y2019