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
#include "../../domain/tracker/pattern.hpp"
#include "../position.hpp"
#include "automation_service.hpp"

namespace noteahead {

static const auto TAG = "CopyManager";

CopyManager::CopyManager() = default;

CopyManager::PositionList CopyManager::pushSourceColumn(const Pattern & pattern, size_t trackIndex, size_t columnIndex, const AutomationService & automationService)
{
    m_copiedData.clear();
    m_copiedAutomations = {};
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
    m_copiedAutomations = automationService.automationsByColumn(pattern.index(), trackIndex, columnIndex);
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
        const auto changes = getPasteColumnChanges(*locked, trackIndex, columnIndex);
        for (const auto & [targetPosition, noteData] : changes) {
            locked->setNoteDataAtPosition(noteData, targetPosition);
            changedPositions.push_back(targetPosition);
        }
    }
    return changedPositions;
}

CopyManager::PositionList CopyManager::pushSourceTrack(const Pattern & pattern, size_t trackIndex, const AutomationService & automationService)
{
    m_copiedData.clear();
    m_copiedAutomations = {};
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).debug() << "Pushing data of pattern " << pattern.index();
    const auto lineCount = pattern.lineCount();
    juzzlin::L(TAG).info() << "Pushing data of track " << trackIndex;
    for (auto && columnIndex : pattern.columnIndices(trackIndex)) {
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
    m_copiedAutomations = automationService.automationsByTrack(pattern.index(), trackIndex);
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
        const auto changes = getPasteTrackChanges(*locked, trackIndex);
        for (const auto & [targetPosition, noteData] : changes) {
            locked->setNoteDataAtPosition(noteData, targetPosition);
            changedPositions.push_back(targetPosition);
        }
    }
    return changedPositions;
}

CopyManager::PositionList CopyManager::pushSourcePattern(const Pattern & pattern, const AutomationService & automationService)
{
    m_copiedData.clear();
    m_copiedAutomations = {};
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).info() << "Pushing data of pattern " << pattern.index();
    const auto lineCount = pattern.lineCount();
    m_sourceLineCount = lineCount;
    for (size_t trackIndex : pattern.trackIndices()) {
        juzzlin::L(TAG).debug() << "Pushing data of track " << trackIndex;
        for (auto && columnIndex : pattern.columnIndices(trackIndex)) {
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
    m_copiedAutomations = automationService.automationsByPattern(pattern.index());
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
        if (locked->lineCount() < m_sourceLineCount) {
            locked->setLineCount(m_sourceLineCount);
        }
        const auto changes = getPastePatternChanges(*locked);
        for (const auto & [targetPosition, newNoteData] : changes) {
            locked->setNoteDataAtPosition(newNoteData, targetPosition);
            changedPositions.push_back(targetPosition);
        }
    }
    return changedPositions;
}

CopyManager::PositionList CopyManager::pushSourceSelection(const Pattern & pattern, const PositionList & positions, const AutomationService & automationService)
{
    m_copiedData.clear();
    m_copiedAutomations = {};
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).info() << "Pushing selected positions on pattern " << pattern.index();
    for (const auto & position : positions) {
        if (const auto noteData = pattern.noteDataAtPosition(position)) {
            m_copiedData.push_back({ position, *noteData });
            changedPositions.push_back(position);
        } else {
            juzzlin::L(TAG).error() << "Invalid position: " << position.toString();
        }

        m_copiedAutomations.merge(automationService.automationsByLine(position.pattern, position.track, position.column, position.line));
    }
    m_mode = Mode::Selection;
    return changedPositions;
}

size_t CopyManager::sourceLineCount() const
{
    return m_sourceLineCount;
}

size_t CopyManager::getMinLineIndex() const
{
    size_t minLine = std::numeric_limits<size_t>::max();
    for (auto && [position, noteData] : m_copiedData) {
        minLine = std::min(minLine, static_cast<size_t>(position.line));
    }
    m_copiedAutomations.forEach([&minLine](auto && automation) {
        minLine = std::min(minLine, static_cast<size_t>(automation.interpolation().line0));
    });
    return minLine != std::numeric_limits<size_t>::max() ? minLine : 0;
}

size_t CopyManager::getMinColumnIndex() const
{
    size_t minColumn = std::numeric_limits<size_t>::max();
    for (auto && [position, noteData] : m_copiedData) {
        minColumn = std::min(minColumn, static_cast<size_t>(position.column));
    }
    m_copiedAutomations.forEach([&minColumn](auto && automation) {
        minColumn = std::min(minColumn, static_cast<size_t>(automation.location().column()));
    });
    return minColumn != std::numeric_limits<size_t>::max() ? minColumn : 0;
}

size_t CopyManager::getMinTrackIndex() const
{
    size_t minTrack = std::numeric_limits<size_t>::max();
    for (auto && [position, noteData] : m_copiedData) {
        minTrack = std::min(minTrack, static_cast<size_t>(position.track));
    }
    m_copiedAutomations.forEach([&minTrack](auto && automation) {
        minTrack = std::min(minTrack, static_cast<size_t>(automation.location().track()));
    });
    return minTrack != std::numeric_limits<size_t>::max() ? minTrack : 0;
}

