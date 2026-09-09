// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CourseDirector/CourseDirectorComputer.hpp"
#include "TestUtil.hpp"

#include <cmath>

static constexpr double TAS = 30;
static constexpr FloatDuration TICK{1};

static constexpr SpeedVector
WindFrom(double degrees, double norm) noexcept
{
  return SpeedVector(Angle::Degrees(degrees), norm);
}

/**
 * The wind triangle on its own, without the filter or the hysteresis.
 */
static void
TestWindCorrectionAngle() noexcept
{
  const auto calm = CalcWindCorrectionAngle(Angle::Zero(),
                                            SpeedVector::Zero(), TAS);
  ok1(calm.has_value());
  ok1(equals(calm->Degrees(), 0.));

  /* without an airspeed the triangle cannot be solved */
  ok1(!CalcWindCorrectionAngle(Angle::Zero(), WindFrom(270, 10), 0));

  /* heading north with the wind coming from the west: the wind blows
     the aircraft east, so it must be crabbed west, and the heading to
     fly is the course minus a positive correction */
  const auto expected = Angle::asin(10. / TAS).Degrees();
  const auto from_west = CalcWindCorrectionAngle(Angle::Zero(),
                                                 WindFrom(270, 10), TAS);
  ok1(from_west.has_value());
  ok1(equals(from_west->Degrees(), expected));

  const auto from_east = CalcWindCorrectionAngle(Angle::Zero(),
                                                 WindFrom(90, 10), TAS);
  ok1(equals(from_east->Degrees(), -expected));

  /* a pure head or tail wind needs no correction at all */
  ok1(equals(CalcWindCorrectionAngle(Angle::Zero(),
                                     WindFrom(0, 10), TAS)->Degrees(), 0.));
  ok1(equals(CalcWindCorrectionAngle(Angle::Zero(),
                                     WindFrom(180, 10), TAS)->Degrees(), 0.));

  /* the same wind is a tail wind on an easterly course */
  ok1(equals(CalcWindCorrectionAngle(Angle::Degrees(90),
                                     WindFrom(270, 10), TAS)->Degrees(), 0.));

  /* a crosswind stronger than the airspeed: the course cannot be made
     good at all */
  ok1(!CalcWindCorrectionAngle(Angle::Zero(), WindFrom(270, 40), TAS));

  /* the crosswind component fits, but the head wind exceeds the
     airspeed, so the aircraft would move backwards along the course */
  ok1(!CalcWindCorrectionAngle(Angle::Zero(), WindFrom(315, 40), TAS));
}

/**
 * Feed one sample into a fresh computer, which the filter adopts
 * verbatim, and check the mapping onto the display scale.
 */
static CourseDirectorState
FirstSample(double bearing_degrees) noexcept
{
  CourseDirectorComputer computer;
  const auto state = computer.Compute(Angle::Degrees(bearing_degrees),
                                      Angle::Zero(), SpeedVector::Zero(),
                                      TAS, TICK);
  return *state;
}

static void
TestDeflection() noexcept
{
  CourseDirectorComputer computer;
  const auto centred = computer.Compute(Angle::Zero(), Angle::Zero(),
                                        SpeedVector::Zero(), TAS, TICK);
  ok1(centred.has_value());
  ok1(equals(centred->deflection, 0.));
  ok1(centred->on_course);

  ok1(equals(FirstSample(30).deflection, 1.));

  /* beyond full scale the bar simply stays at the stop */
  ok1(equals(FirstSample(60).deflection, 1.));

  const auto left = FirstSample(-15);
  ok1(equals(left.deflection, -0.5));
  ok1(equals(left.error.Degrees(), -15.));
}

/**
 * The on-course state must not follow a single threshold, or a noisy
 * heading makes it flicker.
 */
static void
TestHysteresis() noexcept
{
  /* long enough that the filter has all but settled on each sample,
     so that this exercises the hysteresis and not the filter */
  static constexpr FloatDuration SETTLED{10};

  CourseDirectorComputer computer;
  const auto centred = computer.Compute(Angle::Zero(), Angle::Zero(),
                                        SpeedVector::Zero(), TAS, TICK);
  ok1(centred->on_course);

  ok1(computer.Compute(Angle::Degrees(3), Angle::Zero(),
                       SpeedVector::Zero(), TAS, SETTLED)->on_course);

  ok1(!computer.Compute(Angle::Degrees(5), Angle::Zero(),
                        SpeedVector::Zero(), TAS, SETTLED)->on_course);

  /* dropping back below the upper threshold is not enough to go green
     again; it has to come back inside the lower one */
  ok1(!computer.Compute(Angle::Degrees(3), Angle::Zero(),
                        SpeedVector::Zero(), TAS, SETTLED)->on_course);

  ok1(computer.Compute(Angle::Degrees(1), Angle::Zero(),
                       SpeedVector::Zero(), TAS, SETTLED)->on_course);
}

