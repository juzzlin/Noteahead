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

#include "keyboard_service_test.hpp"

#include "../../application/service/keyboard_service.hpp"
#include "../../application/service/application_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/player_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../domain/midi_note_data.hpp"

#include <QSignalSpy>

namespace noteahead {

class MockApplicationService : public ApplicationService
{
public:
    bool editMode() const override { return m_editMode; }
    void setEditMode(bool editMode) override { m_editMode = editMode; emit editModeChanged(m_editMode); }
    void toggleEditMode() override { setEditMode(!m_editMode); }

    void requestLiveNoteOn(quint8, quint8, quint8) override
    {
        MidiNoteData data;
        emit liveNoteOnRequested(nullptr, data);
    }
    void requestLiveNoteOff(quint8, quint8) override
    {
        MidiNoteData data;
        emit liveNoteOffRequested(nullptr, data);
    }
    void requestLiveNoteOnAtCurrentPosition() override
    {
        emit liveNoteOnAtCurrentPositionRequested(nullptr);
    }

private:
    bool m_editMode = false;
};

class MockEditorService : public EditorService
{
public:
    Position position() const override { return m_position; }
    void setMockPosition(const Position & position) { m_position = position; }
    void requestScroll(int steps) override { m_position.line += steps; }
    quint64 linesPerBeat() const override { return 4; }
    bool isAtNoteColumn() const override { return m_isAtNoteColumn; }
    void setMockIsAtNoteColumn(bool val) { m_isAtNoteColumn = val; }
    bool isAtVelocityColumn() const override { return m_isAtVelocityColumn; }
    void setMockIsAtVelocityColumn(bool val) { m_isAtVelocityColumn = val; }
    bool isAtDelayColumn() const override { return m_isAtDelayColumn; }
    void setMockIsAtDelayColumn(bool val) { m_isAtDelayColumn = val; }
    bool requestNoteOnAtCurrentPosition(quint8, quint8, quint8) override { return true; }
    bool requestDigitSetAtCurrentPosition(quint8 digit) override
    {
        m_digitSet = digit;
        return true;
    }
    bool requestPosition(quint64, quint64, quint64, qint64, quint64) override { return true; }

    std::optional<quint8> digitSet() const { return m_digitSet; }

private:
    Position m_position;
    bool m_isAtNoteColumn = true;
    bool m_isAtVelocityColumn = false;
    bool m_isAtDelayColumn = false;
    std::optional<quint8> m_digitSet;
};

class MockPlayerService : public PlayerService
{
public:
    MockPlayerService()
      : PlayerService { nullptr, nullptr, nullptr, nullptr, nullptr }
    {
    }
    bool isPlaying() const override { return m_isPlaying; }
    bool play() override { m_isPlaying = true; emit isPlayingChanged(); return true; }
    void stop() override { m_isPlaying = false; emit isPlayingChanged(); }

private:
    bool m_isPlaying = false;
};

class MockSelectionService : public SelectionService
{
public:
    void clear() override { m_cleared = true; }
    bool cleared() const { return m_cleared; }
private:
    bool m_cleared = false;
};

class MockSettingsService : public SettingsService
{
public:
    int step(int) const override { return 1; }
    int velocity(int) const override { return 100; }
};

void KeyboardServiceTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("KeyboardServiceTest");
    qRegisterMetaType<MidiNoteData>("MidiNoteData");
    qRegisterMetaType<MidiNoteData>("MidiNoteDataCR");
}

void KeyboardServiceTest::cleanupTestCase()
{
    QSettings settings {};
    settings.clear();
}

void KeyboardServiceTest::test_activeOctave_shouldUpdateAndEmitSignal()
{
    const auto applicationService = std::make_shared<MockApplicationService>();
    const auto editorService = std::make_shared<MockEditorService>();
    const auto playerService = std::make_shared<MockPlayerService>();
    const auto selectionService = std::make_shared<MockSelectionService>();
    const auto settingsService = std::make_shared<MockSettingsService>();

    KeyboardService keyboardService { applicationService, editorService, playerService, selectionService, settingsService };

    QSignalSpy spy { &keyboardService, &KeyboardService::activeOctaveChanged };

    keyboardService.setActiveOctave(5);
    QCOMPARE(keyboardService.activeOctave(), 5);
    QCOMPARE(spy.count(), 1);
}

void KeyboardServiceTest::test_handleKeyPressed_Up_shouldScrollEditor()
{
    const auto applicationService = std::make_shared<MockApplicationService>();
    const auto editorService = std::make_shared<MockEditorService>();
    const auto playerService = std::make_shared<MockPlayerService>();
    const auto selectionService = std::make_shared<MockSelectionService>();
    const auto settingsService = std::make_shared<MockSettingsService>();

    KeyboardService keyboardService { applicationService, editorService, playerService, selectionService, settingsService };

    Position pos {};
    pos.line = 10;
    editorService->setMockPosition(pos);

    QVERIFY(keyboardService.handleKeyPressed(Qt::Key_Up, Qt::NoModifier, false));
    QCOMPARE(editorService->position().line, quint64 { 9 });
}

