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

#ifndef SEMANTIC_VERSION_HPP
#define SEMANTIC_VERSION_HPP

#include <compare>
#include <optional>
#include <string>
#include <string_view>

namespace noteahead {

//! A release version, major.minor.patch, compared part by part as numbers: 12.10.0 is newer than
//! 12.9.0, which comparing the strings would get wrong.
struct SemanticVersion
{
    int major { 0 };
    int minor { 0 };
    int patch { 0 };

    //! "12.1.1", or "v12.1.1" as a tag may be written. Anything else, a pre-release suffix such as
    //! "12.2.0-beta" included, is not a release and gives nothing.
    static std::optional<SemanticVersion> parse(std::string_view text);

    std::string toString() const;

    auto operator<=>(const SemanticVersion &) const = default;
};

} // namespace noteahead

#endif // SEMANTIC_VERSION_HPP
