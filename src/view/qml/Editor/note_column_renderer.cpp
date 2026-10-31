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

#include "note_column_renderer.hpp"

#include "../../../application/models/note_column_model.hpp"
#include "../../../common/constants.hpp"

#include <QAbstractListModel>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace noteahead {

NoteColumnRenderer::NoteColumnRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
}

QAbstractListModel * NoteColumnRenderer::model() const
{
    return m_model;
}

void NoteColumnRenderer::setModel(QAbstractListModel * model)
{
    if (m_model != model) {
        if (m_model) {
            disconnect(m_model, nullptr, this, nullptr);
        }
        m_model = model;
        if (m_model) {
            connect(m_model, &QAbstractListModel::dataChanged, this, [this](const QModelIndex & topLeft, const QModelIndex & bottomRight) {
                updateRows(topLeft.row(), bottomRight.row());
            });
            connect(m_model, &QAbstractListModel::modelReset, this, [this] { requestFullRepaint(); });
            connect(m_model, &QAbstractListModel::rowsInserted, this, [this] { requestFullRepaint(); });
            connect(m_model, &QAbstractListModel::rowsRemoved, this, [this] { requestFullRepaint(); });
        }
        emit modelChanged();
        requestFullRepaint();
    }
}

double NoteColumnRenderer::scrollOffset() const
{
    return m_scrollOffset;
}

void NoteColumnRenderer::setScrollOffset(double scrollOffset)
{
    if (!qFuzzyCompare(m_scrollOffset, scrollOffset)) {
        m_scrollOffset = scrollOffset;
        emit scrollOffsetChanged();
        requestFullRepaint();
    }
}

int NoteColumnRenderer::visibleLines() const
{
    return m_visibleLines;
}

void NoteColumnRenderer::setVisibleLines(int visibleLines)
{
    if (m_visibleLines != visibleLines) {
        m_visibleLines = visibleLines;
        emit visibleLinesChanged();
        requestFullRepaint();
    }
}

int NoteColumnRenderer::automationDisplayMode() const
{
    return m_automationDisplayMode;
}

void NoteColumnRenderer::setAutomationDisplayMode(int mode)
{
    if (m_automationDisplayMode != mode) {
        m_automationDisplayMode = mode;
        emit automationDisplayModeChanged();
        setHoveredCurve({});
        requestFullRepaint();
    }
}

int NoteColumnRenderer::automationCurveThicknessTenths() const
{
    return m_automationCurveThicknessTenths;
}

void NoteColumnRenderer::setAutomationCurveThicknessTenths(int tenths)
{
    if (m_automationCurveThicknessTenths != tenths) {
        m_automationCurveThicknessTenths = tenths;
        emit automationCurveThicknessTenthsChanged();
        requestFullRepaint();
    }
}

QColor NoteColumnRenderer::cursorColor() const
{
    return m_cursorColor;
}

void NoteColumnRenderer::setCursorColor(const QColor & cursorColor)
{
    if (m_cursorColor != cursorColor) {
        m_cursorColor = cursorColor;
        emit cursorColorChanged();
        requestFullRepaint();
    }
}

QColor NoteColumnRenderer::textColor() const
{
    return m_textColor;
}

void NoteColumnRenderer::setTextColor(const QColor & textColor)
{
    if (m_textColor != textColor) {
        m_textColor = textColor;
        emit textColorChanged();
        requestFullRepaint();
    }
}

QColor NoteColumnRenderer::textColorEmpty() const
{
    return m_textColorEmpty;
}

void NoteColumnRenderer::setTextColorEmpty(const QColor & textColorEmpty)
{
    if (m_textColorEmpty != textColorEmpty) {
        m_textColorEmpty = textColorEmpty;
        emit textColorEmptyChanged();
        requestFullRepaint();
    }
}

QColor NoteColumnRenderer::textColorGhost() const
{
    return m_textColorGhost;
}

void NoteColumnRenderer::setTextColorGhost(const QColor & textColorGhost)
{
    if (m_textColorGhost != textColorGhost) {
        m_textColorGhost = textColorGhost;
        emit textColorGhostChanged();
        requestFullRepaint();
    }
}

QVariantList NoteColumnRenderer::automationCurveColors() const
{
    QVariantList colors;
    for (auto && color : m_automationCurveColors) {
        colors.append(color);
    }
    return colors;
}

void NoteColumnRenderer::setAutomationCurveColors(const QVariantList & automationCurveColors)
{
    QList<QColor> colors;
    for (auto && color : automationCurveColors) {
        colors.append(qvariant_cast<QColor>(color));
    }
    if (m_automationCurveColors != colors) {
        m_automationCurveColors = colors;
        emit automationCurveColorsChanged();
        requestFullRepaint();
    }
}

