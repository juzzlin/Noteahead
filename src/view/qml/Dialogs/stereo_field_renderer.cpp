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

#include "stereo_field_renderer.hpp"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace noteahead {

namespace {

// Brighter than a passive readout would be: this one is meant to be read at a glance while a mix
// is playing, not studied.
const QColor backgroundColor { 17, 17, 17 };
const QColor guideColor { 130, 130, 130 };
const QColor guideFaintColor { 72, 72, 72 };
const QColor labelColor { 205, 205, 205 };
const QColor warningColor { 224, 78, 62 };
const QColor cautionColor { 232, 176, 64 };

//! Where the amplitude rings sit, in dB below full scale.
constexpr std::array<double, 3> ringDbs { -6.0, -12.0, -18.0 };

//! Floor of the level bars, in dB.
constexpr double levelFloorDb = -60.0;

//! Below this a pair is losing enough to a mono fold-down to be worth saying so about.
constexpr double phaseRiskCorrelation = 0.0;

//! How far past a boundary the reading has to travel before the word is allowed to change.
constexpr double classHysteresis = 0.03;

//! What the field is called, by the lowest correlation that still counts as being in the class.
//! Ordered from the widest to the narrowest, which is from the lowest correlation to the highest.
struct WidthClass
{
    double lowerBound;
    const char * name;
    const char * explanation;
};

constexpr std::array<WidthClass, 6> widthClasses {
    { { -1.0, QT_TRANSLATE_NOOP("StereoFieldRenderer", "PHASE RISK"), QT_TRANSLATE_NOOP("StereoFieldRenderer", "much of this cancels when the mix is summed to mono") },
      { 0.0, QT_TRANSLATE_NOOP("StereoFieldRenderer", "VERY WIDE"), QT_TRANSLATE_NOOP("StereoFieldRenderer", "close to the phase axis -- worth checking this one in mono") },
      { 0.2, QT_TRANSLATE_NOOP("StereoFieldRenderer", "WIDE"), QT_TRANSLATE_NOOP("StereoFieldRenderer", "clearly wide, and it still survives being summed to mono") },
      { 0.5, QT_TRANSLATE_NOOP("StereoFieldRenderer", "MEDIUM"), QT_TRANSLATE_NOOP("StereoFieldRenderer", "a natural stereo spread with room either way") },
      { 0.8, QT_TRANSLATE_NOOP("StereoFieldRenderer", "NARROW"), QT_TRANSLATE_NOOP("StereoFieldRenderer", "mostly centred -- there is little width here to lose") },
      { 0.95, QT_TRANSLATE_NOOP("StereoFieldRenderer", "MONO"), QT_TRANSLATE_NOOP("StereoFieldRenderer", "both channels are carrying the same signal") } }
};

//! The class the correlation falls in with no regard for where it has been.
int rawWidthClass(double correlation)
{
    int index = 0;
    for (int i = 0; i < static_cast<int>(widthClasses.size()); i++) {
        if (correlation >= widthClasses[static_cast<size_t>(i)].lowerBound) {
            index = i;
        }
    }
    return index;
}

constexpr int labelFontSize = 13;
constexpr int valueFontSize = 15;
constexpr int legendFontSize = 12;
constexpr int headingFontSize = 20;

double dbToRadiusFraction(double db)
{
    return std::pow(10.0, db / 20.0);
}

void setFontSize(QPainter * painter, int size, bool bold = false)
{
    auto font = painter->font();
    font.setPixelSize(size);
    font.setBold(bold);
    painter->setFont(font);
}

void drawCentredLabel(QPainter * painter, const QPointF & centre, const QString & text, const QColor & color)
{
    const QFontMetricsF metrics { painter->font() };
    const auto bounds = metrics.boundingRect(text);
    painter->setPen(color);
    painter->drawText(QPointF { centre.x() - bounds.width() / 2.0, centre.y() + bounds.height() / 3.0 }, text);
}

} // namespace

StereoFieldRenderer::StereoFieldRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
}

QVariantList StereoFieldRenderer::points() const
{
    return m_points;
}

void StereoFieldRenderer::setPoints(const QVariantList & points)
{
    m_points = points;
    emit pointsChanged();
    update();
}

qreal StereoFieldRenderer::correlation() const
{
    return m_correlation;
}

