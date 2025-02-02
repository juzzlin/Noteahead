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

#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/pattern.hpp"
#include "position.hpp"

namespace noteahead {

static const auto TAG = "CopyManager";

CopyManager::CopyManager()
{
}

CopyManager::PositionList CopyManager::pushSourcePattern(const Pattern & pattern)
{
    m_copiedData.clear();
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).info() << "Pushing data of pattern " << pattern.index();
    const auto lineCount = pattern.lineCount();
    for (size_t trackIndex = 0; trackIndex < pattern.trackCount(); trackIndex++) {
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

CopyManager::PositionList CopyManager::pastePattern(PatternS targetPattern)
{
    if (!targetPattern) {
        throw std::runtime_error("Target or source not set");
    }

    juzzlin::L(TAG).info() << "Pasting copied data on pattern " << targetPattern->index();

    PositionList changedPositions;
    for (const auto & [sourcePosition, newNoteData] : m_copiedData) {
        const Position targetPosition = { targetPattern->index(), sourcePosition.track, sourcePosition.column, sourcePosition.line, 0 };
        if (targetPattern->hasPosition(targetPosition)) {
            targetPattern->setNoteDataAtPosition(newNoteData, targetPosition);
            changedPositions.push_back(targetPosition);
        }
    }
    return changedPositions;
}

CopyManager::PositionList CopyManager::pasteTrack(PatternS targetPattern, size_t trackIndex)
{
    if (!targetPattern) {
        throw std::runtime_error("Target or source not set");
    }

    juzzlin::L(TAG).info() << "Pasting copied data on pattern " << targetPattern->index() << ", track " << trackIndex;

    PositionList changedPositions;
    for (const auto & [sourcePosition, newNoteData] : m_copiedData) {
        if (const Position targetPosition = { targetPattern->index(), trackIndex, sourcePosition.column, sourcePosition.line, 0 }; targetPattern->hasPosition(targetPosition)) {
            targetPattern->setNoteDataAtPosition(newNoteData, targetPosition);
            changedPositions.push_back(targetPosition);
        }
    }
    return changedPositions;
}

CopyManager::PositionList CopyManager::pasteColumn(PatternS targetPattern, size_t trackIndex, size_t columnIndex)
{
    if (!targetPattern) {
        throw std::runtime_error("Target or source not set");
    }

    juzzlin::L(TAG).info() << "Pasting copied data on pattern " << targetPattern->index() << ", track " << trackIndex << ", column " << columnIndex;

    PositionList changedPositions;
    for (const auto & [sourcePosition, newNoteData] : m_copiedData) {
        if (const Position targetPosition = { targetPattern->index(), trackIndex, columnIndex, sourcePosition.line, 0 }; targetPattern->hasPosition(targetPosition)) {
            targetPattern->setNoteDataAtPosition(newNoteData, targetPosition);
            changedPositions.push_back(targetPosition);
        }
    }
    return changedPositions;
}

CopyManager::Mode CopyManager::mode() const
{
    return m_mode;
}

CopyManager::~CopyManager() = default;

} // namespace noteahead