static void
TestFilter() noexcept
{
  CourseDirectorComputer computer;

  const auto first = computer.Compute(Angle::Degrees(20), Angle::Zero(),
                                      SpeedVector::Zero(), TAS, TICK);
  ok1(equals(first->error.Degrees(), 20.));

  /* one time constant towards zero leaves 1/e of the error */
  const auto decayed =
    computer.Compute(Angle::Zero(), Angle::Zero(), SpeedVector::Zero(),
                     TAS, CourseDirectorComputer::FILTER_TIME_CONSTANT);
  ok1(equals(decayed->error.Degrees(), 20 * std::exp(-1.)));

  /* a sample that carries no elapsed time must not move the filter */
  const auto stalled = computer.Compute(Angle::Zero(), Angle::Zero(),
                                        SpeedVector::Zero(), TAS,
                                        FloatDuration{});
  ok1(equals(stalled->error.Degrees(), 20 * std::exp(-1.)));
}

/**
 * A single unusable sample must not cost the on-course state, or the
 * indicator flickers on one dropped airspeed reading.
 */
static void
TestGlitch() noexcept
{
  static constexpr FloatDuration SETTLED{10};

  CourseDirectorComputer computer;
  computer.Compute(Angle::Degrees(1), Angle::Zero(), SpeedVector::Zero(),
                   TAS, TICK);
  ok1(computer.Compute(Angle::Degrees(3), Angle::Zero(), SpeedVector::Zero(),
                       TAS, SETTLED)->on_course);

  /* the airspeed drops out for one sample */
  ok1(!computer.Compute(Angle::Degrees(3), Angle::Zero(), WindFrom(270, 10),
                        0, TICK));

  /* the aircraft has not moved, so the state must come back with it
     rather than wait for the error to fall below ON_COURSE_ENTER */
  ok1(computer.Compute(Angle::Degrees(3), Angle::Zero(), SpeedVector::Zero(),
                       TAS, TICK)->on_course);
}

/**
 * Crossing +/-180 degrees must move the bar the short way, not sweep
 * it across the whole scale through the centre.
 */
static void
TestWrapAround() noexcept
{
  CourseDirectorComputer computer;
  computer.Compute(Angle::Degrees(170), Angle::Zero(), SpeedVector::Zero(),
                   TAS, TICK);

  const auto wrapped =
    computer.Compute(Angle::Degrees(190), Angle::Zero(), SpeedVector::Zero(),
                     TAS, CourseDirectorComputer::FILTER_TIME_CONSTANT);

  const auto expected = Angle::Degrees(170 + 20 * (1 - std::exp(-1.)))
    .AsDelta().Degrees();
  ok1(equals(wrapped->error.Degrees(), expected));

  /* it went out past the stop, not back through the middle */
  ok1(wrapped->error.AbsoluteDegrees() > 170);
}

static void
TestReset() noexcept
{
  CourseDirectorComputer computer;
  computer.Compute(Angle::Degrees(20), Angle::Zero(), SpeedVector::Zero(),
                   TAS, TICK);

  computer.Reset();
  const auto restarted =
    computer.Compute(Angle::Degrees(5), Angle::Zero(), SpeedVector::Zero(),
                     TAS, CourseDirectorComputer::FILTER_TIME_CONSTANT);
  ok1(equals(restarted->error.Degrees(), 5.));

  /* an unusable wind triangle yields no command ... */
  ok1(!computer.Compute(Angle::Zero(), Angle::Zero(), WindFrom(270, 40),
                        TAS, TICK));

  /* ... and drops the filter, so the display does not resume from a
     stale value */
  const auto resumed =
    computer.Compute(Angle::Degrees(7), Angle::Zero(), SpeedVector::Zero(),
                     TAS, CourseDirectorComputer::FILTER_TIME_CONSTANT);
  ok1(equals(resumed->error.Degrees(), 7.));
}

int
main()
{
  plan_tests(34);

  TestWindCorrectionAngle();
  TestDeflection();
  TestHysteresis();
  TestFilter();
  TestGlitch();
  TestWrapAround();
  TestReset();

  return exit_status();
}
