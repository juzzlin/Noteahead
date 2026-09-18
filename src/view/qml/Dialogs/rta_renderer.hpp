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

#include <cstdint>
#include <utility>
#include <vector>

namespace noteahead {

class RtaRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(int dbRange READ dbRange WRITE setDbRange NOTIFY dbRangeChanged)
    Q_PROPERTY(bool showPinkNoise READ showPinkNoise WRITE setShowPinkNoise NOTIFY showPinkNoiseChanged)
    Q_PROPERTY(float pinkNoiseLevel READ pinkNoiseLevel WRITE setPinkNoiseLevel NOTIFY pinkNoiseLevelChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

public:
    explicit RtaRenderer(QQuickItem * parent = nullptr);

    //! The levels to draw, one per bar, in dB.
    //!
    //! Taken as a vector straight from the analyzer rather than through a QML list: this is called
    //! sixty times a second on up to 128 bars, and boxing every one of them into a QVariant on the
    //! way through the engine costs more than drawing them does.
    void setBandLevels(const std::vector<float> & levels);

    //! Where the bars sit, as normalised log-frequency spans, and which layout they came from.
    void setBandLayout(const std::vector<std::pair<float, float>> & positions, uint32_t generation);

    //! The layout generation last handed to setBandLayout(), so the caller can skip sending one
    //! that has not moved.
    uint32_t layoutGeneration() const;

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
    void dbRangeChanged();
    void showPinkNoiseChanged();
    void pinkNoiseLevelChanged();
    void accentColorChanged();

private:
    static QColor barColor(float levelDb, float floorDb, const QColor & accent);
    static double logFreqNorm(double freq);

    std::vector<float> m_bands;
    std::vector<std::pair<float, float>> m_bandPositions;
    uint32_t m_layoutGeneration = 0;
    int m_dbRange = 60;
    bool m_showPinkNoise = true;
    float m_pinkNoiseLevel = -18.0f;
    QColor m_accentColor { 0, 180, 255 };
};

} // namespace noteahead

#endif // RTA_RENDERER_HPP
