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

#include "keyboard_service.hpp"

#include "application_service.hpp"
#include "editor_service.hpp"
#include "player_service.hpp"
#include "selection_service.hpp"
#include "settings_service.hpp"

#include <map>

#include <QKeyEvent>

namespace noteahead {

KeyboardService::KeyboardService(std::shared_ptr<ApplicationService> applicationService,
                                 std::shared_ptr<EditorService> editorService,
                                 std::shared_ptr<PlayerService> playerService,
                                 std::shared_ptr<SelectionService> selectionService,
                                 std::shared_ptr<SettingsService> settingsService,
                                 QObject * parent)
  : QObject { parent }
  , m_applicationService { applicationService }
  , m_editorService { editorService }
  , m_playerService { playerService }
  , m_selectionService { selectionService }
  , m_settingsService { settingsService }
{
}

bool KeyboardService::handleKeyPressed(int key, int modifiers, bool isAutoRepeat)
{
    if (key == Qt::Key_Up) {
        handleUp(modifiers);
        return true;
    } else if (key == Qt::Key_Down) {
        handleDown(modifiers);
        return true;
    } else if (key == Qt::Key_Left) {
        handleLeft(modifiers);
        return true;
    } else if (key == Qt::Key_Right) {
        handleRight(modifiers);
        return true;
    } else if (key == Qt::Key_PageUp) {
        handlePageUp(modifiers);
        return true;
    } else if (key == Qt::Key_PageDown) {
        handlePageDown(modifiers);
        return true;
    } else if (key == Qt::Key_Tab) {
        handleTab();
        return true;
    } else if (key == Qt::Key_Delete) {
        handleDelete();
        return true;
    } else if (key == Qt::Key_Escape) {
        handleEscape();
        return true;
    } else if (key == Qt::Key_Home) {
        handleHome();
        return true;
    } else if (key == Qt::Key_Insert) {
        handleInsert();
        return true;
    } else if (key == Qt::Key_Backspace) {
        handleBackspace();
        return true;
    } else if (key == Qt::Key_Space) {
        handleSpace();
        return true;
    } else if (key == Qt::Key_F3) {
        setActiveOctave(activeOctave() - 1);
        return true;
    } else if (key == Qt::Key_F4) {
        setActiveOctave(activeOctave() + 1);
        return true;
    } else if (key == Qt::Key_A) {
        handleNoteOff();
        return true;
    } else if (modifiers == Qt::NoModifier) {
        handleNoteTriggered(key, modifiers, isAutoRepeat);
    }

    return false;
}

bool KeyboardService::handleKeyReleased(int key, int modifiers, bool isAutoRepeat)
{
    Q_UNUSED(modifiers)

    if (!isAutoRepeat) {
        handleLiveNoteReleased(key);
    }

    return false;
}

int KeyboardService::activeOctave() const
{
    return m_activeOctave;
}

void KeyboardService::setActiveOctave(int octave)
{
    if (octave >= 0 && octave <= 8 && m_activeOctave != octave) {
        m_activeOctave = octave;
        emit activeOctaveChanged(m_activeOctave);
    }
}

void KeyboardService::handleScrollCommon(int modifiers, int scrollAmount)
{
    if (!m_playerService->isPlaying()) {
        if (static_cast<unsigned int>(modifiers) & Qt::ShiftModifier) {
            auto position = m_editorService->position();
            m_selectionService->requestSelectionStart(position.pattern, position.track, position.column, position.line);
            m_editorService->requestScroll(scrollAmount);
            position = m_editorService->position();
            m_selectionService->requestSelectionEnd(position.pattern, position.track, position.column, position.line);
        } else {
            m_selectionService->clear();
            m_editorService->requestScroll(scrollAmount);
        }
    }
}

void KeyboardService::handleUp(int modifiers)
{
    handleScrollCommon(modifiers, -1);
}

void KeyboardService::handleDown(int modifiers)
{
    handleScrollCommon(modifiers, 1);
}

void KeyboardService::handlePageUp(int modifiers)
{
    handleScrollCommon(modifiers, -static_cast<int>(m_editorService->linesPerBeat()));
}

void KeyboardService::handlePageDown(int modifiers)
{
    handleScrollCommon(modifiers, static_cast<int>(m_editorService->linesPerBeat()));
}

void KeyboardService::handleLeft(int modifiers)
{
    if (static_cast<unsigned int>(modifiers) & Qt::ShiftModifier) {
        if (!m_playerService->isPlaying()) {
            auto position = m_editorService->position();
            m_selectionService->requestSelectionStart(position.pattern, position.track, position.column, position.line);
            m_editorService->requestColumnLeft();
            position = m_editorService->position();
            m_selectionService->requestSelectionEnd(position.pattern, position.track, position.column, position.line);
        }
    } else {
        m_selectionService->clear();
        m_editorService->requestCursorLeft();
    }
}

void KeyboardService::handleRight(int modifiers)
{
    if (static_cast<unsigned int>(modifiers) & Qt::ShiftModifier) {
        if (!m_playerService->isPlaying()) {
            auto position = m_editorService->position();
            m_selectionService->requestSelectionStart(position.pattern, position.track, position.column, position.line);
            m_editorService->requestColumnRight(true);
            position = m_editorService->position();
            m_selectionService->requestSelectionEnd(position.pattern, position.track, position.column, position.line);
        }
    } else {
        m_selectionService->clear();
        m_editorService->requestCursorRight();
    }
}

void KeyboardService::handleTab()
{
    m_selectionService->clear();
    m_editorService->requestColumnRight(false);
}

void KeyboardService::handleDelete()
{
    m_selectionService->clear();
    if (m_applicationService->editMode()) {
        if (m_editorService->isAtNoteColumn()) {
            m_editorService->requestNoteDeletionAtCurrentPosition(false);
            m_editorService->requestScroll(m_settingsService->step(1));
        } else if (m_editorService->isAtVelocityColumn()) {
            if (m_editorService->requestDigitSetAtCurrentPosition(0)) {
                m_editorService->requestScroll(m_settingsService->step(1));
            }
        }
    }
}

void KeyboardService::handleEscape()
{
    m_selectionService->clear();
    m_applicationService->setEditMode(!m_applicationService->editMode());
}

void KeyboardService::handleHome()
{
    m_selectionService->clear();
    if (!m_playerService->isPlaying()) {
        const auto position = m_editorService->position();
        m_editorService->requestPosition(position.pattern, 0, 0, 0, 0);
    }
}

void KeyboardService::handleInsert()
{
    m_selectionService->clear();
    if (m_applicationService->editMode()) {
        m_editorService->requestNoteInsertionAtCurrentPosition();
    }
}

void KeyboardService::handleBackspace()
{
    m_selectionService->clear();
    if (m_applicationService->editMode()) {
        m_editorService->requestNoteDeletionAtCurrentPosition(true);
    }
}

void KeyboardService::handleSpace()
{
    m_selectionService->clear();
    if (!m_playerService->isPlaying()) {
        m_playerService->play();
    } else {
        m_playerService->stop();
    }
}

void KeyboardService::handleNoteOff()
{
    if (m_applicationService->editMode()) {
        if (m_editorService->requestNoteOffAtCurrentPosition()) {
            m_editorService->requestScroll(m_settingsService->step(1));
        }
    }
}

void KeyboardService::handleNoteTriggered(int key, int modifiers, bool isAutoRepeat)
{
    Q_UNUSED(modifiers)
    if (m_applicationService->editMode()) {
        if (m_editorService->isAtNoteColumn()) {
            if (const auto note = effectiveNote(key)) {
                const auto octave = effectiveOctave(key);
                if (m_editorService->requestNoteOnAtCurrentPosition(*note, *octave, static_cast<uint8_t>(m_settingsService->velocity(100)))) {
                    m_editorService->requestScroll(m_settingsService->step(1));
                }
            }
            if (!isAutoRepeat) {
                if (const auto note = effectiveNote(key)) {
                    const auto octave = effectiveOctave(key);
                    m_applicationService->requestLiveNoteOn(*note, static_cast<uint8_t>(*octave), static_cast<uint8_t>(m_settingsService->velocity(100)));
                }
            }
        } else if (m_editorService->isAtVelocityColumn()) {
            if (const auto digit = keyToDigit(key)) {
                if (m_editorService->requestDigitSetAtCurrentPosition(static_cast<uint8_t>(*digit))) {
                    m_editorService->requestScroll(m_settingsService->step(1));
                }
            }
            if (!isAutoRepeat) {
                m_applicationService->requestLiveNoteOnAtCurrentPosition();
            }
        }
    } else if (!isAutoRepeat) {
        if (const auto note = effectiveNote(key)) {
            const auto octave = effectiveOctave(key);
            m_applicationService->requestLiveNoteOn(*note, static_cast<uint8_t>(*octave), static_cast<uint8_t>(m_settingsService->velocity(100)));
        }
    }
}

void KeyboardService::handleLiveNoteReleased(int key)
{
    if (const auto note = effectiveNote(key)) {
        const auto octave = effectiveOctave(key);
        m_applicationService->requestLiveNoteOff(*note, *octave);
    }
}

std::optional<KeyboardService::NoteInfo> KeyboardService::keyToNote(int key) const
{
    static const std::map<int, NoteInfo> noteMap = {
        // Lower octave starting with Z (C)
        { Qt::Key_Z, { 1, 0 } },
        { Qt::Key_S, { 2, 0 } },
        { Qt::Key_X, { 3, 0 } },
        { Qt::Key_D, { 4, 0 } },
        { Qt::Key_C, { 5, 0 } },
        { Qt::Key_V, { 6, 0 } },
        { Qt::Key_G, { 7, 0 } },
        { Qt::Key_B, { 8, 0 } },
        { Qt::Key_H, { 9, 0 } },
        { Qt::Key_N, { 10, 0 } },
        { Qt::Key_J, { 11, 0 } },
        { Qt::Key_M, { 12, 0 } },
        { Qt::Key_Comma, { 1, 1 } },
        { Qt::Key_L, { 2, 1 } },
        { Qt::Key_Period, { 3, 1 } },
        { Qt::Key_Odiaeresis, { 4, 1 } },
        { Qt::Key_Minus, { 5, 1 } },
        // Higher octave starting with Q (C)
        { Qt::Key_Q, { 1, 1 } },
        { Qt::Key_2, { 2, 1 } },
        { Qt::Key_W, { 3, 1 } },
        { Qt::Key_3, { 4, 1 } },
        { Qt::Key_E, { 5, 1 } },
        { Qt::Key_R, { 6, 1 } },
        { Qt::Key_5, { 7, 1 } },
        { Qt::Key_T, { 8, 1 } },
        { Qt::Key_6, { 9, 1 } },
        { Qt::Key_Y, { 10, 1 } },
        { Qt::Key_7, { 11, 1 } },
        { Qt::Key_U, { 12, 1 } },
        { Qt::Key_I, { 1, 2 } },
        { Qt::Key_9, { 2, 2 } },
        { Qt::Key_O, { 3, 2 } },
        { Qt::Key_0, { 4, 2 } },
        { Qt::Key_P, { 5, 2 } }
    };
    if (const auto it = noteMap.find(key); it != noteMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<int> KeyboardService::keyToDigit(int key) const
{
    static const std::map<int, int> digitMap = {
        { Qt::Key_0, 0 },
        { Qt::Key_1, 1 },
        { Qt::Key_2, 2 },
        { Qt::Key_3, 3 },
        { Qt::Key_4, 4 },
        { Qt::Key_5, 5 },
        { Qt::Key_6, 6 },
        { Qt::Key_7, 7 },
        { Qt::Key_8, 8 },
        { Qt::Key_9, 9 }
    };
    if (const auto it = digitMap.find(key); it != digitMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<uint8_t> KeyboardService::effectiveNote(int key) const
{
    if (const auto noteInfo = keyToNote(key)) {
        return noteInfo->note;
    }
    return std::nullopt;
}

std::optional<uint8_t> KeyboardService::effectiveOctave(int key) const
{
    if (const auto noteInfo = keyToNote(key)) {
        return m_activeOctave + noteInfo->octave;
    }
    return std::nullopt;
}

} // namespace noteahead