void StereoFieldRenderer::setCorrelation(qreal correlation)
{
    const int widthClass = nextWidthClass(m_widthClass, correlation);
    if (!qFuzzyCompare(m_correlation, correlation) || widthClass != m_widthClass) {
        m_correlation = correlation;
        m_widthClass = widthClass;
        emit correlationChanged();
        update();
    }
}

QVariantList StereoFieldRenderer::bandCorrelations() const
{
    return m_bandCorrelations;
}

void StereoFieldRenderer::setBandCorrelations(const QVariantList & correlations)
{
    m_bandCorrelations = correlations;
    for (size_t i = 0; i < m_bandPhaseRisk.size(); i++) {
        const double value = i < static_cast<size_t>(correlations.size()) ? correlations.at(static_cast<int>(i)).toDouble() : 1.0;
        m_bandPhaseRisk[i] = nextPhaseRisk(m_bandPhaseRisk[i], value);
    }
    emit bandCorrelationsChanged();
    update();
}

qreal StereoFieldRenderer::midDb() const
{
    return m_midDb;
}

void StereoFieldRenderer::setMidDb(qreal db)
{
    if (!qFuzzyCompare(m_midDb, db)) {
        m_midDb = db;
        emit midDbChanged();
        update();
    }
}

qreal StereoFieldRenderer::sideDb() const
{
    return m_sideDb;
}

void StereoFieldRenderer::setSideDb(qreal db)
{
    if (!qFuzzyCompare(m_sideDb, db)) {
        m_sideDb = db;
        emit sideDbChanged();
        update();
    }
}

qreal StereoFieldRenderer::balance() const
{
    return m_balance;
}

void StereoFieldRenderer::setBalance(qreal balance)
{
    if (!qFuzzyCompare(m_balance, balance)) {
        m_balance = balance;
        emit balanceChanged();
        update();
    }
}

qreal StereoFieldRenderer::zoom() const
{
    return m_zoom;
}

void StereoFieldRenderer::setZoom(qreal zoom)
{
    if (!qFuzzyCompare(m_zoom, zoom)) {
        m_zoom = zoom;
        emit zoomChanged();
        update();
    }
}

bool StereoFieldRenderer::showGuides() const
{
    return m_showGuides;
}

void StereoFieldRenderer::setShowGuides(bool show)
{
    if (m_showGuides != show) {
        m_showGuides = show;
        emit showGuidesChanged();
        update();
    }
}

QColor StereoFieldRenderer::accentColor() const
{
    return m_accentColor;
}

void StereoFieldRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

int StereoFieldRenderer::nextWidthClass(int current, double correlation)
{
    const int raw = rawWidthClass(correlation);
    if (raw == current) {
        return current;
    }

    // Staying put costs nothing; leaving costs the reading a margin past whichever end of the
    // current class it is trying to leave by.
    const size_t index = static_cast<size_t>(current);
    const double lower = widthClasses[index].lowerBound - classHysteresis;
    const double upper = index + 1 < widthClasses.size()
      ? widthClasses[index + 1].lowerBound + classHysteresis
      : std::numeric_limits<double>::max();

    return correlation >= lower && correlation < upper ? current : raw;
}

bool StereoFieldRenderer::nextPhaseRisk(bool current, double correlation)
{
    return current
      ? correlation < phaseRiskCorrelation + classHysteresis
      : correlation < phaseRiskCorrelation;
}

QString StereoFieldRenderer::verdictText() const
{
    return tr(widthClasses[static_cast<size_t>(m_widthClass)].name);
}

QString StereoFieldRenderer::explanation() const
{
    return tr(widthClasses[static_cast<size_t>(m_widthClass)].explanation);
}

QColor StereoFieldRenderer::verdictColor() const
{
    if (m_widthClass == 0) {
        return warningColor;
    }
    // The class next to the phase axis is called out too, in amber rather than red: it is a thing
    // to check, not a thing that is already wrong.
    if (m_widthClass == 1) {
        return cautionColor;
    }
    return m_accentColor;
}

QColor StereoFieldRenderer::barColor(bool phaseRisk) const
{
    return phaseRisk ? warningColor : m_accentColor;
}

