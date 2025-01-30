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

void CopyManager::setSourcePattern(PatternS pattern)
{
    m_sourcePattern = pattern;
}

void CopyManager::setTargetPattern(PatternS pattern)
{
    m_targetPattern = pattern;
}

CopyManager::PositionList CopyManager::pastePattern()
{
    if (!m_sourcePattern || !m_targetPattern) {
        throw std::runtime_error("Target or source not set");
    }

    juzzlin::L(TAG).info() << "Pasting pattern " << m_sourcePattern->index() << " on pattern " << m_targetPattern->index();

    if (m_sourcePattern == m_targetPattern) {
        return {};
    }

    PositionList changedPositions;
    const auto lineCount = std::min(m_sourcePattern->lineCount(), m_targetPattern->lineCount());
    for (size_t trackIndex = 0; trackIndex < m_sourcePattern->trackCount(); trackIndex++) {
        juzzlin::L(TAG).debug() << "Copying track " << trackIndex;
        const auto columnCount = m_sourcePattern->columnCount(trackIndex);
        for (size_t columnIndex = 0; columnIndex < columnCount; columnIndex++) {
            juzzlin::L(TAG).debug() << "Copying column " << columnIndex;
            for (size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
                juzzlin::L(TAG).debug() << "Copying line " << lineIndex;
                const Position sourcePosition = { m_sourcePattern->index(), trackIndex, columnIndex, lineIndex, 0 };
                const auto newNoteData = m_sourcePattern->noteDataAtPosition(sourcePosition);
                const Position targetPosition = { m_targetPattern->index(), trackIndex, columnIndex, lineIndex, 0 };
                const auto oldNoteData = m_targetPattern->noteDataAtPosition(targetPosition);
                if (newNoteData != oldNoteData) {
                    m_targetPattern->setNoteDataAtPosition(*newNoteData, targetPosition);
                    changedPositions.push_back(targetPosition);
                }
            }
        }
    }
    return changedPositions;
}

CopyManager::~CopyManager() = default;

} // namespace noteahead
