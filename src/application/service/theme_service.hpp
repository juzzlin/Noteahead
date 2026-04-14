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

#ifndef THEME_SERVICE_HPP
#define THEME_SERVICE_HPP

#include <QColor>
#include <QObject>
#include <QVariantList>

namespace noteahead {

class ThemeService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

    Q_PROPERTY(QColor lineNumberColumnBackgroundColor READ lineNumberColumnBackgroundColor CONSTANT)
    Q_PROPERTY(QColor lineNumberColumnBorderColor READ lineNumberColumnBorderColor CONSTANT)
    Q_PROPERTY(QColor lineNumberColumnCellBackgroundColor READ lineNumberColumnCellBackgroundColor CONSTANT)
    Q_PROPERTY(QColor lineNumberColumnCellBorderColor READ lineNumberColumnCellBorderColor CONSTANT)
    Q_PROPERTY(QColor lineNumberColumnOverflowTextColor READ lineNumberColumnOverflowTextColor CONSTANT)
    Q_PROPERTY(QColor mainMenuTextColor READ mainMenuTextColor CONSTANT)
    Q_PROPERTY(QColor mainToolBarGradientStartColor READ mainToolBarGradientStartColor CONSTANT)
    Q_PROPERTY(QColor mainToolBarGradientStopColor READ mainToolBarGradientStopColor CONSTANT)
    Q_PROPERTY(QColor mainToolBarSeparatorColor READ mainToolBarSeparatorColor CONSTANT)
    Q_PROPERTY(QColor mainToolBarTextColor READ mainToolBarTextColor CONSTANT)
    Q_PROPERTY(QColor noteColumnBackgroundColor READ noteColumnBackgroundColor CONSTANT)
    Q_PROPERTY(QColor noteColumnBorderColor READ noteColumnBorderColor CONSTANT)
    Q_PROPERTY(QColor noteColumnCellBackgroundColor READ noteColumnCellBackgroundColor CONSTANT)
    Q_PROPERTY(QColor noteColumnCellBorderColor READ noteColumnCellBorderColor CONSTANT)
    Q_PROPERTY(QColor noteColumnTextColor READ noteColumnTextColor CONSTANT)
    Q_PROPERTY(QColor noteColumnTextColorEmpty READ noteColumnTextColorEmpty CONSTANT)
    Q_PROPERTY(QColor positionBarBorderColor READ positionBarBorderColor CONSTANT)
    Q_PROPERTY(QColor positionBarBorderColorEditMode READ positionBarBorderColorEditMode CONSTANT)
    Q_PROPERTY(QColor progressBarBackgroundColor READ progressBarBackgroundColor CONSTANT)
    Q_PROPERTY(QColor recentFileItemTextColor READ recentFileItemTextColor CONSTANT)
    Q_PROPERTY(QColor trackBorderColor READ trackBorderColor CONSTANT)
    Q_PROPERTY(QColor trackHeaderBackgroundColor READ trackHeaderBackgroundColor CONSTANT)
    Q_PROPERTY(QColor trackHeaderBorderColor READ trackHeaderBorderColor CONSTANT)

    Q_PROPERTY(QVariantList trackHeaderTextColors READ trackHeaderTextColors NOTIFY trackHeaderTextColorsChanged)

public:
    ThemeService();
    ~ThemeService() override;

    QColor accentColor() const;
    void setAccentColor(const QColor & accentColor);

    QColor lineNumberColumnBackgroundColor() const;
    QColor lineNumberColumnBorderColor() const;
    QColor lineNumberColumnCellBackgroundColor() const;
    QColor lineNumberColumnCellBorderColor() const;
    QColor lineNumberColumnOverflowTextColor() const;
    QColor mainMenuTextColor() const;
    QColor mainToolBarGradientStartColor() const;
    QColor mainToolBarGradientStopColor() const;
    QColor mainToolBarSeparatorColor() const;
    QColor mainToolBarTextColor() const;
    QColor noteColumnBackgroundColor() const;
    QColor noteColumnBorderColor() const;
    QColor noteColumnCellBackgroundColor() const;
    QColor noteColumnCellBorderColor() const;
    QColor noteColumnTextColor() const;
    QColor noteColumnTextColorEmpty() const;
    QColor positionBarBorderColor() const;
    QColor positionBarBorderColorEditMode() const;
    QColor progressBarBackgroundColor() const;
    QColor recentFileItemTextColor() const;
    QColor trackBorderColor() const;
    QColor trackHeaderBackgroundColor() const;
    QColor trackHeaderBorderColor() const;

    QVariantList trackHeaderTextColors() const;

    Q_INVOKABLE QColor trackHeaderTextColor(int trackIndex) const;

signals:
    void accentColorChanged();
    void trackHeaderTextColorsChanged();

private:
    QColor m_accentColor;
};

} // namespace noteahead

#endif // THEME_SERVICE_HPP