QColor NoteColumnRenderer::automationCurveCenterLineColor() const
{
    return m_automationCurveCenterLineColor;
}

void NoteColumnRenderer::setAutomationCurveCenterLineColor(const QColor & automationCurveCenterLineColor)
{
    if (m_automationCurveCenterLineColor != automationCurveCenterLineColor) {
        m_automationCurveCenterLineColor = automationCurveCenterLineColor;
        emit automationCurveCenterLineColorChanged();
        requestFullRepaint();
    }
}

void NoteColumnRenderer::requestFullRepaint()
{
    m_fullRepaintPending = true;
    update();
}

void NoteColumnRenderer::updateRows(int firstRow, int lastRow)
{
    // No row geometry to clip against yet: fall back to repainting everything
    if (m_visibleLines <= 0 || height() <= 0.0 || width() <= 0.0) {
        requestFullRepaint();
        return;
    }

    // A full repaint is still waiting to be painted. QQuickPaintedItem holds one dirty rect and lets
    // a later partial update replace it, so narrowing it here would leave the rest of the column
    // showing what it drew before, e.g. the lines from before a scroll offset change.
    if (m_fullRepaintPending) {
        requestFullRepaint();
        return;
    }

    if (firstRow > lastRow) {
        std::swap(firstRow, lastRow);
    }

    // Automation curves are drawn as polylines between row centres, so a changed row also
    // disturbs the segments reaching into its neighbors. Pad by one row on each side.
    const qreal rowHeight = height() / m_visibleLines;
    const qreal top = (firstRow - 1 - m_scrollOffset) * rowHeight;
    const qreal bottom = (lastRow + 2 - m_scrollOffset) * rowHeight;
    update(QRectF(0.0, top, width(), bottom - top).toAlignedRect());
}

std::vector<std::pair<size_t, size_t>> NoteColumnRenderer::valueRuns(const std::vector<std::optional<double>> & values)
{
    std::vector<std::pair<size_t, size_t>> runs;
    std::optional<size_t> first;
    for (size_t i = 0; i < values.size(); i++) {
        if (values.at(i).has_value()) {
            if (!first.has_value()) {
                first = i;
            }
        } else if (first.has_value()) {
            runs.push_back({ *first, i - 1 });
            first.reset();
        }
    }
    if (first.has_value()) {
        runs.push_back({ *first, values.size() - 1 });
    }
    return runs;
}

int NoteColumnRenderer::startRow() const
{
    return static_cast<int>(m_scrollOffset);
}

int NoteColumnRenderer::endRow() const
{
    return m_model ? std::min(startRow() + m_visibleLines + 1, m_model->rowCount()) : startRow();
}

qreal NoteColumnRenderer::rowHeight() const
{
    return m_visibleLines > 0 ? height() / m_visibleLines : 0.0;
}

qreal NoteColumnRenderer::curveMargin() const
{
    // Inset so the extremes stay visible instead of merging with the column border
    return std::min(4.0, width() / 8.0);
}

std::vector<NoteColumnRenderer::CurveGeometry> NoteColumnRenderer::automationCurveGeometry(int startRow, int endRow, qreal rowHeight) const
{
    std::vector<CurveGeometry> geometry;
    const auto model = qobject_cast<NoteColumnModel *>(m_model.data());
    if (!model || endRow <= startRow) {
        return geometry;
    }

    // Rows, not lines: the model shifts by the position bar and the offset areas hold no lines
    const auto curves = model->automationCurves(startRow, endRow);
    const qreal margin = curveMargin();
    const qreal usableWidth = std::max(1.0, width() - 2.0 * margin);
    const auto valueX = [&](double value) { return margin + value * usableWidth; };
    const auto rowY = [&](int row) { return (static_cast<double>(row) - m_scrollOffset + 0.5) * rowHeight; };

    for (const auto & curve : curves) {
        CurveGeometry shape { .id = curve.id,
                              .kind = static_cast<int>(curve.kind),
                              .isPitchBend = curve.kind == AutomationService::AutomationKind::PitchBend,
                              .isGhost = curve.isGhost,
                              .indexInPattern = curve.indexInPattern,
                              .runs = {} };
        for (auto && [first, last] : valueRuns(curve.values)) {
            QPolygonF run;
            // A run of one has no segment to draw, so a polyline would paint nothing at all. Mark it
            // with a dash across its own line instead: tied to the row height so it scales with zoom.
            if (first == last) {
                const qreal tickWidth = std::min(usableWidth, rowHeight * 0.8);
                const qreal centerX = valueX(*curve.values.at(first));
                const qreal y = rowY(startRow + static_cast<int>(first));
                run.append(QPointF(std::max(margin, centerX - tickWidth / 2.0), y));
                run.append(QPointF(std::min(margin + usableWidth, centerX + tickWidth / 2.0), y));
            } else {
                for (size_t i = first; i <= last; i++) {
                    run.append(QPointF(valueX(*curve.values.at(i)), rowY(startRow + static_cast<int>(i))));
                }
            }
            shape.runs.push_back(std::move(run));
        }
        geometry.push_back(std::move(shape));
    }
    return geometry;
}

