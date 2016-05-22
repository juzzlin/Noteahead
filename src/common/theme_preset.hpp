// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef THEME_PRESET_HPP
#define THEME_PRESET_HPP

#include <QColor>
#include <QString>

#include <optional>

namespace noteahead {

//! A named set of theme settings, built in or saved by the user.
//!
//! Every value is optional so that a theme saved before a setting existed still loads: what it
//! does not name is taken from Init. A spectrum color left out follows the accent, which is also
//! what Init does.
struct ThemePreset
{
    QString name {};
    std::optional<QColor> accentColor {};
    std::optional<QColor> cursorColor {};
    std::optional<int> paletteAccentBlend {};
    std::optional<int> volumeMeterStyle {};
    std::optional<QColor> spectrumPostColor {};
    std::optional<QColor> spectrumPreColor {};
};

} // namespace noteahead

#endif // THEME_PRESET_HPP
