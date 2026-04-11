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

#include <QAbstractListModel>
#include <QPainter>

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
            connect(m_model, &QAbstractListModel::dataChanged, this, [this] { update(); });
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
    const qreal totalTextWidth = charWidth * 10;
    const qreal textX = (width() - totalTextWidth) / 2.0;

    for (int i = startRow; i < endRow; ++i) {
        const QModelIndex idx = m_model->index(i, 0);
        if (!idx.isValid()) {
            continue;
        }

        const bool isVirtualRow = m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::IsVirtualRow)).toBool();
        if (isVirtualRow) {
            continue;
        }

        const QColor bgColor = qvariant_cast<QColor>(m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::Color)));
        const qreal y = (i - m_scrollOffset) * rowHeight;

        // Background
        painter->fillRect(QRectF(0, y, width(), rowHeight), bgColor);

        // Text
        const QString lineText = m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::Line)).toString();
        const QString noteText = m_model->data(idx, static_cast<int>(NoteColumnModel::DataRole::Note)).toString();
        const QColor textColor = (noteText != "" && noteText != "---") ? QColor("#ffffff") : QColor("#888888");

        painter->setPen(textColor);
        const qreal textY = y + (rowHeight + fm.ascent() - fm.descent()) / 2.0;
        painter->drawText(QPointF(textX, textY), lineText);

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
            } else {
                focusX = textX + (4 + lineColumn) * charWidth;
            }

            QColor focusColor = QColor("red");
            focusColor.setAlphaF(0.5);
            painter->fillRect(QRectF(focusX, y + (rowHeight - fm.height()) / 2.0, focusWidth, fm.height()), focusColor);
        }
    }
}

} // namespace noteahead
