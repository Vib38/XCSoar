// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

class CourseDirectorWidget final : public WindowWidget,
                                   private NullBlackboardListener {
  void Update(const MoreData &basic, const DerivedInfo &calculated) noexcept;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  /* virtual methods from class BlackboardListener */
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) noexcept override;
};