void StereoFieldRenderer::paintGoniometerGuides(QPainter * painter, const QRectF & rect, double radius) const
{
    const auto centre = rect.center();

    // Trouble lies along the horizontal axis, not below the centre: a pair in phase traces the
    // vertical line through both of its half cycles, so shading the lower half would condemn
    // perfectly good mono material. What cancels in mono is what approaches pure side, which is the
    // two wedges either side of the horizontal.
    const QRectF dial { centre.x() - radius, centre.y() - radius, radius * 2.0, radius * 2.0 };
    constexpr int wedgeHalfAngle = 25;
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor { warningColor.red(), warningColor.green(), warningColor.blue(), 22 });
    painter->drawPie(dial, -wedgeHalfAngle * 16, wedgeHalfAngle * 2 * 16);
    painter->drawPie(dial, (180 - wedgeHalfAngle) * 16, wedgeHalfAngle * 2 * 16);
    painter->setBrush(Qt::NoBrush);

    // Amplitude rings, so distance from the centre can be read as a level rather than guessed.
    painter->setPen(QPen { guideFaintColor, 1.0 });
    for (const double db : ringDbs) {
        const double ringRadius = radius * dbToRadiusFraction(db);
        painter->drawEllipse(centre, ringRadius, ringRadius);
    }

    painter->setPen(QPen { guideColor, 1.0 });
    painter->drawEllipse(centre, radius, radius);

    // The four axes. Mono runs up, pure side runs across, and the two channels sit on the
    // diagonals: a trace hugging one of them is a mix that has collapsed onto that channel.
    const double diagonal = radius / std::numbers::sqrt2;
    painter->drawLine(QPointF { centre.x(), centre.y() - radius }, QPointF { centre.x(), centre.y() + radius });
    painter->drawLine(QPointF { centre.x() - radius, centre.y() }, QPointF { centre.x() + radius, centre.y() });
    painter->setPen(QPen { guideFaintColor, 1.0, Qt::DashLine });
    painter->drawLine(QPointF { centre.x() - diagonal, centre.y() - diagonal }, QPointF { centre.x() + diagonal, centre.y() + diagonal });
    painter->drawLine(QPointF { centre.x() + diagonal, centre.y() - diagonal }, QPointF { centre.x() - diagonal, centre.y() + diagonal });

    // Words rather than initials: the point of the graticule is that the display can be read
    // without the manual open beside it.
    setFontSize(painter, labelFontSize, true);
    const double labelInset = radius - 14.0;
    const double diagonalInset = labelInset / std::numbers::sqrt2;
    drawCentredLabel(painter, QPointF { centre.x(), centre.y() - labelInset }, tr("MONO"), labelColor);
    drawCentredLabel(painter, QPointF { centre.x() - labelInset + 4.0, centre.y() }, tr("SIDE"), labelColor);
    drawCentredLabel(painter, QPointF { centre.x() + labelInset - 4.0, centre.y() }, tr("SIDE"), labelColor);
    drawCentredLabel(painter, QPointF { centre.x() - diagonalInset, centre.y() - diagonalInset }, tr("L"), labelColor);
    drawCentredLabel(painter, QPointF { centre.x() + diagonalInset, centre.y() - diagonalInset }, tr("R"), labelColor);

    // Named inside the wedge it belongs to, rather than somewhere the trace has to be read around.
    setFontSize(painter, legendFontSize, false);
    drawCentredLabel(painter, QPointF { centre.x() + radius * 0.56, centre.y() + radius * 0.26 }, tr("OUT OF PHASE"), warningColor);

    // The ring the middle one sits on is labelled, which is enough to scale the other two by.
    const double ringRadius = radius * dbToRadiusFraction(ringDbs[1]);
    drawCentredLabel(painter, QPointF { centre.x() + ringRadius + 20.0, centre.y() - 9.0 },
                     QStringLiteral("%1 dB").arg(static_cast<int>(ringDbs[1])), guideColor);
}