namespace {

double distanceToSegment(const QPointF & point, const QPointF & a, const QPointF & b)
{
    const QPointF ab = b - a;
    const double lengthSquared = QPointF::dotProduct(ab, ab);
    const double t = lengthSquared > 0.0 ? std::clamp(QPointF::dotProduct(point - a, ab) / lengthSquared, 0.0, 1.0) : 0.0;
    const QPointF nearest = a + t * ab;
    return std::hypot(point.x() - nearest.x(), point.y() - nearest.y());
}

} // namespace

std::optional<NoteColumnRenderer::CurveKey> NoteColumnRenderer::curveAt(QPointF point) const
{
    if (m_automationDisplayMode != static_cast<int>(Constants::AutomationDisplayMode::Curve) || m_visibleLines <= 0) {
        return {};
    }

    // Generous enough to hit a thin trace with a mouse, and never less than the trace itself
    const double reach = std::max(6.0, static_cast<double>(m_automationCurveThicknessTenths) / 10.0);
    std::optional<std::pair<double, CurveKey>> nearest;
    const auto geometry = automationCurveGeometry(startRow(), endRow(), rowHeight());
    for (const auto & curve : geometry) {
        if (curve.isGhost) {
            continue;
        }
        for (const auto & run : curve.runs) {
            for (qsizetype i = 0; i + 1 < run.size(); i++) {
                if (const double distance = distanceToSegment(point, run.at(i), run.at(i + 1)); distance <= reach && (!nearest || distance < nearest->first)) {
                    nearest = std::pair { distance, CurveKey { curve.id, curve.kind } };
                }
            }
        }
    }
    if (!nearest) {
        return {};
    }
    return nearest->second;
}

QVariantMap NoteColumnRenderer::automationCurveAt(qreal x, qreal y) const
{
    if (const auto curve = curveAt({ x, y }); curve) {
        return { { "id", curve->id }, { "kind", curve->kind } };
    }
    return {};
}

void NoteColumnRenderer::setHoverPosition(qreal x, qreal y)
{
    setHoveredCurve(curveAt({ x, y }));
}

void NoteColumnRenderer::clearHover()
{
    setHoveredCurve({});
}

bool NoteColumnRenderer::automationCurveHovered() const
{
    return m_hoveredCurve.has_value();
}

void NoteColumnRenderer::setHoveredCurve(std::optional<CurveKey> curve)
{
    if (m_hoveredCurve != curve) {
        const bool wasHovered = m_hoveredCurve.has_value();
        m_hoveredCurve = curve;
        if (wasHovered != m_hoveredCurve.has_value()) {
            emit automationCurveHoveredChanged();
        }
        requestFullRepaint();
    }
}

