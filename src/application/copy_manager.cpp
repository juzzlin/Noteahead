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
                }
            }
        }
    }
    return changedPositions;
}

CopyManager::PositionList CopyManager::pasteCopiedData(PatternS targetPattern)
{
    if (!targetPattern) {
        throw std::runtime_error("Target or source not set");
    }

    juzzlin::L(TAG).info() << "Pasting coped data on pattern " << targetPattern->index();

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

CopyManager::PositionList CopyManager::pushSourceTrack(const Pattern & pattern, size_t trackIndex)
{
    m_copiedData.clear();
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).info() << "Pushing data of pattern " << pattern.index();
    const auto lineCount = pattern.lineCount();
    juzzlin::L(TAG).debug() << "Pushing data of track " << trackIndex;
    const auto columnCount = pattern.columnCount(trackIndex);
    for (size_t columnIndex = 0; columnIndex < columnCount; columnIndex++) {
        juzzlin::L(TAG).debug() << "Pushing data of column " << columnIndex;
        for (size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            juzzlin::L(TAG).debug() << "Pushing data of line " << lineIndex;
            if (const Position position = { pattern.index(), trackIndex, columnIndex, lineIndex, 0 }; pattern.hasPosition(position)) {
                m_copiedData.push_back({ position, *pattern.noteDataAtPosition(position) });
                changedPositions.push_back(position);
            }
        }
    }
    return changedPositions;
}

CopyManager::~CopyManager() = default;

} // namespace noteahead