void StereoFieldRenderer::paintGoniometer(QPainter * painter, const QRectF & rect) const
{
    const auto centre = rect.center();
    const double radius = std::min(rect.width(), rect.height()) / 2.0 - 14.0;
    if (radius <= 0.0) {
        return;
    }

    if (m_showGuides) {
        paintGoniometerGuides(painter, rect, radius);
    } else {
        painter->setPen(QPen { guideFaintColor, 1.0 });
        painter->drawEllipse(centre, radius, radius);
    }

    const int count = m_points.size() / 2;
    if (count <= 0) {
        return;
    }

    // Rotated so that a pair in phase runs straight up: the eye reads a vertical smear as centred
    // and a horizontal one as out of phase far faster than it reads a diagonal one either way.
    QPainterPath path;
    bool started = false;
    for (int i = 0; i < count; i++) {
        const double left = m_points.at(i * 2).toDouble();
        const double right = m_points.at(i * 2 + 1).toDouble();
        const double x = centre.x() + (right - left) * 0.5 * radius * m_zoom;
        const double y = centre.y() - (left + right) * 0.5 * radius * m_zoom;
        if (!started) {
            path.moveTo(x, y);
            started = true;
        } else {
            path.lineTo(x, y);
        }
    }

    painter->save();
    painter->setClipRect(rect);
    painter->setPen(QPen { QColor { m_accentColor.red(), m_accentColor.green(), m_accentColor.blue(), 170 }, 1.0 });
    painter->drawPath(path);
    painter->restore();
}

void StereoFieldRenderer::paintCorrelationScale(QPainter * painter, const QRectF & rect, const QString & label, double value, bool phaseRisk, bool withLegend) const
{
    setFontSize(painter, labelFontSize, withLegend);

    const double labelWidth = 66.0;
    const double valueWidth = 54.0;
    const QRectF bar { rect.left() + labelWidth, rect.top(), rect.width() - labelWidth - valueWidth, rect.height() };
    if (bar.width() <= 0.0) {
        return;
    }

    painter->setPen(labelColor);
    painter->drawText(QRectF { rect.left(), rect.top(), labelWidth - 6.0, rect.height() }, Qt::AlignVCenter | Qt::AlignRight, label);

    // The half that will not survive a fold-down to mono is tinted, faintly enough that a bar
    // reaching the positive end does not look half-faulty.
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor { warningColor.red(), warningColor.green(), warningColor.blue(), 12 });
    painter->drawRect(QRectF { bar.left(), bar.top(), bar.width() / 2.0, bar.height() });
    painter->setBrush(Qt::NoBrush);

    if (m_showGuides) {
        // Ticks at the quarters, with the centre one drawn brighter: zero is the value that matters.
        for (int i = 0; i <= 4; i++) {
            const double x = bar.left() + bar.width() * i / 4.0;
            painter->setPen(QPen { i == 2 ? guideColor : guideFaintColor, 1.0 });
            painter->drawLine(QPointF { x, bar.top() }, QPointF { x, bar.bottom() });
        }
    }

    painter->setPen(QPen { guideFaintColor, 1.0 });
    painter->drawRect(bar);

    const double clamped = std::clamp(value, -1.0, 1.0);
    const double centreX = bar.center().x();
    const double endX = centreX + clamped * bar.width() / 2.0;
    const QRectF fill { std::min(centreX, endX), bar.top() + 2.0, std::abs(endX - centreX), bar.height() - 4.0 };

    painter->setPen(Qt::NoPen);
    painter->setBrush(barColor(phaseRisk));
    painter->drawRect(fill);
    painter->setBrush(Qt::NoBrush);

    setFontSize(painter, valueFontSize, false);
    painter->setPen(barColor(phaseRisk));
    painter->drawText(QRectF { bar.right() + 6.0, rect.top(), valueWidth - 6.0, rect.height() }, Qt::AlignVCenter | Qt::AlignLeft,
                      QString::number(clamped, 'f', 2));

    if (withLegend) {
        // Naming the ends is what turns the number into a reading.
        setFontSize(painter, legendFontSize, false);
        const QRectF legend { bar.left(), bar.bottom() + 1.0, bar.width(), 14.0 };
        painter->setPen(warningColor);
        painter->drawText(legend, Qt::AlignLeft | Qt::AlignVCenter, tr("-1 out of phase"));
        painter->setPen(guideColor);
        painter->drawText(legend, Qt::AlignHCenter | Qt::AlignVCenter, tr("0 wide"));
        painter->drawText(legend, Qt::AlignRight | Qt::AlignVCenter, tr("+1 mono"));
    }
}

