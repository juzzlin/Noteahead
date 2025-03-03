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

#include <optional>

#include "position.hpp"

namespace noteahead {

class SelectionService : public QObject
{
    Q_OBJECT

public:
    SelectionService();

    Q_INVOKABLE bool isSelected(size_t pattern, size_t track, size_t column, size_t line) const;
    Q_INVOKABLE bool isValidSelection() const;
    Q_INVOKABLE bool requestSelectionEnd(size_t pattern, size_t track, size_t column, size_t line);
    Q_INVOKABLE bool requestSelectionStart(size_t pattern, size_t track, size_t column, size_t line);
    Q_INVOKABLE void clear();

    using PositionList = std::vector<Position>;
    PositionList selectedPositions() const;

signals:
    void selectionChanged();

private:
    std::optional<Position> m_startPosition;
    std::optional<Position> m_endPosition;
};

} // namespace noteahead

#endif // SELECTION_SERVICE_HPP
