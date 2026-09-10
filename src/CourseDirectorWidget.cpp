// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CourseDirectorWidget.hpp"
#include "CourseDirector/CourseDirectorComputer.hpp"
#include "Interface.hpp"
#include "Look/Look.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "Renderer/CourseDirectorRenderer.hpp"
#include "UIGlobals.hpp"
#include "time/DeltaTime.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/AntiFlickerWindow.hpp"

#include <cmath>

/**
 * A Window which shows the course director band.
 */
class CourseDirectorWindow : public AntiFlickerWindow {
  const CourseDirectorLook &look;

  CourseDirectorComputer computer;
  DeltaTime delta_time;

  std::optional<CourseDirectorState> state;

public:
  explicit CourseDirectorWindow(const CourseDirectorLook &_look) noexcept
    :look(_look)
  {
    delta_time.Reset();
  }

  void ReadBlackboard(const MoreData &basic,
                      const DerivedInfo &calculated) noexcept {
    state = Compute(basic, calculated);
    Invalidate();
  }

private:
  void Reset() noexcept {
    computer.Reset();
    delta_time.Reset();
  }

  [[nodiscard]]
  std::optional<CourseDirectorState> Compute(const MoreData &basic,
                                             const DerivedInfo &calculated) noexcept;

protected:
  /* virtual methods from AntiFlickerWindow */
  void OnPaintBuffer(Canvas &canvas) noexcept override {
    CourseDirectorRenderer::Draw(canvas, canvas.GetRect(), look, state);
  }
};

std::optional<CourseDirectorState>
CourseDirectorWindow::Compute(const MoreData &basic,
                              const DerivedInfo &calculated) noexcept
{
  const auto &vector = calculated.task_stats.current_leg.vector_remaining;

  /* the bearing is undefined once the turn point is reached, and the
     heading is whatever the last fix left behind while on the ground */
  if (!calculated.task_stats.task_valid || !vector.IsValid() ||
      vector.distance <= 0 ||
      !calculated.flight.flying ||
      !basic.time_available || !basic.attitude.heading_available ||
      !basic.track_available || !basic.ground_speed_available) {
    Reset();
    return std::nullopt;
  }

  const auto dt = delta_time.Update(basic.time, {}, {});
  if (dt.count() < 0) {
    /* a time warp: the filter would carry a value from a time that no
       longer applies */
    Reset();
    return std::nullopt;
  }

  const SpeedVector wind = calculated.GetWindOrZero();

  /* the airspeed is derived here rather than taken from
     NMEAInfo::true_airspeed, because the computation needs the
     airspeed the heading is consistent with.  Without a compass the
     heading itself comes from the ground vector and the wind, and a
     measured airspeed that disagrees with them biases the error for
     as long as the disagreement lasts */
  const auto x = basic.track.fastsine() * basic.ground_speed +
    wind.bearing.fastsine() * wind.norm;
  const auto y = basic.track.fastcosine() * basic.ground_speed +
    wind.bearing.fastcosine() * wind.norm;
  const double true_airspeed = hypot(x, y);

  return computer.Compute(vector.bearing, basic.attitude.heading, wind,
                          true_airspeed, dt);
}

void
CourseDirectorWidget::Update(const MoreData &basic,
                             const DerivedInfo &calculated) noexcept
{
  ((CourseDirectorWindow &)GetWindow()).ReadBlackboard(basic, calculated);
}

void
CourseDirectorWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  const Look &look = UIGlobals::GetLook();

  WindowStyle style;
  style.Hide();

  auto w = std::make_unique<CourseDirectorWindow>(look.course_director);
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

void
CourseDirectorWidget::Show(const PixelRect &rc) noexcept
{
  Update(CommonInterface::Basic(), CommonInterface::Calculated());
  CommonInterface::GetLiveBlackboard().AddListener(*this);

  WindowWidget::Show(rc);
}

void
CourseDirectorWidget::Hide() noexcept
{
  WindowWidget::Hide();

  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

void
CourseDirectorWidget::OnCalculatedUpdate(const MoreData &basic,
                                         const DerivedInfo &calculated) noexcept
{
  Update(basic, calculated);
}
