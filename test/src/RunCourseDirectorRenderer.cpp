// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON

#include "Main.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Form/Button.hpp"
#include "Look/CourseDirectorLook.hpp"
#include "Renderer/CourseDirectorRenderer.hpp"

/** The interval between updates, and the time step handed to the computer. */
static constexpr auto TICK = std::chrono::milliseconds(100);

static constexpr double TAS = 30;

class CourseDirectorWindow : public PaintWindow {
  const CourseDirectorLook &look;

  CourseDirectorComputer computer;
  std::optional<CourseDirectorState> state;

public:
  explicit CourseDirectorWindow(const CourseDirectorLook &_look) noexcept
    :look(_look) {}

  void Update(Angle heading, SpeedVector wind) noexcept {
    state = computer.Compute(Angle::Zero(), heading, wind, TAS,
                             FloatDuration{TICK});
    Invalidate();
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    CourseDirectorRenderer::Draw(canvas, canvas.GetRect(), look, state);
  }
};

static void
Main(TestMainWindow &main_window)
{
  CourseDirectorLook look;
  look.Initialise(false, normal_font);

  WindowStyle with_border;
  with_border.Border();

  CourseDirectorWindow window(look);
  window.Create(main_window, main_window.GetClientRect(), with_border);
  main_window.SetFullWindow(window);

  unsigned n = 0;

  UI::PeriodicTimer timer([&window, &n](){
    /* sweep the heading across the scale and back, then hold a
       crosswind stronger than the airspeed for three seconds, which
       is the case where no command can be given */
    constexpr unsigned sweep_ticks = 200;
    constexpr unsigned excessive_wind_ticks = 30;

    const unsigned phase = n++ % (sweep_ticks + excessive_wind_ticks);
    const bool sweeping = phase < sweep_ticks;

    const SpeedVector wind = sweeping
      ? SpeedVector(Angle::Degrees(270), 10)
      : SpeedVector(Angle::Degrees(270), 40);

    constexpr unsigned half = sweep_ticks / 2;
    const double ramp = !sweeping
      ? 0.5
      : (phase < half
         ? double(phase) / half
         : double(sweep_ticks - phase) / half);

    /* the sweep is wider than the scale, so that both the clamped bar
       and the on course state are shown once per cycle */
    window.Update(Angle::Degrees(-40 + 80 * ramp), wind);
  });

  timer.Schedule(TICK);
  main_window.RunEventLoop();
}
