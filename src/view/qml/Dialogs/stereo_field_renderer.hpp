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

#ifndef STEREO_FIELD_RENDERER_HPP
#define STEREO_FIELD_RENDERER_HPP

#include <QColor>
#include <QQuickPaintedItem>
#include <QVariantList>

#include <array>

class QPainter;
class QRectF;

namespace noteahead {

//! Draws the stereo field: a goniometer on the left, and the numbers that summarise it on the
//! right.
//!
//! Everything here is drawn against a graticule rather than floating free. A goniometer trace on a
//! blank square says only "wide" or "narrow"; the same trace over marked mono, side and channel
//! axes says which way the field leans and how close it is to the axis along which a mix cancels
//! in mono, which is what the display is actually for. The guides can be turned off once the shape
//! is familiar.
class StereoFieldRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    //! Recent sample pairs, flat: [l0, r0, l1, r1, ...].
    Q_PROPERTY(QVariantList points READ points WRITE setPoints NOTIFY pointsChanged)
    Q_PROPERTY(qreal correlation READ correlation WRITE setCorrelation NOTIFY correlationChanged)
    //! Low, mid and high correlation.
    Q_PROPERTY(QVariantList bandCorrelations READ bandCorrelations WRITE setBandCorrelations NOTIFY bandCorrelationsChanged)
    Q_PROPERTY(qreal midDb READ midDb WRITE setMidDb NOTIFY midDbChanged)
    Q_PROPERTY(qreal sideDb READ sideDb WRITE setSideDb NOTIFY sideDbChanged)
    Q_PROPERTY(qreal balance READ balance WRITE setBalance NOTIFY balanceChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(bool showGuides READ showGuides WRITE setShowGuides NOTIFY showGuidesChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    //! The latched width class as a word, for anything that wants to show it outside the painter.
    Q_PROPERTY(QString verdict READ verdictText NOTIFY correlationChanged)

public:
    explicit StereoFieldRenderer(QQuickItem * parent = nullptr);

    QVariantList points() const;
    void setPoints(const QVariantList & points);

    qreal correlation() const;
    void setCorrelation(qreal correlation);

    QVariantList bandCorrelations() const;
    void setBandCorrelations(const QVariantList & correlations);

    qreal midDb() const;
    void setMidDb(qreal db);

    qreal sideDb() const;
    void setSideDb(qreal db);

    qreal balance() const;
    void setBalance(qreal balance);

    qreal zoom() const;
    void setZoom(qreal zoom);

    bool showGuides() const;
    void setShowGuides(bool show);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    void paint(QPainter * painter) override;

    //! Which width class the field is in next, given which one it is in now.
    //!
    //! A correlation sitting on a boundary would otherwise flip the word thirty times a second,
    //! which is unreadable and reads as a fault in the meter rather than in the mix. A class is
    //! therefore only left once the reading has travelled a margin past the boundary it entered by.
    static int nextWidthClass(int current, double correlation);

    //! The same latching for a single band, which only has to say whether it is in trouble.
    static bool nextPhaseRisk(bool current, double correlation);

    //! What the field currently amounts to, in one word.
    QString verdictText() const;

signals:
    void pointsChanged();
    void correlationChanged();
    void bandCorrelationsChanged();
    void midDbChanged();
    void sideDbChanged();
    void balanceChanged();
    void zoomChanged();
    void showGuidesChanged();
    void accentColorChanged();

private:
    //! The dial itself: the graticule, then the trace over it.
    void paintGoniometer(QPainter * painter, const QRectF & rect) const;
    void paintGoniometerGuides(QPainter * painter, const QRectF & rect, double radius) const;

    //! A scale running from -1 to 1 with its own ticks, used for the broadband reading and once
    //! more, smaller, for each band. The legend under it names what the two ends and the centre
    //! mean, which is the difference between a number and a reading.
    void paintCorrelationScale(QPainter * painter, const QRectF & rect, const QString & label, double value, bool phaseRisk, bool withLegend) const;

    //! Where the energy sits between the channels, on a scale marked L, C and R.
    void paintBalance(QPainter * painter, const QRectF & rect) const;

    //! Level bar shared by the two decibel readings.
    void paintLevelBar(QPainter * painter, const QRectF & rect, const QString & label, double db, const QColor & color, bool withLegend) const;

    //! The current verdict as a sentence, and as the colour it and the heading are worth.
    QString explanation() const;
    QColor verdictColor() const;
    QColor barColor(bool phaseRisk) const;

    QVariantList m_points;
    QVariantList m_bandCorrelations { 1.0, 1.0, 1.0 };
    qreal m_correlation { 1.0 };
    qreal m_midDb { -100.0 };
    qreal m_sideDb { -100.0 };
    qreal m_balance { 0.0 };
    qreal m_zoom { 1.0 };
    bool m_showGuides { true };
    QColor m_accentColor { 0, 180, 255 };

    //! Latched so the heading does not chatter; see nextWidthClass(). Starts where a silent meter
    //! reads, which is dead centre.
    int m_widthClass { 5 };
    std::array<bool, 3> m_bandPhaseRisk { false, false, false };
};

} // namespace noteahead

#endif // STEREO_FIELD_RENDERER_HPP
