// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CourseDirectorRenderer.hpp"
#include "Look/CourseDirectorLook.hpp"
#include "Screen/Layout.hpp"
#include "Units/Units.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "util/Macros.hpp"
#include "util/StringFormat.hpp"

#include <algorithm>
#include <cmath>

/** The interval between graduations [degrees]. */
static constexpr int TICK_INTERVAL = 10;

void
CourseDirectorRenderer::Draw(Canvas &canvas, const PixelRect &rc,
                             const CourseDirectorLook &look,
                             const std::optional<CourseDirectorState> &state) noexcept
{
  canvas.DrawFilledRectangle(rc, look.background_brush);

  const int width = rc.GetWidth(), height = rc.GetHeight();
  const auto centre = rc.GetCenter();

  const int margin = Layout::VptScale(2);
  const int bar_half_width = std::max(1, (int)Layout::VptScale(3));

  /* full deflection is the state the pilot most needs to recognise,
     so the scale stops half a bar short of the edge rather than
     letting the bar be clipped there */
  const int half_span = width / 2 - bar_half_width - margin;
  if (half_span <= 0 || height <= 4 * margin)
    return;

  const bool active = state.has_value();

  const int index_half_width = std::max(2, (int)Layout::VptScale(4));
  const int index_height = std::max(2, (int)Layout::VptScale(5));

  const int tick_bottom = rc.bottom - margin;
  const int tick_top = tick_bottom - std::max(1, height / 3);
  const int full_scale_tick_top = tick_bottom - std::max(1, height / 2);

  canvas.Select(active ? look.centre_pen : look.inactive_pen);
  canvas.DrawLine({centre.x, rc.top + margin}, {centre.x, tick_bottom});

  canvas.Select(active ? look.tick_pen : look.inactive_pen);

  const int full_scale = (int)CourseDirectorComputer::FULL_SCALE.Degrees();
  for (int degrees = TICK_INTERVAL; degrees <= full_scale;
       degrees += TICK_INTERVAL) {
    const int offset = half_span * degrees / full_scale;
    /* the outermost graduation is longer, because it is the one that
       says where the scale ends */
    const int top = degrees + TICK_INTERVAL > full_scale
      ? full_scale_tick_top
      : tick_top;

    canvas.DrawLine({centre.x - offset, top}, {centre.x - offset, tick_bottom});
    canvas.DrawLine({centre.x + offset, top}, {centre.x + offset, tick_bottom});
  }

  if (!active)
    /* no index and no bar: a dead scale must not be mistaken for a
       command to fly straight ahead */
    return;

  /* the full scale value, without which the scale is ambiguous: the
     same picture would mean ten degrees or forty */
  char buffer[8];
  StringFormatUnsafe(buffer, "%d" DEG, full_scale);

  canvas.Select(*look.font);
  canvas.SetTextColor(look.label_color);
  canvas.SetBackgroundTransparent();

  const auto label_size = canvas.CalcTextSize(buffer);

  /* the two labels and the index share the top of the band; drop them
     rather than let them collide */
  const bool labelled =
    (int)label_size.width + margin + index_half_width < half_span &&
    (int)label_size.height + margin < height / 2;

  if (labelled) {
    const int label_top = rc.top + margin;
    canvas.DrawText({centre.x - half_span + margin, label_top}, buffer);
    canvas.DrawText({centre.x + half_span - margin - (int)label_size.width,
                     label_top}, buffer);
  }

  canvas.SelectNullPen();
  canvas.Select(look.index_brush);

  const BulkPixelPoint index[] = {
    {centre.x - index_half_width, rc.top},
    {centre.x + index_half_width, rc.top},
    {centre.x, rc.top + index_height},
  };
  canvas.DrawPolygon(index, ARRAY_SIZE(index));

  /* the bar starts below the labels, because at full deflection it
     ends up exactly where they are drawn */
  const int header_height = labelled
    ? std::max<int>(index_height, margin + label_size.height)
    : index_height;
  const int bar_top = rc.top + header_height;
  const int bar_bottom = rc.bottom - margin;
  if (bar_bottom <= bar_top)
    return;

  const int bar_centre = centre.x +
    (int)std::lround(state->deflection * half_span);

  canvas.Select(state->on_course ? look.on_course_brush : look.bar_brush);
  canvas.Select(state->on_course ? look.on_course_pen : look.bar_pen);

  const BulkPixelPoint bar[] = {
    {bar_centre - bar_half_width, bar_top},
    {bar_centre + bar_half_width, bar_top},
    {bar_centre + bar_half_width, bar_bottom},
    {bar_centre - bar_half_width, bar_bottom},
  };
  canvas.DrawPolygon(bar, ARRAY_SIZE(bar));
}
