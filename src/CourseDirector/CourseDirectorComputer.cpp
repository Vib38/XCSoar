// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CourseDirectorComputer.hpp"

#include <algorithm>
#include <cmath>

std::optional<Angle>
CalcWindCorrectionAngle(const Angle course, const SpeedVector wind,
                        const double true_airspeed) noexcept
{
  if (!wind.IsNonZero())
    return Angle::Zero();

  if (true_airspeed <= 0)
    return std::nullopt;

  /* the angle between the direction the wind blows towards and the
     desired course, the same quantity as
     GlideState::effective_wind_angle */
  const Angle theta = wind.bearing.Reciprocal() - course;

  const auto sine = wind.norm * theta.sin() / true_airspeed;
  if (sine < -1 || sine > 1)
    /* the crosswind component exceeds the airspeed, so this course
       cannot be made good at all */
    return std::nullopt;

  const Angle correction = Angle::asin(sine);

  /* the crosswind fits, but the along-course components may still add
     up to a negative ground speed, in which case the course cannot be
     held either */
  if (true_airspeed * correction.cos() + wind.norm * theta.cos() <= 0)
    return std::nullopt;

  return correction;
}

void
CourseDirectorComputer::Reset() noexcept
{
  filter_valid = false;
  filtered_error = Angle::Zero();
  on_course = false;
}

std::optional<CourseDirectorState>
CourseDirectorComputer::Compute(const Angle bearing, const Angle heading,
                                const SpeedVector wind,
                                const double true_airspeed,
                                const FloatDuration dt) noexcept
{
  const auto correction = CalcWindCorrectionAngle(bearing, wind,
                                                  true_airspeed);
  if (!correction) {
    Reset();
    return std::nullopt;
  }

  const Angle required_heading = bearing - *correction;
  const Angle error = (required_heading - heading).AsDelta();

  if (!filter_valid) {
    /* the first sample: adopt the value instead of sliding towards it
       from an arbitrary start, and seed the hysteresis with the wide
       threshold, so that one dropped sample does not force the
       aircraft back below #ON_COURSE_ENTER */
    filtered_error = error;
    filter_valid = true;
    on_course = error.Absolute() < ON_COURSE_LEAVE;
  } else if (dt > FloatDuration{}) {
    const auto factor = 1 - std::exp(-(dt / FILTER_TIME_CONSTANT));

    /* filter the difference rather than the angle itself, so that the
       wrap at +/-180 degrees does not sweep the bar across the whole
       scale */
    const Angle difference = (error - filtered_error).AsDelta();
    filtered_error = (filtered_error + difference * factor).AsDelta();
  }
  /* else the clock did not advance; keep the previous value */

  const auto full_scale = FULL_SCALE.Degrees();
  const auto deflection =
    std::clamp(filtered_error.Degrees(), -full_scale, full_scale) / full_scale;

  const Angle absolute_error = filtered_error.Absolute();
  if (on_course) {
    if (absolute_error > ON_COURSE_LEAVE)
      on_course = false;
  } else if (absolute_error < ON_COURSE_ENTER) {
    on_course = true;
  }

  return CourseDirectorState{filtered_error, deflection, on_course};
}
