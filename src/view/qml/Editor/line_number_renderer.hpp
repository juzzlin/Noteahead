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

#ifndef LINE_NUMBER_RENDERER_HPP
#define LINE_NUMBER_RENDERER_HPP

#include <QQuickPaintedItem>

namespace noteahead {

class LineNumberRenderer : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int scrollOffset READ scrollOffset WRITE setScrollOffset NOTIFY scrollOffsetChanged)
    Q_PROPERTY(int visibleLines READ visibleLines WRITE setVisibleLines NOTIFY visibleLinesChanged)
    Q_PROPERTY(int currentLineCount READ currentLineCount WRITE setCurrentLineCount NOTIFY currentLineCountChanged)
    Q_PROPERTY(int linesPerBeat READ linesPerBeat WRITE setLinesPerBeat NOTIFY linesPerBeatChanged)
    Q_PROPERTY(int positionBarLine READ positionBarLine WRITE setPositionBarLine NOTIFY positionBarLineChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)

public:
    explicit LineNumberRenderer(QQuickItem * parent = nullptr);

    int scrollOffset() const;
    void setScrollOffset(int scrollOffset);

    int visibleLines() const;
    void setVisibleLines(int visibleLines);

    int currentLineCount() const;
    void setCurrentLineCount(int currentLineCount);

    int linesPerBeat() const;
    void setLinesPerBeat(int linesPerBeat);

    int positionBarLine() const;
    void setPositionBarLine(int positionBarLine);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor & color);

    QColor textColor() const;
    void setTextColor(const QColor & color);

    void paint(QPainter * painter) override;

signals:
    void scrollOffsetChanged();
    void visibleLinesChanged();
    void currentLineCountChanged();
    void linesPerBeatChanged();
    void positionBarLineChanged();
    void backgroundColorChanged();
    void textColorChanged();

private:
    int m_scrollOffset { 0 };
    int m_visibleLines { 0 };
    int m_currentLineCount { 0 };
    int m_linesPerBeat { 0 };
    int m_positionBarLine { 0 };
    QColor m_backgroundColor { Qt::black };
    QColor m_textColor { Qt::white };
};

} // namespace noteahead

#endif // LINE_NUMBER_RENDERER_HPP
