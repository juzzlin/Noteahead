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
#include "../../../application/service/theme_service.hpp"
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
            connect(m_model, &QAbstractListModel::modelReset, this, [this] { update(); });
            connect(m_model, &QAbstractListModel::rowsInserted, this, [this] { update(); });
            connect(m_model, &QAbstractListModel::rowsRemoved, this, [this] { update(); });
        }
        emit modelChanged();
        update();
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
        update();
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
        update();
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
        update();
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
        update();
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
        update();
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
        update();
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
        update();
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
        update();
    }
}

void NoteColumnRenderer::updateRows(int firstRow, int lastRow)
{
    // No row geometry to clip against yet: fall back to repainting everything
    if (m_visibleLines <= 0 || height() <= 0.0 || width() <= 0.0) {
        update();
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

void NoteColumnRenderer::paintAutomationCurves(QPainter * painter, int startRow, int endRow, qreal rowHeight)
{
    const auto model = qobject_cast<NoteColumnModel *>(m_model.data());
    if (!model || endRow <= startRow) {
        return;
    }

    // Rows, not lines: the model shifts by the position bar and the offset areas hold no lines
    const auto curves = model->automationCurves(startRow, endRow);
    if (curves.empty()) {
        return;
    }

    // Inset so the extremes stay visible instead of merging with the column border
    const qreal margin = std::min(4.0, width() / 8.0);
    const qreal usableWidth = std::max(1.0, width() - 2.0 * margin);
    const auto valueX = [&](double value) { return margin + value * usableWidth; };
    const auto rowY = [&](int row) { return (static_cast<double>(row) - m_scrollOffset + 0.5) * rowHeight; };

    const auto palette = ThemeService::automationCurveColorList();

    // Pitch bend swings around a centre the eye needs to find, so mark it once behind the traces
    if (std::ranges::any_of(curves, [](auto && curve) { return curve.isPitchBend; })) {
        QColor centerColor { "#808080" };
        centerColor.setAlphaF(0.35);
        painter->setPen(QPen { centerColor, 1.0, Qt::DashLine });
        painter->drawLine(QPointF(valueX(0.5), 0.0), QPointF(valueX(0.5), height()));
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    for (size_t curveIndex = 0; curveIndex < curves.size(); curveIndex++) {
        const auto & curve = curves.at(curveIndex);
        QColor color = palette.at(static_cast<int>(curveIndex % static_cast<size_t>(palette.size())));
        color.setAlphaF(0.75);
        painter->setPen(QPen { color, static_cast<double>(m_automationCurveThicknessTenths) / 10.0 });

        // Broken into runs, so a gap where the automation does not reach is a gap on screen too
        QPolygonF run;
        for (size_t i = 0; i < curve.values.size(); i++) {
            if (const auto & value = curve.values.at(i); value.has_value()) {
                run.append(QPointF(valueX(*value), rowY(startRow + static_cast<int>(i))));
            } else if (!run.isEmpty()) {
                painter->drawPolyline(run);
                run.clear();
            }
        }
        if (!run.isEmpty()) {
            painter->drawPolyline(run);
        }
    }
    painter->setRenderHint(QPainter::Antialiasing, false);
}

void NoteColumnRenderer::paint(QPainter * painter)
{
    if (!m_model || m_visibleLines <= 0) {
        return;
    }

    const qreal rowHeight = height() / m_visibleLines;
    const int startRow = static_cast<int>(m_scrollOffset);
    const int endRow = std::min(startRow + m_visibleLines + 1, m_model->rowCount());

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
