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

#include "selection_service.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

namespace noteahead {

static const auto TAG = "SelectionService";

SelectionService::SelectionService() = default;

size_t SelectionService::minLine() const
{
    const auto selectedPositions = this->selectedPositions();
    const auto minLineIt = std::ranges::min_element(selectedPositions,
                                                    [](const auto & a, const auto & b) { return a.line < b.line; });
    return (minLineIt != selectedPositions.end()) ? minLineIt->line : 0;
}

size_t SelectionService::maxLine() const
{
    const auto selectedPositions = this->selectedPositions();
    const auto maxLineIt = std::ranges::max_element(selectedPositions,
                                                    [](const auto & a, const auto & b) { return a.line < b.line; });
    return (maxLineIt != selectedPositions.end()) ? maxLineIt->line : 0;
}

size_t SelectionService::track() const
{
    return m_startPosition.has_value() ? m_startPosition->track : 0;
}

SelectionService::PositionList SelectionService::selectedPositions() const
{
    PositionList positions;

    if (isValidSelection()) {
        auto start = *m_startPosition;
        auto end = *m_endPosition;
        for (size_t column = std::min(start.column, end.column); column <= std::max(start.column, end.column); column++) {
            for (size_t line = std::min(start.line, end.line); line <= std::max(start.line, end.line); line++) {
                positions.push_back({ start.pattern, start.track, column, line, start.lineColumn });
            }
        }
    }

    return positions;
}

bool SelectionService::isValidSelection() const
{
    if (!m_startPosition.has_value() || !m_endPosition.has_value()) {
        return false;
    }

    if (m_startPosition->pattern != m_endPosition->pattern) {
        return false;
    }

    if (m_startPosition->track != m_endPosition->track) {
        return false;
    }

    return true;
}

bool SelectionService::isSelected(size_t pattern, size_t track, size_t column, size_t line) const
{
    return std::ranges::any_of(selectedPositions(), [&](const Position & pos) {
        return pos.pattern == pattern && pos.track == track && pos.column == column && pos.line == line;
    });
}

bool SelectionService::requestSelectionStart(size_t pattern, size_t track, size_t column, size_t line)
{
    juzzlin::L(TAG).debug() << "Requesting selection start";

    if (m_startPosition.has_value()) {
        return false;
    }

    const Position position = { pattern, track, column, line };
    m_startPosition = position;
    juzzlin::L(TAG).debug() << "New selection start: " << position.toString();
    emit isValidSelectionChanged();
    return true;
}

bool SelectionService::requestSelectionEnd(size_t pattern, size_t track, size_t column, size_t line)
{
    juzzlin::L(TAG).debug() << "Requesting selection end";

    if (m_startPosition.has_value() && pattern == m_startPosition->pattern && track == m_startPosition->track) {
        const Position position = { pattern, track, column, line };
        auto oldEndPosition = m_endPosition;
        m_endPosition = position;
        juzzlin::L(TAG).info() << "New selection end: " << position.toString();
        if (oldEndPosition.has_value()) {
            emit selectionCleared(*m_startPosition, *oldEndPosition);
        }
        emit selectionChanged(*m_startPosition, *m_endPosition);
        emit isValidSelectionChanged();
        return true;
    }

    return false;
}

void SelectionService::clear()
{
    if (isValidSelection()) {
        juzzlin::L(TAG).debug() << "Clear";
        const auto prevStart = m_startPosition;
        const auto prevEnd = m_endPosition;
        m_startPosition.reset();
        m_endPosition.reset();
        if (prevStart.has_value() && prevEnd.has_value()) {
            emit selectionCleared(*prevStart, *prevEnd);
        }
        emit isValidSelectionChanged();
    }
}

} // namespace noteahead
