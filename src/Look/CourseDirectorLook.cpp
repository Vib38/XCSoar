// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CourseDirectorLook.hpp"
#include "Colors.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"

void
CourseDirectorLook::Initialise(const bool dark_mode, const Font &_font)
{
  const Color background_color = HasColors()
    ? (dark_mode ? COLOR_DARK_THEME_BACKGROUND : COLOR_WHITE)
    : COLOR_WHITE;
  const Color foreground_color = HasColors() && dark_mode
    ? COLOR_WHITE
    : COLOR_BLACK;

  background_brush.Create(background_color);

  const Color tick_color = HasColors()
    ? (dark_mode ? Color(0x6f, 0xc8, 0xf0) : Color(0x30, 0x58, 0x70))
    : COLOR_BLACK;
  tick_pen.Create(Layout::ScalePenWidth(1), tick_color);
  centre_pen.Create(Pen::DASH2, Layout::ScaleFinePenWidth(1), tick_color);

  index_brush.Create(foreground_color);

  /* COLOR_GREEN is too bright to read against a white band; see
     COLOR_LIGHT_GREEN in Colors.hpp */
  const Color on_course_color = dark_mode ? COLOR_GREEN : COLOR_LIGHT_GREEN;

  if (HasColors()) {
    bar_brush.Create(COLOR_RED);
    bar_pen.Create(Layout::ScalePenWidth(1), DarkColor(COLOR_RED));
    on_course_brush.Create(on_course_color);
    on_course_pen.Create(Layout::ScalePenWidth(1), DarkColor(on_course_color));
  } else {
    /* without colours the two states are told apart by fill: solid
       while off course, hollow once centred */
    bar_brush.Create(COLOR_BLACK);
    bar_pen.Create(Layout::ScalePenWidth(1), COLOR_BLACK);
    on_course_brush.Create(COLOR_WHITE);
    on_course_pen.Create(Layout::ScalePenWidth(2), COLOR_BLACK);
  }

  inactive_pen.Create(Layout::ScalePenWidth(1),
                      HasColors() ? COLOR_LIGHT_GRAY : COLOR_GRAY);

  label_color = foreground_color;

  font = &_font;
}
