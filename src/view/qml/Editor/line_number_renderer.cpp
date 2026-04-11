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

#include "line_number_renderer.hpp"

#include <QPainter>

namespace noteahead {

LineNumberRenderer::LineNumberRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
}

int LineNumberRenderer::scrollOffset() const
{
    return m_scrollOffset;
}

void LineNumberRenderer::setScrollOffset(int scrollOffset)
{
    if (m_scrollOffset != scrollOffset) {
        m_scrollOffset = scrollOffset;
        emit scrollOffsetChanged();
        update();
    }
}

int LineNumberRenderer::visibleLines() const
{
    return m_visibleLines;
}

void LineNumberRenderer::setVisibleLines(int visibleLines)
{
    if (m_visibleLines != visibleLines) {
        m_visibleLines = visibleLines;
        emit visibleLinesChanged();
        update();
    }
}

int LineNumberRenderer::currentLineCount() const
{
    return m_currentLineCount;
}

void LineNumberRenderer::setCurrentLineCount(int currentLineCount)
{
    if (m_currentLineCount != currentLineCount) {
        m_currentLineCount = currentLineCount;
        emit currentLineCountChanged();
        update();
    }
}

int LineNumberRenderer::linesPerBeat() const
{
    return m_linesPerBeat;
}

void LineNumberRenderer::setLinesPerBeat(int linesPerBeat)
{
    if (m_linesPerBeat != linesPerBeat) {
        m_linesPerBeat = linesPerBeat;
        emit linesPerBeatChanged();
        update();
    }
}

int LineNumberRenderer::positionBarLine() const
{
    return m_positionBarLine;
}

void LineNumberRenderer::setPositionBarLine(int positionBarLine)
{
    if (m_positionBarLine != positionBarLine) {
        m_positionBarLine = positionBarLine;
        emit positionBarLineChanged();
        update();
    }
}

QColor LineNumberRenderer::backgroundColor() const
{
    return m_backgroundColor;
}

void LineNumberRenderer::setBackgroundColor(const QColor & color)
{
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        emit backgroundColorChanged();
        update();
    }
}

QColor LineNumberRenderer::textColor() const
{
    return m_textColor;
}

void LineNumberRenderer::setTextColor(const QColor & color)
{
    if (m_textColor != color) {
        m_textColor = color;
        emit textColorChanged();
        update();
    }
}

void LineNumberRenderer::paint(QPainter * painter)
{
    if (m_visibleLines <= 0 || m_currentLineCount <= 0) {
        return;
    }

    const qreal rowHeight = height() / m_visibleLines;

    QFont font;
    font.setFamily("monospace");
    font.setPixelSize(rowHeight * 0.8);
    painter->setFont(font);

    const QFontMetricsF fm { font };

    const int beatLine1 = m_linesPerBeat;
    const int beatLine2 = beatLine1 % 3 ? beatLine1 / 2 : beatLine1 / 3;
    const int beatLine3 = beatLine1 % 6 ? beatLine1 / 4 : beatLine1 / 6;

    for (int i = 0; i < m_visibleLines + 1; ++i) {
        const int rowIdx = m_scrollOffset + i;
        const int lineNumber = rowIdx - m_positionBarLine;

        if (lineNumber < 0 || lineNumber >= m_currentLineCount) {
            continue;
        }

        const int wrappedLineNumber = (lineNumber % m_currentLineCount + m_currentLineCount) % m_currentLineCount;
        const qreal y = i * rowHeight;

        // Draw background
        painter->fillRect(QRectF(0, y, width(), rowHeight), m_backgroundColor);

        // Highlight logic from IndexHighlight.qml
        qreal opacity = 0;
        if (beatLine1 > 0) {
            if (!(wrappedLineNumber % beatLine1))
                opacity = 0.25;
            else if (beatLine3 > 0 && beatLine2 > 0 && !(wrappedLineNumber % beatLine3) && !(wrappedLineNumber % beatLine2))
                opacity = 0.10;
            else if (beatLine3 > 0 && !(wrappedLineNumber % beatLine3))
                opacity = 0.05;
        }

        if (opacity > 0) {
            QColor highlightColor = Qt::white;
            highlightColor.setAlphaF(opacity);
            painter->fillRect(QRectF(0, y, width(), rowHeight), highlightColor);
        }

        // Text
        const QString formattedLineNumber = wrappedLineNumber < 10 ? QString("0%1").arg(wrappedLineNumber) : QString::number(wrappedLineNumber);
        painter->setPen(m_textColor);

        const qreal textX = (width() - fm.horizontalAdvance(formattedLineNumber)) / 2.0;
        const qreal textY = y + (rowHeight + fm.ascent() - fm.descent()) / 2.0;
        painter->drawText(QPointF(textX, textY), formattedLineNumber);

        // Bottom border line to match QML border behavior (noteahead style uses dark borders)
        painter->setPen(QColor("#222222"));
        painter->drawLine(QPointF(0, y + rowHeight), QPointF(width(), y + rowHeight));
    }
}

} // namespace noteahead
