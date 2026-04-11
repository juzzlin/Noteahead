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

#ifndef NOTE_COLUMN_RENDERER_HPP
#define NOTE_COLUMN_RENDERER_HPP

#include <QQuickPaintedItem>

class QAbstractListModel;

namespace noteahead {

class NoteColumnRenderer : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel * model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(double scrollOffset READ scrollOffset WRITE setScrollOffset NOTIFY scrollOffsetChanged)
    Q_PROPERTY(int visibleLines READ visibleLines WRITE setVisibleLines NOTIFY visibleLinesChanged)

public:
    explicit NoteColumnRenderer(QQuickItem * parent = nullptr);

    QAbstractListModel * model() const;
    void setModel(QAbstractListModel * model);

    double scrollOffset() const;
    void setScrollOffset(double scrollOffset);

    int visibleLines() const;
    void setVisibleLines(int visibleLines);

    void paint(QPainter * painter) override;

signals:
    void modelChanged();
    void scrollOffsetChanged();
    void visibleLinesChanged();

private:
    QAbstractListModel * m_model { nullptr };
    double m_scrollOffset { 0.0 };
    int m_visibleLines { 0 };
};

} // namespace noteahead

#endif // NOTE_COLUMN_RENDERER_HPP
