// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef SELECTION_SERVICE_HPP
#define SELECTION_SERVICE_HPP

#include <QObject>

#include <functional>
#include <optional>
#include <vector>

#include "../position.hpp"

namespace noteahead {

class SelectionService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isValidSelection READ isValidSelection NOTIFY isValidSelectionChanged)

public:
    SelectionService();

    virtual Q_INVOKABLE bool isSelected(size_t pattern, size_t track, size_t column, size_t line) const;
    virtual Q_INVOKABLE bool isValidSelection() const;

    virtual Q_INVOKABLE bool requestSelectionEnd(size_t pattern, size_t track, size_t column, size_t line);
    virtual Q_INVOKABLE bool requestSelectionStart(size_t pattern, size_t track, size_t column, size_t line);

    virtual Q_INVOKABLE void clear();

    Q_INVOKABLE size_t minColumn() const;
    Q_INVOKABLE size_t maxColumn() const;

    Q_INVOKABLE size_t minLine() const;
    Q_INVOKABLE size_t maxLine() const;

    Q_INVOKABLE size_t track() const;

    using PositionList = std::vector<Position>;
    PositionList selectedPositions() const;

    using ColumnIndexList = std::vector<size_t>;
    //! Gives back a track's column indices in the order they are drawn.
    using ColumnOrderResolver = std::function<ColumnIndexList(size_t trackIndex)>;

    //! Teaches the selection what the columns' order on screen is.
    //!
    //! A column's index is its identity, not its place: inserting one gives it the next free index
    //! wherever it lands, so after an insert the indices no longer run left to right. A selection
    //! spans what the user dragged across, which is a range of places, and without this there is no
    //! way to tell which indices those places hold.
    //!
    //! Unset -- and for a track the resolver does not know -- the indices are taken as the order,
    //! which is what they are until something is inserted or deleted.
    void setColumnOrderResolver(ColumnOrderResolver resolver);

    //! The selected columns' indices, left to right on screen. What anything walking a selection
    //! across columns has to iterate: the indices themselves are not a range and cannot be counted
    //! through once a column has been inserted or deleted.
    Q_INVOKABLE ColumnIndexList selectedColumns() const;

signals:
    void isValidSelectionChanged();
    void selectionCleared(const Position & startPosition, const Position & endPosition);
    void selectionChanged(const Position & startPosition, const Position & endPosition);

private:
    //! The indices between the two ends of the selection, in display order, both ends included.
    ColumnIndexList selectedColumns(size_t trackIndex, size_t startColumn, size_t endColumn) const;

    ColumnOrderResolver m_columnOrderResolver;
    std::optional<Position> m_startPosition;
    std::optional<Position> m_endPosition;
};

} // namespace noteahead

#endif // SELECTION_SERVICE_HPP
