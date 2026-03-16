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
#include "automation_service.hpp"

namespace noteahead {

static const auto TAG = "CopyManager";

CopyManager::CopyManager() = default;

CopyManager::PositionList CopyManager::pushSourceColumn(const Pattern & pattern, size_t trackIndex, size_t columnIndex, const AutomationService & automationService)
{
    m_copiedData.clear();
    m_copiedMidiCcAutomations.clear();
    m_copiedPitchBendAutomations.clear();
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
    m_copiedMidiCcAutomations = automationService.midiCcAutomationsByColumn(pattern.index(), trackIndex, columnIndex);
    m_copiedPitchBendAutomations = automationService.pitchBendAutomationsByColumn(pattern.index(), trackIndex, columnIndex);
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
    m_copiedMidiCcAutomations.clear();
    m_copiedPitchBendAutomations.clear();
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
    m_copiedMidiCcAutomations = automationService.midiCcAutomationsByTrack(pattern.index(), trackIndex);
    m_copiedPitchBendAutomations = automationService.pitchBendAutomationsByTrack(pattern.index(), trackIndex);
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
    m_copiedMidiCcAutomations.clear();
    m_copiedPitchBendAutomations.clear();
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
    m_copiedMidiCcAutomations = automationService.midiCcAutomationsByPattern(pattern.index());
    m_copiedPitchBendAutomations = automationService.pitchBendAutomationsByPattern(pattern.index());
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
    m_copiedMidiCcAutomations.clear();
    m_copiedPitchBendAutomations.clear();
    CopyManager::PositionList changedPositions;
    juzzlin::L(TAG).info() << "Pushing selected positions on pattern " << pattern.index();
    for (const auto & position : positions) {
        if (const auto noteData = pattern.noteDataAtPosition(position)) {
            m_copiedData.push_back({ position, *noteData });
            changedPositions.push_back(position);
        } else {
            juzzlin::L(TAG).error() << "Invalid position: " << position.toString();
        }

        const auto midiCc = automationService.midiCcAutomationsByLine(position.pattern, position.track, position.column, position.line);
        for (auto && automation : midiCc) {
            if (std::ranges::find(m_copiedMidiCcAutomations, automation) == m_copiedMidiCcAutomations.end()) {
                m_copiedMidiCcAutomations.push_back(automation);
            }
        }

        const auto pitchBend = automationService.pitchBendAutomationsByLine(position.pattern, position.track, position.column, position.line);
        for (auto && automation : pitchBend) {
            if (std::ranges::find(m_copiedPitchBendAutomations, automation) == m_copiedPitchBendAutomations.end()) {
                m_copiedPitchBendAutomations.push_back(automation);
            }
        }
    }
    m_mode = Mode::Selection;
    return changedPositions;
}

size_t CopyManager::getMinLineIndex() const
{
    const auto it = std::ranges::min_element(m_copiedData,
                                             [](const auto & a, const auto & b) { return a.first.line < b.first.line; });
    const auto itMidiCc = std::ranges::min_element(m_copiedMidiCcAutomations,
                                                   [](const auto & a, const auto & b) { return a.interpolation().line0 < b.interpolation().line0; });
    const auto itPitchBend = std::ranges::min_element(m_copiedPitchBendAutomations,
                                                      [](const auto & a, const auto & b) { return a.interpolation().line0 < b.interpolation().line0; });

    size_t minLine = (it != m_copiedData.end() ? it->first.line : std::numeric_limits<size_t>::max());
    if (itMidiCc != m_copiedMidiCcAutomations.end()) {
        minLine = std::min(minLine, itMidiCc->interpolation().line0);
    }
    if (itPitchBend != m_copiedPitchBendAutomations.end()) {
        minLine = std::min(minLine, itPitchBend->interpolation().line0);
    }

    return minLine != std::numeric_limits<size_t>::max() ? minLine : 0;
}

size_t CopyManager::getMinColumnIndex() const
{
    const auto it = std::ranges::min_element(m_copiedData,
                                             [](const auto & a, const auto & b) { return a.first.column < b.first.column; });
    const auto itMidiCc = std::ranges::min_element(m_copiedMidiCcAutomations,
                                                   [](const auto & a, const auto & b) { return a.location().column() < b.location().column(); });
    const auto itPitchBend = std::ranges::min_element(m_copiedPitchBendAutomations,
                                                      [](const auto & a, const auto & b) { return a.location().column() < b.location().column(); });

    size_t minColumn = (it != m_copiedData.end() ? it->first.column : std::numeric_limits<size_t>::max());
    if (itMidiCc != m_copiedMidiCcAutomations.end()) {
        minColumn = std::min(minColumn, static_cast<size_t>(itMidiCc->location().column()));
    }
    if (itPitchBend != m_copiedPitchBendAutomations.end()) {
        minColumn = std::min(minColumn, static_cast<size_t>(itPitchBend->location().column()));
    }

    return minColumn != std::numeric_limits<size_t>::max() ? minColumn : 0;
}

size_t CopyManager::getMinTrackIndex() const
{
    const auto it = std::ranges::min_element(m_copiedData,
                                             [](const auto & a, const auto & b) { return a.first.track < b.first.track; });
    const auto itMidiCc = std::ranges::min_element(m_copiedMidiCcAutomations,
                                                   [](const auto & a, const auto & b) { return a.location().track() < b.location().track(); });
    const auto itPitchBend = std::ranges::min_element(m_copiedPitchBendAutomations,
                                                      [](const auto & a, const auto & b) { return a.location().track() < b.location().track(); });

    size_t minTrack = (it != m_copiedData.end() ? it->first.track : std::numeric_limits<size_t>::max());
    if (itMidiCc != m_copiedMidiCcAutomations.end()) {
        minTrack = std::min(minTrack, static_cast<size_t>(itMidiCc->location().track()));
    }
    if (itPitchBend != m_copiedPitchBendAutomations.end()) {
        minTrack = std::min(minTrack, static_cast<size_t>(itPitchBend->location().track()));
    }

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

CopyManager::MidiCcAutomationList CopyManager::getPasteColumnMidiCcAutomationChanges(const Pattern & targetPattern, size_t trackIndex, size_t columnIndex) const
{
    MidiCcAutomationList changes;
    for (const auto & automation : m_copiedMidiCcAutomations) {
        auto updated = automation;
        updated.setLocation({ targetPattern.index(), trackIndex, columnIndex });
        changes.push_back(updated);
    }
    return changes;
}

CopyManager::PitchBendAutomationList CopyManager::getPasteColumnPitchBendAutomationChanges(const Pattern & targetPattern, size_t trackIndex, size_t columnIndex) const
{
    PitchBendAutomationList changes;
    for (const auto & automation : m_copiedPitchBendAutomations) {
        auto updated = automation;
        updated.setLocation({ targetPattern.index(), trackIndex, columnIndex });
        changes.push_back(updated);
    }
    return changes;
}

CopyManager::MidiCcAutomationList CopyManager::getPasteTrackMidiCcAutomationChanges(const Pattern & targetPattern, size_t trackIndex) const
{
    MidiCcAutomationList changes;
    const auto minTrack = getMinTrackIndex();
    for (const auto & automation : m_copiedMidiCcAutomations) {
        auto updated = automation;
        const auto trackOffset = automation.location().track() - minTrack;
        updated.setLocation({ targetPattern.index(), trackIndex + trackOffset, automation.location().column() });
        changes.push_back(updated);
    }
    return changes;
}

CopyManager::PitchBendAutomationList CopyManager::getPasteTrackPitchBendAutomationChanges(const Pattern & targetPattern, size_t trackIndex) const
{
    PitchBendAutomationList changes;
    const auto minTrack = getMinTrackIndex();
    for (const auto & automation : m_copiedPitchBendAutomations) {
        auto updated = automation;
        const auto trackOffset = automation.location().track() - minTrack;
        updated.setLocation({ targetPattern.index(), trackIndex + trackOffset, automation.location().column() });
        changes.push_back(updated);
    }
    return changes;
}

CopyManager::MidiCcAutomationList CopyManager::getPastePatternMidiCcAutomationChanges(const Pattern & targetPattern) const
{
    MidiCcAutomationList changes;
    for (const auto & automation : m_copiedMidiCcAutomations) {
        auto updated = automation;
        updated.setLocation({ targetPattern.index(), automation.location().track(), automation.location().column() });
        changes.push_back(updated);
    }
    return changes;
}

CopyManager::PitchBendAutomationList CopyManager::getPastePatternPitchBendAutomationChanges(const Pattern & targetPattern) const
{
    PitchBendAutomationList changes;
    for (const auto & automation : m_copiedPitchBendAutomations) {
        auto updated = automation;
        updated.setLocation({ targetPattern.index(), automation.location().track(), automation.location().column() });
        changes.push_back(updated);
    }
    return changes;
}

CopyManager::MidiCcAutomationList CopyManager::getPasteSelectionMidiCcAutomationChanges(const Pattern & targetPattern, const Position & targetPosition) const
{
    MidiCcAutomationList changes;
    const auto minLine = getMinLineIndex();
    const auto minColumn = getMinColumnIndex();
    const auto minTrack = getMinTrackIndex();

    for (const auto & automation : m_copiedMidiCcAutomations) {
        auto updated = automation;
        auto location = automation.location();
        auto interpolation = automation.interpolation();

        const auto lineOffset = targetPosition.line - minLine;
        interpolation.line0 += lineOffset;
        interpolation.line1 += lineOffset;
        updated.setInterpolation(interpolation);

        const auto trackOffset = location.track() - minTrack;
        const auto columnOffset = location.column() - minColumn;
        updated.setLocation({ targetPattern.index(), targetPosition.track + trackOffset, targetPosition.column + columnOffset });

        changes.push_back(updated);
    }
    return changes;
}

CopyManager::PitchBendAutomationList CopyManager::getPasteSelectionPitchBendAutomationChanges(const Pattern & targetPattern, const Position & targetPosition) const
{
    PitchBendAutomationList changes;
    const auto minLine = getMinLineIndex();
    const auto minColumn = getMinColumnIndex();
    const auto minTrack = getMinTrackIndex();

    for (const auto & automation : m_copiedPitchBendAutomations) {
        auto updated = automation;
        auto location = automation.location();
        auto interpolation = automation.interpolation();

        const auto lineOffset = targetPosition.line - minLine;
        interpolation.line0 += lineOffset;
        interpolation.line1 += lineOffset;
        updated.setInterpolation(interpolation);

        const auto trackOffset = location.track() - minTrack;
        const auto columnOffset = location.column() - minColumn;
        updated.setLocation({ targetPattern.index(), targetPosition.track + trackOffset, targetPosition.column + columnOffset });

        changes.push_back(updated);
    }
    return changes;
}

} // namespace noteahead
