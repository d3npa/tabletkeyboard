// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#pragma once

// Ranges shared by the App setters (which enforce them) and the settings
// dialog controls, so the two sides cannot drift apart.
namespace osk {
namespace limits {
inline constexpr int minKeyUnitPx = 0; // 0 = the theme's key_unit
inline constexpr int maxKeyUnitPx = 240;
inline constexpr double minScale = 0.5;
inline constexpr double maxScale = 3.0;
inline constexpr int minStickyTimeoutMs = 0; // 0 = never expire on its own
inline constexpr int maxStickyTimeoutMs = 10000;
} // namespace limits
} // namespace osk