void StereoFieldRenderer::paintBalance(QPainter * painter, const QRectF & rect) const
{
    setFontSize(painter, labelFontSize, false);

    const double labelWidth = 66.0;
    const double valueWidth = 54.0;
    const QRectF bar { rect.left() + labelWidth, rect.top(), rect.width() - labelWidth - valueWidth, rect.height() };
    if (bar.width() <= 0.0) {
        return;
    }

    painter->setPen(labelColor);
    painter->drawText(QRectF { rect.left(), rect.top(), labelWidth - 6.0, rect.height() }, Qt::AlignVCenter | Qt::AlignRight, tr("Balance"));

    painter->setPen(QPen { guideFaintColor, 1.0 });
    painter->drawRect(bar);

    if (m_showGuides) {
        painter->setPen(QPen { guideColor, 1.0 });
        painter->drawLine(QPointF { bar.center().x(), bar.top() }, QPointF { bar.center().x(), bar.bottom() });
        setFontSize(painter, legendFontSize, false);
        painter->setPen(guideColor);
        painter->drawText(QRectF { bar.left() + 4.0, bar.top(), 24.0, bar.height() }, Qt::AlignVCenter | Qt::AlignLeft, tr("L"));
        painter->drawText(QRectF { bar.right() - 28.0, bar.top(), 24.0, bar.height() }, Qt::AlignVCenter | Qt::AlignRight, tr("R"));
    }

    const double clamped = std::clamp(m_balance, -1.0, 1.0);
    const double centreX = bar.center().x();
    const double endX = centreX + clamped * bar.width() / 2.0;

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_accentColor);
    painter->drawRect(QRectF { std::min(centreX, endX), bar.top() + 2.0, std::max(std::abs(endX - centreX), 2.0), bar.height() - 4.0 });
    painter->setBrush(Qt::NoBrush);
}

void StereoFieldRenderer::paintLevelBar(QPainter * painter, const QRectF & rect, const QString & label, double db, const QColor & color, bool withLegend) const
{
    setFontSize(painter, labelFontSize, false);

    const double labelWidth = 66.0;
    const double valueWidth = 60.0;
    const QRectF bar { rect.left() + labelWidth, rect.top(), rect.width() - labelWidth - valueWidth, rect.height() };
    if (bar.width() <= 0.0) {
        return;
    }

    painter->setPen(labelColor);
    painter->drawText(QRectF { rect.left(), rect.top(), labelWidth - 6.0, rect.height() }, Qt::AlignVCenter | Qt::AlignRight, label);

    if (m_showGuides) {
        // A tick every 12 dB, which is close enough to read by and sparse enough to stay legible.
        for (int tick = 0; tick >= static_cast<int>(levelFloorDb); tick -= 12) {
            const double x = bar.left() + bar.width() * (1.0 - tick / levelFloorDb);
            painter->setPen(QPen { guideFaintColor, 1.0 });
            painter->drawLine(QPointF { x, bar.top() }, QPointF { x, bar.bottom() });
        }
    }

    painter->setPen(QPen { guideFaintColor, 1.0 });
    painter->drawRect(bar);

    const double fraction = std::clamp((db - levelFloorDb) / (0.0 - levelFloorDb), 0.0, 1.0);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawRect(QRectF { bar.left() + 1.0, bar.top() + 2.0, std::max(bar.width() * fraction - 2.0, 0.0), bar.height() - 4.0 });
    painter->setBrush(Qt::NoBrush);

    setFontSize(painter, valueFontSize, false);
    painter->setPen(color);
    painter->drawText(QRectF { bar.right() + 6.0, rect.top(), valueWidth - 6.0, rect.height() }, Qt::AlignVCenter | Qt::AlignLeft,
                      db <= levelFloorDb ? tr("-inf") : QStringLiteral("%1").arg(db, 0, 'f', 1));

    if (withLegend) {
        setFontSize(painter, legendFontSize, false);
        painter->setPen(guideColor);
        const QRectF legend { bar.left(), bar.bottom() + 1.0, bar.width(), 14.0 };
        painter->drawText(legend, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("%1 dB").arg(static_cast<int>(levelFloorDb)));
        painter->drawText(legend, Qt::AlignRight | Qt::AlignVCenter, tr("0 dB  louder side = wider"));
    }
}

