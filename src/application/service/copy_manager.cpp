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

#include "copy_manager.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/pattern.hpp"
#include "../position.hpp"

namespace noteahead {

static const auto TAG = "CopyManager";

CopyManager::CopyManager() = default;

CopyManager::PositionList CopyManager::pushSourceColumn(const Pattern & pattern, size_t trackIndex, size_t columnIndex)
{
    m_copiedData.clear();
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).debug() << "Pushing data of pattern " << pattern.index();
    const auto lineCount = pattern.lineCount();
    juzzlin::L(TAG).debug() << "Pushing data of track " << trackIndex;
    juzzlin::L(TAG).info() << "Pushing data of column " << columnIndex;
    for (size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
        juzzlin::L(TAG).debug() << "Pushing data of line " << lineIndex;
        if (const Position position = { pattern.index(), trackIndex, columnIndex, lineIndex, 0 }; pattern.hasPosition(position)) {
            m_copiedData.push_back({ position, *pattern.noteDataAtPosition(position) });
            changedPositions.push_back(position);
        } else {
            juzzlin::L(TAG).error() << "Invalid position: " << position.toString();
        }
    }
    m_mode = Mode::Column;
    return changedPositions;
}

CopyManager::PositionList CopyManager::pasteColumn(PatternW targetPattern, size_t trackIndex, size_t columnIndex)
{
    PositionList changedPositions;
    if (const auto locked = targetPattern.lock(); !locked) {
        throw std::runtime_error("Target or source not set");
    } else {
        juzzlin::L(TAG).info() << "Pasting copied data on pattern " << locked->index() << ", track " << trackIndex << ", column " << columnIndex;
        for (const auto & [sourcePosition, noteData] : m_copiedData) {
            if (const Position targetPosition = { locked->index(), trackIndex, columnIndex, sourcePosition.line, 0 }; locked->hasPosition(targetPosition)) {
                locked->setNoteDataAtPosition(noteData, targetPosition);
                changedPositions.push_back(targetPosition);
            }
        }
    }
    return changedPositions;
}

CopyManager::PositionList CopyManager::pushSourceTrack(const Pattern & pattern, size_t trackIndex)
{
    m_copiedData.clear();
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).debug() << "Pushing data of pattern " << pattern.index();
    const auto lineCount = pattern.lineCount();
    juzzlin::L(TAG).info() << "Pushing data of track " << trackIndex;
    const auto columnCount = pattern.columnCount(trackIndex);
    for (size_t columnIndex = 0; columnIndex < columnCount; columnIndex++) {
        juzzlin::L(TAG).debug() << "Pushing data of column " << columnIndex;
        for (size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            juzzlin::L(TAG).debug() << "Pushing data of line " << lineIndex;
            if (const Position position = { pattern.index(), trackIndex, columnIndex, lineIndex, 0 }; pattern.hasPosition(position)) {
                m_copiedData.push_back({ position, *pattern.noteDataAtPosition(position) });
                changedPositions.push_back(position);
            } else {
                juzzlin::L(TAG).error() << "Invalid position: " << position.toString();
            }
        }
    }
    m_mode = Mode::Track;
    return changedPositions;
}

CopyManager::PositionList CopyManager::pasteTrack(PatternW targetPattern, size_t trackIndex)
{
    PositionList changedPositions;
    if (const auto locked = targetPattern.lock(); !locked) {
        throw std::runtime_error("Target or source not set");
    } else {
        juzzlin::L(TAG).info() << "Pasting copied data on pattern " << locked->index() << ", track " << trackIndex;
        for (const auto & [sourcePosition, noteData] : m_copiedData) {
            if (const Position targetPosition = { locked->index(), trackIndex, sourcePosition.column, sourcePosition.line, 0 }; locked->hasPosition(targetPosition)) {
                locked->setNoteDataAtPosition(noteData, targetPosition);
                changedPositions.push_back(targetPosition);
            }
        }
    }
    return changedPositions;
}

