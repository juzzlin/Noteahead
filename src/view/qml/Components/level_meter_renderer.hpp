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
#ifndef LEVEL_METER_RENDERER_HPP
#define LEVEL_METER_RENDERER_HPP

#include <QColor>
#include <QQuickPaintedItem>
#include <QString>

#include <optional>
#include <vector>

namespace noteahead {

//! A row of level bars against one scale, as the LUFS and dBTP meters' panel elements draw them.
//! Holds no meter: whoever reads one hands the readings in, and the bars are redrawn only when
//! they change.
class LevelMeterRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

public:
    struct Scale
    {
        double min = -60.0;
        double max = 0.0;
        //! Where a bar turns from the accent to the warning colour, and from that to the danger one
        double caution = -12.0;
        double danger = -6.0;
        std::vector<double> ticks;
        QString unit;

        bool operator==(const Scale &) const = default;
    };

    struct Bar
    {
        QString label;
        double value = -70.0;
        //! A peak hold, drawn as a line and given as the number, when the meter keeps one
        std::optional<double> hold;

        bool operator==(const Bar &) const = default;
    };

    explicit LevelMeterRenderer(QQuickItem * parent = nullptr);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    const Scale & scale() const;
    void setScale(const Scale & scale);

    const std::vector<Bar> & bars() const;
    void setBars(const std::vector<Bar> & bars);

    void paint(QPainter * painter) override;

signals:
    void accentColorChanged();

private:
    double fraction(double value) const;
    QColor colorOf(double value) const;
    QString textOf(double value) const;

    QColor m_accentColor { "#00aaff" };
    Scale m_scale;
    std::vector<Bar> m_bars;
};

} // namespace noteahead

#endif // LEVEL_METER_RENDERER_HPP
