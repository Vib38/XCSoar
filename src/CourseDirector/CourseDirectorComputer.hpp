// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SpeedVector.hpp"
#include "Math/Angle.hpp"
#include "time/FloatDuration.hxx"

#include <optional>

/**
 * Calculate the wind correction angle: how far the aircraft must be
 * pointed away from the desired course so that the resulting ground
 * track follows it.
 *
 * The sign convention is the one used by
 * GlideResult::CalcCruiseBearing(): the heading to fly is the course
 * minus the returned angle.
 *
 * @param course the desired course over ground
 * @param wind the wind vector; its bearing is the direction the wind
 * comes from
 * @param true_airspeed the true airspeed [m/s]
 * @return the correction angle, or std::nullopt if the course cannot
 * be made good: either the crosswind component exceeds the airspeed,
 * or the ground speed along the course is not positive
 */
[[gnu::pure]]
std::optional<Angle>
CalcWindCorrectionAngle(Angle course, SpeedVector wind,
                        double true_airspeed) noexcept;

/**
 * The command presented by the course director.
 */
struct CourseDirectorState {
  /**
   * The filtered heading error, normalised to [-180,+180].  Positive
   * means the aircraft must turn right.
   */
  Angle error;

  /**
   * #error mapped onto the display scale: -1 at full left deflection,
   * +1 at full right, clamped beyond that.
   */
  double deflection;

  /**
   * Is the aircraft on course?  Hysteretic; see #ON_COURSE_ENTER and
   * #ON_COURSE_LEAVE.
   */
  bool on_course;
};

/**
 * Reduces the bearing to the next turn point and the wind to a single
 * left/right deviation the pilot can null.
 *
 * Unlike the "best cruise track" arrow, this commands a heading and
 * not a track.  With a compass or an AHRS connected, heading responds
 * to the stick immediately, whereas ground track lags it by several
 * seconds.  Without one, XCSoar derives the heading from track and
 * wind, and the director degrades to a track director which still
 * nulls at the right place.
 *
 * The low-pass filter and the on-course hysteresis carry state from
 * one call to the next.
 */
class CourseDirectorComputer {
public:
  /** The error at which the display reaches full deflection. */
  static constexpr Angle FULL_SCALE = Angle::Degrees(30);

  /** The error below which the aircraft is considered on course. */
  static constexpr Angle ON_COURSE_ENTER = Angle::Degrees(2);

  /**
   * The error above which the "on course" state is dropped again.
   *
   * Deliberately wider than #ON_COURSE_ENTER.  Without a compass the
   * heading is reconstructed from the GPS track, and a single
   * threshold then makes the indicator change colour dozens of times
   * per minute while the aircraft is in fact steady.
   */
  static constexpr Angle ON_COURSE_LEAVE = Angle::Degrees(4);

  /**
   * Time constant of the low pass filter applied to the error.
   *
   * Every second of filtering costs several seconds of settling time
   * after a disturbance, so this stays short; the hysteresis above,
   * not the filter, is what keeps the display steady.
   */
  static constexpr FloatDuration FILTER_TIME_CONSTANT{0.5};

private:
  bool filter_valid = false;
  Angle filtered_error = Angle::Zero();
  bool on_course = false;

public:
  /**
   * Forget the filter and hysteresis state.  Call this whenever the
   * command becomes meaningless, so that the display does not resume
   * from a stale value.
   */
  void Reset() noexcept;

  /**
   * @param bearing the bearing to the next turn point
   * @param heading the current heading
   * @param wind the wind vector
   * @param true_airspeed the true airspeed [m/s]; it must be the one
   * the heading is consistent with, that is |ground vector + wind|.
   * An airspeed from another source biases the error permanently: in
   * 20 m/s of wind, 30 m/s instead of 33 m/s leaves about 4.5 degrees
   * of error, which is more than the hysteresis absorbs.
   * @param dt the time elapsed since the previous call
   * @return the command, or std::nullopt if no meaningful command can
   * be given
   */
  std::optional<CourseDirectorState> Compute(Angle bearing, Angle heading,
                                             SpeedVector wind,
                                             double true_airspeed,
                                             FloatDuration dt) noexcept;
};