void StereoFieldRenderer::paint(QPainter * painter)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillRect(boundingRect(), backgroundColor);

    const QRectF bounds = boundingRect();
    if (bounds.width() <= 0.0 || bounds.height() <= 0.0) {
        return;
    }

    // The dial takes a square off the left and the readings take the rest, so the goniometer keeps
    // its aspect whatever the dialog is resized to.
    const double dialSide = std::min(bounds.height(), bounds.width() * 0.52);
    const QRectF dialRect { bounds.left(), bounds.top() + (bounds.height() - dialSide) / 2.0, dialSide, dialSide };
    paintGoniometer(painter, dialRect);

    const QRectF panel { dialRect.right() + 14.0, bounds.top() + 8.0, bounds.width() - dialSide - 22.0, bounds.height() - 16.0 };
    if (panel.width() <= 80.0) {
        return;
    }

    // A heading, then seven bars in four groups. The row height grows with the panel instead of
    // being fixed, and whatever is still left over is spread between the groups, so the readings
    // fill the space beside the dial rather than bunching in the middle of it.
    constexpr double legendHeight = 17.0;
    constexpr double innerGap = 4.0;
    constexpr double headingHeight = 28.0;
    constexpr double subHeadingHeight = 20.0;
    constexpr double bandScale = 0.85;
    constexpr double rowsInHeight = 4.0 + bandScale * 3.0;
    constexpr double baseGroupGap = 14.0;

    const double fixedHeight = legendHeight * 2.0 + innerGap * 3.0 + headingHeight + subHeadingHeight;
    const double rowHeight = std::clamp((panel.height() - fixedHeight - baseGroupGap * 4.0) / rowsInHeight, 14.0, 30.0);
    const double bandHeight = rowHeight * bandScale;
    const double barsHeight = rowHeight * 4.0 + bandHeight * 3.0 + fixedHeight;

    // Four gaps: under the heading and between the three groups of bars.
    const double groupGap = std::clamp((panel.height() - barsHeight) / 4.0, baseGroupGap, 40.0);
    const double blockHeight = barsHeight + groupGap * 4.0;

    double y = panel.top() + std::max((panel.height() - blockHeight) / 2.0, 0.0);

    // What the display is saying, before the numbers it is saying it with.
    setFontSize(painter, headingFontSize, true);
    painter->setPen(verdictColor());
    painter->drawText(QRectF { panel.left(), y, panel.width(), headingHeight }, Qt::AlignVCenter | Qt::AlignLeft, verdictText());
    y += headingHeight;

    setFontSize(painter, legendFontSize, false);
    painter->setPen(labelColor);
    painter->drawText(QRectF { panel.left(), y, panel.width(), subHeadingHeight }, Qt::AlignVCenter | Qt::AlignLeft, explanation());
    y += subHeadingHeight + groupGap;

    paintCorrelationScale(painter, QRectF { panel.left(), y, panel.width(), rowHeight }, tr("Corr"), m_correlation, m_widthClass == 0, true);
    y += rowHeight + legendHeight + groupGap;

    static const std::array<const char *, 3> bandNames { QT_TR_NOOP("Low"), QT_TR_NOOP("Mid"), QT_TR_NOOP("High") };
    for (size_t i = 0; i < bandNames.size(); i++) {
        const double value = i < static_cast<size_t>(m_bandCorrelations.size()) ? m_bandCorrelations.at(static_cast<int>(i)).toDouble() : 1.0;
        paintCorrelationScale(painter, QRectF { panel.left(), y, panel.width(), bandHeight }, tr(bandNames[i]), value, m_bandPhaseRisk[i], false);
        y += bandHeight + innerGap;
    }

    y += groupGap - innerGap;
    paintBalance(painter, QRectF { panel.left(), y, panel.width(), rowHeight });
    y += rowHeight + groupGap;

    paintLevelBar(painter, QRectF { panel.left(), y, panel.width(), rowHeight }, tr("Mid"), m_midDb, m_accentColor, false);
    y += rowHeight + innerGap;
    paintLevelBar(painter, QRectF { panel.left(), y, panel.width(), rowHeight }, tr("Side"), m_sideDb,
                  QColor { m_accentColor.red() * 3 / 4, m_accentColor.green() * 3 / 4, m_accentColor.blue() * 3 / 4 }, true);
}

} // namespace noteahead