void KeyboardServiceTest::test_handleKeyPressed_Space_shouldTogglePlayback()
{
    const auto applicationService = std::make_shared<MockApplicationService>();
    const auto editorService = std::make_shared<MockEditorService>();
    const auto playerService = std::make_shared<MockPlayerService>();
    const auto selectionService = std::make_shared<MockSelectionService>();
    const auto settingsService = std::make_shared<MockSettingsService>();

    KeyboardService keyboardService { applicationService, editorService, playerService, selectionService, settingsService };

    QVERIFY(!playerService->isPlaying());
    QVERIFY(keyboardService.handleKeyPressed(Qt::Key_Space, Qt::NoModifier, false));
    QVERIFY(playerService->isPlaying());

    QVERIFY(keyboardService.handleKeyPressed(Qt::Key_Space, Qt::NoModifier, false));
    QVERIFY(!playerService->isPlaying());
}

void KeyboardServiceTest::test_handleKeyPressed_Escape_shouldToggleEditMode()
{
    const auto applicationService = std::make_shared<MockApplicationService>();
    const auto editorService = std::make_shared<MockEditorService>();
    const auto playerService = std::make_shared<MockPlayerService>();
    const auto selectionService = std::make_shared<MockSelectionService>();
    const auto settingsService = std::make_shared<MockSettingsService>();

    KeyboardService keyboardService { applicationService, editorService, playerService, selectionService, settingsService };

    QVERIFY(!applicationService->editMode());
    QVERIFY(keyboardService.handleKeyPressed(Qt::Key_Escape, Qt::NoModifier, false));
    QVERIFY(applicationService->editMode());

    QVERIFY(keyboardService.handleKeyPressed(Qt::Key_Escape, Qt::NoModifier, false));
    QVERIFY(!applicationService->editMode());
}

void KeyboardServiceTest::test_handleKeyPressed_Note_shouldTriggerNoteOn()
{
    const auto applicationService = std::make_shared<MockApplicationService>();
    const auto editorService = std::make_shared<MockEditorService>();
    const auto playerService = std::make_shared<MockPlayerService>();
    const auto selectionService = std::make_shared<MockSelectionService>();
    const auto settingsService = std::make_shared<MockSettingsService>();

    KeyboardService keyboardService { applicationService, editorService, playerService, selectionService, settingsService };

    applicationService->setEditMode(true);
    editorService->setMockIsAtNoteColumn(true);

    QSignalSpy spy { applicationService.get(), &ApplicationService::liveNoteOnRequested };

    // Z key is C in the current octave (default 3)
    QVERIFY(keyboardService.handleKeyPressed(Qt::Key_Z, Qt::NoModifier, false));

    QCOMPARE(spy.count(), 1);
}

void KeyboardServiceTest::test_handleKeyReleased_Note_shouldTriggerNoteOff()
{
    const auto applicationService = std::make_shared<MockApplicationService>();
    const auto editorService = std::make_shared<MockEditorService>();
    const auto playerService = std::make_shared<MockPlayerService>();
    const auto selectionService = std::make_shared<MockSelectionService>();
    const auto settingsService = std::make_shared<MockSettingsService>();

    KeyboardService keyboardService { applicationService, editorService, playerService, selectionService, settingsService };

    QSignalSpy spy { applicationService.get(), &ApplicationService::liveNoteOffRequested };

    QVERIFY(!keyboardService.handleKeyReleased(Qt::Key_Z, Qt::NoModifier, false));

    QCOMPARE(spy.count(), 1);
}

void KeyboardServiceTest::test_handleKeyPressed_Digit_shouldSetDelay_whenAtDelayColumn()
{
    const auto applicationService = std::make_shared<MockApplicationService>();
    const auto editorService = std::make_shared<MockEditorService>();
    const auto playerService = std::make_shared<MockPlayerService>();
    const auto selectionService = std::make_shared<MockSelectionService>();
    const auto settingsService = std::make_shared<MockSettingsService>();

    KeyboardService keyboardService { applicationService, editorService, playerService, selectionService, settingsService };

    applicationService->setEditMode(true);
    editorService->setMockIsAtNoteColumn(false);
    editorService->setMockIsAtDelayColumn(true);

    // Press '5' key
    QVERIFY(keyboardService.handleKeyPressed(Qt::Key_5, Qt::NoModifier, false));

    QVERIFY(editorService->digitSet().has_value());
    QCOMPARE(*editorService->digitSet(), quint8 { 5 });
}

void KeyboardServiceTest::test_handleKeyPressed_Delete_shouldClearDelay_whenAtDelayColumn()
{
    const auto applicationService = std::make_shared<MockApplicationService>();
    const auto editorService = std::make_shared<MockEditorService>();
    const auto playerService = std::make_shared<MockPlayerService>();
    const auto selectionService = std::make_shared<MockSelectionService>();
    const auto settingsService = std::make_shared<MockSettingsService>();

    KeyboardService keyboardService { applicationService, editorService, playerService, selectionService, settingsService };

    applicationService->setEditMode(true);
    editorService->setMockIsAtNoteColumn(false);
    editorService->setMockIsAtDelayColumn(true);

    QVERIFY(keyboardService.handleKeyPressed(Qt::Key_Delete, Qt::NoModifier, false));

    QVERIFY(editorService->digitSet().has_value());
    QCOMPARE(*editorService->digitSet(), quint8 { 0 });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::KeyboardServiceTest)