CopyManager::PositionList CopyManager::pasteSelection(PatternW targetPattern, const Position & targetPosition)
{
    PositionList changedPositions;
    if (const auto locked = targetPattern.lock(); !locked) {
        throw std::runtime_error("Target pattern not set");
    } else if (!m_copiedData.empty()) {
        juzzlin::L(TAG).info() << "Pasting selection at " << targetPosition.toString();
        const auto changes = getPasteSelectionChanges(*locked, targetPosition);
        for (const auto & [newTarget, noteData] : changes) {
            locked->setNoteDataAtPosition(noteData, newTarget);
            changedPositions.push_back(newTarget);
        }
    }
    return changedPositions;
}

CopyManager::Mode CopyManager::mode() const
{
    return m_mode;
}

CopyManager::PasteChangeList CopyManager::getPasteColumnChanges(const Pattern & targetPattern, size_t trackIndex, size_t columnIndex) const
{
    PasteChangeList changes;
    for (const auto & [sourcePosition, noteData] : m_copiedData) {
        if (const Position targetPosition = { targetPattern.index(), trackIndex, columnIndex, sourcePosition.line, 0 }; targetPattern.hasPosition(targetPosition)) {
            changes.push_back({ targetPosition, noteData });
        }
    }
    return changes;
}

CopyManager::PasteChangeList CopyManager::getPasteTrackChanges(const Pattern & targetPattern, size_t trackIndex) const
{
    PasteChangeList changes;
    for (const auto & [sourcePosition, noteData] : m_copiedData) {
        if (const Position targetPosition = { targetPattern.index(), trackIndex, sourcePosition.column, sourcePosition.line, 0 }; targetPattern.hasPosition(targetPosition)) {
            changes.push_back({ targetPosition, noteData });
        }
    }
    return changes;
}

CopyManager::PasteChangeList CopyManager::getPastePatternChanges(const Pattern & targetPattern) const
{
    PasteChangeList changes;
    for (const auto & [sourcePosition, newNoteData] : m_copiedData) {
        const Position targetPosition = { targetPattern.index(), sourcePosition.track, sourcePosition.column, sourcePosition.line, 0 };
        if (targetPattern.hasPosition(targetPosition)) {
            changes.push_back({ targetPosition, newNoteData });
        }
    }
    return changes;
}

CopyManager::PasteChangeList CopyManager::getPasteSelectionChanges(const Pattern & targetPattern, const Position & targetPosition) const
{
    PasteChangeList changes;
    if (!m_copiedData.empty()) {
        const auto minLine = getMinLineIndex();
        const auto minColumn = getMinColumnIndex();
        const auto minTrack = getMinTrackIndex();
        for (const auto & [sourcePosition, noteData] : m_copiedData) {
            Position newTarget = targetPosition;
            newTarget.line += sourcePosition.line - minLine;
            newTarget.column += sourcePosition.column - minColumn;
            newTarget.track += sourcePosition.track - minTrack;
            if (targetPattern.hasPosition(newTarget)) {
                changes.push_back({ newTarget, noteData });
            }
        }
    }
    return changes;
}

AutomationSet CopyManager::getPasteColumnAutomationChanges(const Pattern & targetPattern, size_t trackIndex, size_t columnIndex) const
{
    auto changes = m_copiedAutomations;
    changes.forEach([&](auto && automation) {
        automation.setLocation({ targetPattern.index(), trackIndex, columnIndex });
    });
    return changes;
}

AutomationSet CopyManager::getPasteTrackAutomationChanges(const Pattern & targetPattern, size_t trackIndex) const
{
    const auto minTrack = getMinTrackIndex();
    auto changes = m_copiedAutomations;
    changes.forEach([&](auto && automation) {
        const auto trackOffset = automation.location().track() - minTrack;
        automation.setLocation({ targetPattern.index(), trackIndex + trackOffset, automation.location().column() });
    });
    return changes;
}

AutomationSet CopyManager::getPastePatternAutomationChanges(const Pattern & targetPattern) const
{
    auto changes = m_copiedAutomations;
    changes.forEach([&](auto && automation) {
        automation.setLocation({ targetPattern.index(), automation.location().track(), automation.location().column() });
    });
    return changes;
}

AutomationSet CopyManager::getPasteSelectionAutomationChanges(const Pattern & targetPattern, const Position & targetPosition) const
{
    const auto minLine = getMinLineIndex();
    const auto minColumn = getMinColumnIndex();
    const auto minTrack = getMinTrackIndex();

    auto changes = m_copiedAutomations;
    changes.forEach([&](auto && automation) {
        auto interpolation = automation.interpolation();
        const auto lineOffset = targetPosition.line - minLine;
        interpolation.line0 += lineOffset;
        interpolation.line1 += lineOffset;
        automation.setInterpolation(interpolation);

        const auto & location = automation.location();
        const auto trackOffset = location.track() - minTrack;
        const auto columnOffset = location.column() - minColumn;
        automation.setLocation({ targetPattern.index(), targetPosition.track + trackOffset, targetPosition.column + columnOffset });
    });
    return changes;
}

} // namespace noteahead
