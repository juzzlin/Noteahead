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

#ifndef KEYBOARD_SERVICE_HPP
#define KEYBOARD_SERVICE_HPP

#include <QObject>

#include <memory>
#include <optional>

namespace noteahead {

class ApplicationService;
class EditorService;
class PlayerService;
class SelectionService;
class SettingsService;

class KeyboardService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int activeOctave READ activeOctave WRITE setActiveOctave NOTIFY activeOctaveChanged)

public:
    explicit KeyboardService(std::shared_ptr<ApplicationService> applicationService,
                             std::shared_ptr<EditorService> editorService,
                             std::shared_ptr<PlayerService> playerService,
                             std::shared_ptr<SelectionService> selectionService,
                             std::shared_ptr<SettingsService> settingsService,
                             QObject * parent = nullptr);

    Q_INVOKABLE bool handleKeyPressed(int key, int modifiers, bool isAutoRepeat);
    Q_INVOKABLE bool handleKeyReleased(int key, int modifiers, bool isAutoRepeat);

    int activeOctave() const;
    void setActiveOctave(int octave);

signals:
    void activeOctaveChanged(int activeOctave);

private:
    void handleScrollCommon(int modifiers, int scrollAmount);
    void handleUp(int modifiers);
    void handleDown(int modifiers);
    void handleLeft(int modifiers);
    void handleRight(int modifiers);
    void handlePageUp(int modifiers);
    void handlePageDown(int modifiers);
    void handleTab();
    void handleDelete();
    void handleEscape();
    void handleHome();
    void handleInsert();
    void handleBackspace();
    void handleSpace();
    void handleNoteOff();
    void handleNoteTriggered(int key, int modifiers, bool isAutoRepeat);
    void handleLiveNoteReleased(int key);

    struct NoteInfo {
        uint8_t note = 0;
        uint8_t octave = 0;
    };

    std::optional<NoteInfo> keyToNote(int key) const;
    std::optional<int> keyToDigit(int key) const;
    std::optional<uint8_t> effectiveNote(int key) const;
    std::optional<uint8_t> effectiveOctave(int key) const;

    std::shared_ptr<ApplicationService> m_applicationService;
    std::shared_ptr<EditorService> m_editorService;
    std::shared_ptr<PlayerService> m_playerService;
    std::shared_ptr<SelectionService> m_selectionService;
    std::shared_ptr<SettingsService> m_settingsService;

    int m_activeOctave = 3;
};

} // namespace noteahead

#endif // KEYBOARD_SERVICE_HPP