CopyManager::PositionList CopyManager::pushSourcePattern(const Pattern & pattern)
{
    m_copiedData.clear();
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).info() << "Pushing data of pattern " << pattern.index();
    const auto lineCount = pattern.lineCount();
    for (size_t trackIndex : pattern.trackIndices()) {
        juzzlin::L(TAG).debug() << "Pushing data of track " << trackIndex;
        const auto columnCount = pattern.columnCount(trackIndex);
        for (size_t columnIndex = 0; columnIndex < columnCount; columnIndex++) {
            juzzlin::L(TAG).debug() << "Pushing data of column " << columnIndex;
            for (size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
                juzzlin::L(TAG).debug() << "Pushing data of line " << lineIndex;
                if (const Position position = { pattern.index(), trackIndex, columnIndex, lineIndex, 0 }; pattern.hasPosition(position)) {
                    m_copiedData.push_back({ position, *pattern.noteDataAtPosition(position) });
                    changedPositions.push_back(position);
                } else {
                    juzzlin::L(TAG).error() << "Invalid position: " << position.toString();
                }
            }
        }
    }
    m_mode = Mode::Pattern;
    return changedPositions;
}

CopyManager::PositionList CopyManager::pastePattern(PatternW targetPattern)
{
    PositionList changedPositions;
    if (const auto locked = targetPattern.lock(); !locked) {
        throw std::runtime_error("Target or source not set");
    } else {
        juzzlin::L(TAG).info() << "Pasting copied data on pattern " << locked->index();
        for (const auto & [sourcePosition, newNoteData] : m_copiedData) {
            const Position targetPosition = { locked->index(), sourcePosition.track, sourcePosition.column, sourcePosition.line, 0 };
            if (locked->hasPosition(targetPosition)) {
                locked->setNoteDataAtPosition(newNoteData, targetPosition);
                changedPositions.push_back(targetPosition);
            }
        }
    }
    return changedPositions;
}

CopyManager::PositionList CopyManager::pushSourceSelection(const Pattern & pattern, const PositionList & positions)
{
    m_copiedData.clear();
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).info() << "Pushing selected positions on pattern " << pattern.index();
    for (const auto & position : positions) {
        if (const auto noteData = pattern.noteDataAtPosition(position)) {
            m_copiedData.push_back({ position, *noteData });
            changedPositions.push_back(position);
        } else {
            juzzlin::L(TAG).error() << "Invalid position: " << position.toString();
        }
    }
    m_mode = Mode::Selection;
    return changedPositions;
}

size_t CopyManager::getMinLineIndex() const
{
    const auto it = std::ranges::min_element(m_copiedData,
                                             [](const auto & a, const auto & b) { return a.first.line < b.first.line; });
    return it != m_copiedData.end() ? it->first.line : 0;
}

size_t CopyManager::getMinColumnIndex() const
{
    const auto it = std::ranges::min_element(m_copiedData,
                                             [](const auto & a, const auto & b) { return a.first.column < b.first.column; });
    return it != m_copiedData.end() ? it->first.column : 0;
}

CopyManager::PositionList CopyManager::pasteSelection(PatternW targetPattern, const Position & targetPosition)
{
    PositionList changedPositions;
    if (const auto locked = targetPattern.lock(); !locked) {
        throw std::runtime_error("Target pattern not set");
    } else if (!m_copiedData.empty()) {
        juzzlin::L(TAG).info() << "Pasting selection at " << targetPosition.toString();
        const auto minLine = getMinLineIndex();
        const auto minColumn = getMinColumnIndex();
        for (const auto & [sourcePosition, noteData] : m_copiedData) {
            Position newTarget = targetPosition;
            newTarget.line += sourcePosition.line - minLine;
            newTarget.column += sourcePosition.column - minColumn;
            if (locked->hasPosition(newTarget)) {
                locked->setNoteDataAtPosition(noteData, newTarget);
                changedPositions.push_back(newTarget);
            }
        }
    }
    return changedPositions;
}

CopyManager::Mode CopyManager::mode() const
{
    return m_mode;
}

} // namespace noteahead
