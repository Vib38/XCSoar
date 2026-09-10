// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "CourseDirector/CourseDirectorComputer.hpp"

#include <optional>

struct PixelRect;
struct CourseDirectorLook;
class Canvas;

namespace CourseDirectorRenderer {

/**
 * Draw the course director band, a horizontal scale with a fixed
 * index at its centre and a bar the pilot flies onto it.
 *
 * @param state the command; while it is std::nullopt the band shows a
 * dead scale, because a bar left at its last position would be read
 * as a command that is no longer given
 */
void Draw(Canvas &canvas, const PixelRect &rc,
          const CourseDirectorLook &look,
          const std::optional<CourseDirectorState> &state) noexcept;

} // namespace CourseDirectorRenderer
