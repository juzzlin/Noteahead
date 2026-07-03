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

#ifndef RTA_RENDERER_HPP
#define RTA_RENDERER_HPP

#include <QQuickPaintedItem>
#include <QVariantList>

namespace noteahead {

class RtaRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QVariantList bands READ bands WRITE setBands NOTIFY bandsChanged)
    Q_PROPERTY(QVariantList bandPositions READ bandPositions WRITE setBandPositions NOTIFY bandPositionsChanged)
    Q_PROPERTY(int dbRange READ dbRange WRITE setDbRange NOTIFY dbRangeChanged)
    Q_PROPERTY(bool showPinkNoise READ showPinkNoise WRITE setShowPinkNoise NOTIFY showPinkNoiseChanged)
    Q_PROPERTY(float pinkNoiseLevel READ pinkNoiseLevel WRITE setPinkNoiseLevel NOTIFY pinkNoiseLevelChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

public:
    explicit RtaRenderer(QQuickItem * parent = nullptr);

    QVariantList bands() const;
    void setBands(const QVariantList & bands);

    QVariantList bandPositions() const;
    void setBandPositions(const QVariantList & positions);

    int dbRange() const;
    void setDbRange(int dbRange);

    bool showPinkNoise() const;
    void setShowPinkNoise(bool show);

    float pinkNoiseLevel() const;
    void setPinkNoiseLevel(float level);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    void paint(QPainter * painter) override;

signals:
    void bandsChanged();
    void bandPositionsChanged();
    void dbRangeChanged();
    void showPinkNoiseChanged();
    void pinkNoiseLevelChanged();
    void accentColorChanged();

private:
    static QColor barColor(float levelDb, float floorDb, const QColor & accent);
    static double logFreqNorm(double freq);

    QVariantList m_bands;
    QVariantList m_bandPositions; // flat: [xLo0, xHi0, xLo1, xHi1, ...]
    int m_dbRange = 60;
    bool m_showPinkNoise = true;
    float m_pinkNoiseLevel = -18.0f;
    QColor m_accentColor { 0, 180, 255 };
};

} // namespace noteahead

#endif // RTA_RENDERER_HPP
