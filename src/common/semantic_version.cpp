// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#include "semantic_version.hpp"

#include <array>
#include <charconv>
#include <format>

namespace noteahead {

std::optional<SemanticVersion> SemanticVersion::parse(std::string_view text)
{
    if (text.starts_with('v') || text.starts_with('V')) {
        text.remove_prefix(1);
    }
    std::array<int, 3> parts {};
    for (size_t i = 0; i < parts.size(); i++) {
        if (text.empty()) {
            return {};
        }
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), parts.at(i));
        if (error != std::errc {} || parts.at(i) < 0) {
            return {};
        }
        text.remove_prefix(static_cast<size_t>(end - text.data()));
        // Dots between the parts, and nothing at all after the last one
        if (i + 1 < parts.size()) {
            if (!text.starts_with('.')) {
                return {};
            }
            text.remove_prefix(1);
        }
    }
    if (!text.empty()) {
        return {};
    }
    return SemanticVersion { .major = parts.at(0), .minor = parts.at(1), .patch = parts.at(2) };
}

std::string SemanticVersion::toString() const
{
    return std::format("{}.{}.{}", major, minor, patch);
}

} // namespace noteahead
