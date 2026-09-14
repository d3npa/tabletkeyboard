#pragma once

// Ids the application layer knows by name. They are part of the layout schema:
// the shipped layouts use them to line the nav cluster and the numpad up with
// the F-row (which App can hide), so both sides have to agree on the strings.
namespace osk {
namespace blocks {
inline constexpr const char *kFrow = "frow";
inline constexpr const char *kFrowGap = "frowgap";
inline constexpr const char *kNumpad = "numpad";
} // namespace blocks
} // namespace osk
