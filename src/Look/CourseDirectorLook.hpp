// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

class Font;

/**
 * Colours for the course director band.
 *
 * The band is opaque, so it needs a ground colour of its own; the map
 * overlays take theirs from the map below them.
 */
struct CourseDirectorLook {
  /** The opaque ground of the band. */
  Brush background_brush;

  /** The graduations. */
  Pen tick_pen;

  /** The hairline marking the centre of the scale. */
  Pen centre_pen;

  /** The fixed index the bar is centred against. */
  Brush index_brush;

  /** The moving bar while off course. */
  Brush bar_brush;
  Pen bar_pen;

  /**
   * The moving bar while on course.
   *
   * On a display without colours this is an outline rather than
   * another shade, because red and green are indistinguishable
   * there.
   */
  Brush on_course_brush;
  Pen on_course_pen;

  /** The graduations while no command can be given. */
  Pen inactive_pen;

  /** The scale labels. */
  Color label_color;

  const Font *font;

  void Initialise(bool dark_mode, const Font &font);
};