void NoteColumnRenderer::paintAutomationCurves(QPainter * painter, int startRow, int endRow, qreal rowHeight)
{
    const auto & palette = m_automationCurveColors;
    const auto geometry = automationCurveGeometry(startRow, endRow, rowHeight);
    if (geometry.empty() || palette.isEmpty()) {
        return;
    }

    const qreal margin = curveMargin();
    const qreal usableWidth = std::max(1.0, width() - 2.0 * margin);

    // Pitch bend swings around a centre the eye needs to find, so mark it once behind the traces
    if (std::ranges::any_of(geometry, [](auto && curve) { return curve.isPitchBend; })) {
        QColor centerColor = m_automationCurveCenterLineColor;
        centerColor.setAlphaF(0.35);
        painter->setPen(QPen { centerColor, 1.0, Qt::DashLine });
        const qreal centerX = margin + 0.5 * usableWidth;
        painter->drawLine(QPointF(centerX, 0.0), QPointF(centerX, height()));
    }

    const double thickness = static_cast<double>(m_automationCurveThicknessTenths) / 10.0;
    const CurveGeometry * hovered = nullptr;
    painter->setRenderHint(QPainter::Antialiasing, true);
    for (const auto & curve : geometry) {
        if (!curve.isGhost && m_hoveredCurve == CurveKey { curve.id, curve.kind }) {
            hovered = &curve; // Last, so that it stays on top of any curve crossing it
            continue;
        }
        QColor color = palette.at(static_cast<int>(curve.indexInPattern % static_cast<size_t>(palette.size())));
        // A curve from the offset areas belongs to a neighboring pattern, and reads as context the
        // way the ghost rows under it do rather than competing with the pattern being edited.
        color.setAlphaF(curve.isGhost ? 0.3 : 0.75);
        painter->setPen(QPen { color, thickness });
        for (const auto & run : curve.runs) {
            painter->drawPolyline(run);
        }
    }
    if (hovered) {
        const QColor color = palette.at(static_cast<int>(hovered->indexInPattern % static_cast<size_t>(palette.size()))).lighter(150);
        painter->setPen(QPen { color, thickness * 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin });
        for (const auto & run : hovered->runs) {
            painter->drawPolyline(run);
        }
    }
    painter->setRenderHint(QPainter::Antialiasing, false);
}

void NoteColumnRenderer::paint(QPainter * painter)
{
    // Runs in the scene graph's sync phase with the GUI thread blocked, so no locking is needed.
    // Whatever was pending has been handed to the painter by now: any repaint reaching here while
    // the flag is set was requested as a full one.
    m_fullRepaintPending = false;

    if (!m_model || m_visibleLines <= 0) {
        return;
    }

    const qreal rowHeight = this->rowHeight();
    const int startRow = this->startRow();
    const int endRow = this->endRow();

    QFont font;
    font.setFamily("monospace");
    font.setPixelSize(rowHeight * 0.8);
    painter->setFont(font);

    const QFontMetricsF fm { font };
    const qreal charWidth = fm.horizontalAdvance("0");
    const qreal totalTextWidth = charWidth * 14;
    const qreal textX = (width() - totalTextWidth) / 2.0;

    for (int i = startRow; i < endRow; ++i) {
        const QModelIndex idx = m_model->index(i, 0);
        if (!idx.isValid()) {
            continue;
        }

        const bool isVirtualRow = m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::IsVirtualRow)).toBool();
        const bool isGhostRow = m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::IsGhostRow)).toBool();
        if (isVirtualRow && !isGhostRow) {
            continue;
        }

        const QColor bgColor = qvariant_cast<QColor>(m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::Color)));
        const qreal y = (i - m_scrollOffset) * rowHeight;

        // Background
        painter->fillRect(QRectF(0, y, width(), rowHeight), bgColor);

        // Text
        const QString lineText = m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::Line)).toString();
        const QString noteText = m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::Note)).toString();
        QColor rowTextColor;
        if (isGhostRow) {
            rowTextColor = m_textColorGhost; // Dim gray so the neighboring pattern reads as a subordinate glimpse
        } else {
            rowTextColor = (noteText != "" && noteText != "---") ? m_textColor : m_textColorEmpty;
        }

        painter->setPen(rowTextColor);
        const qreal textY = y + (rowHeight + fm.ascent() - fm.descent()) / 2.0;
        painter->drawText(QPointF(textX, textY), lineText);

        // Ghost rows are a non-interactive preview: never draw the focus overlay
        if (isGhostRow) {
            continue;
        }

        // Focus
        const bool isFocused = m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::IsFocused)).toBool();
        if (isFocused) {
            const int lineColumn = m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::LineColumn)).toInt();
            qreal focusX = textX;
            qreal focusWidth = charWidth;

            if (lineColumn == 0) {
                focusWidth = 3 * charWidth;
            } else if (lineColumn <= 3) {
                focusX = textX + (3 + lineColumn) * charWidth;
            } else if (lineColumn <= 5) {
                focusX = textX + (4 + lineColumn) * charWidth;
            } else {
                focusX = textX + (5 + lineColumn) * charWidth;
            }

            QColor focusColor = m_cursorColor;
            focusColor.setAlphaF(0.5); // Half-transparent, so the note under the cursor stays readable
            painter->fillRect(QRectF(focusX, y + (rowHeight - fm.height()) / 2.0, focusWidth, fm.height()), focusColor);
        }
    }

    if (m_automationDisplayMode == static_cast<int>(Constants::AutomationDisplayMode::Curve)) {
        paintAutomationCurves(painter, startRow, endRow, rowHeight);
    }
}

} // namespace noteahead
