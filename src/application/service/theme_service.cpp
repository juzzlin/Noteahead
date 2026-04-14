// This file is part of Noteahead.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
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

#include "theme_service.hpp"

#include "../../infra/settings.hpp"

namespace noteahead {

ThemeService::ThemeService()
  : m_accentColor { Settings::accentColor(QColor { "orange" }) }
{
}

ThemeService::~ThemeService() = default;

QColor ThemeService::accentColor() const
{
    return m_accentColor;
}

void ThemeService::setAccentColor(const QColor & accentColor)
{
    if (m_accentColor != accentColor) {
        m_accentColor = accentColor;
        Settings::setAccentColor(m_accentColor);
        emit accentColorChanged();
        emit trackHeaderTextColorsChanged();
    }
}

QColor ThemeService::lineNumberColumnBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::lineNumberColumnBorderColor() const
{
    return QColor { "#444444" };
}

QColor ThemeService::lineNumberColumnCellBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::lineNumberColumnCellBorderColor() const
{
    return QColor { "#222222" };
}

QColor ThemeService::lineNumberColumnOverflowTextColor() const
{
    return QColor { "#444444" };
}

QColor ThemeService::mainMenuTextColor() const
{
    return QColor { "white" };
}

QColor ThemeService::mainToolBarGradientStartColor() const
{
    return QColor { "#303030" };
}

QColor ThemeService::mainToolBarGradientStopColor() const
{
    return QColor { "black" };
}

QColor ThemeService::mainToolBarSeparatorColor() const
{
    return QColor { "white" };
}

QColor ThemeService::mainToolBarTextColor() const
{
    return QColor { "white" };
}

QColor ThemeService::noteColumnBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::noteColumnBorderColor() const
{
    return QColor { "#444444" };
}

QColor ThemeService::noteColumnCellBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::noteColumnCellBorderColor() const
{
    return QColor { "#222222" };
}

QColor ThemeService::noteColumnTextColor() const
{
    return QColor { "white" };
}

QColor ThemeService::noteColumnTextColorEmpty() const
{
    return QColor { "#888888" };
}

QColor ThemeService::positionBarBorderColor() const
{
    return QColor { "white" };
}

QColor ThemeService::positionBarBorderColorEditMode() const
{
    return QColor { "red" };
}

QColor ThemeService::progressBarBackgroundColor() const
{
    return QColor { "#4a4a4a" };
}

QColor ThemeService::recentFileItemTextColor() const
{
    return QColor { "white" };
}

QColor ThemeService::trackBorderColor() const
{
    return QColor { "#222222" };
}

QColor ThemeService::trackHeaderBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::trackHeaderBorderColor() const
{
    return QColor { "#222222" };
}

QVariantList ThemeService::trackHeaderTextColors() const
{
    return QVariantList {
        m_accentColor,
        QColor { "white" },
        QColor { "#ff5555" },
        QColor { "#55ff55" },
        QColor { "#5555ff" },
        QColor { "#ffff55" },
        QColor { "#ff55ff" },
        QColor { "#55ffff" },
        QColor { "#aaaaaa" },
        QColor { "#ff8800" },
        QColor { "#88ff00" },
        QColor { "#0088ff" },
        QColor { "#ff0088" },
        QColor { "#8800ff" },
        QColor { "#00ff88" },
        QColor { "#888888" }
    };
}

QColor ThemeService::trackHeaderTextColor(int trackIndex) const
{
    const auto colors = trackHeaderTextColors();
    return colors.at(trackIndex % colors.size()).value<QColor>();
}

} // namespace noteahead
