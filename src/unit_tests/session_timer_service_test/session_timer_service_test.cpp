// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#include "session_timer_service_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/session_timer_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../infra/data_service.hpp"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include <memory>

namespace noteahead {

namespace {

//! The idle timeout without touching the user's settings
class StubSettingsService : public SettingsService
{
public:
    int idleTimeoutMinutes() const override
    {
        return minutes;
    }

    int minutes { 1 };
};

struct Fixture
{
    Fixture()
      : settings { std::make_shared<StubSettingsService>() }
      , editor { std::make_shared<EditorService>(std::make_shared<SelectionService>(), settings, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>()) }
    {
        editor->initialize();
        timer = std::make_unique<SessionTimerService>(editor, settings, [this] { return now; });
    }

    //! Ticks once a second for @p seconds, as the service's own timer does
    void run(int seconds)
    {
        for (int i = 0; i < seconds; i++) {
            now += 1000;
            timer->tick();
        }
    }

    qint64 now { 0 };
    std::shared_ptr<StubSettingsService> settings;
    std::shared_ptr<EditorService> editor;
    std::unique_ptr<SessionTimerService> timer;
};

} // namespace

void SessionTimerServiceTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("SessionTimerServiceTest");
}

void SessionTimerServiceTest::test_tick_active_shouldCountSessionAndTotal()
{
    Fixture fixture;
    fixture.run(30);
    QCOMPARE(fixture.timer->sessionSeconds(), 30);
    QCOMPARE(fixture.timer->totalSeconds(), 30);
    QCOMPARE(fixture.editor->song()->timeSpentSeconds(), size_t { 30 });
    QVERIFY(!fixture.timer->idle());
}

void SessionTimerServiceTest::test_tick_shortTicks_shouldCarryPartSeconds()
{
    Fixture fixture;
    for (int i = 0; i < 5; i++) {
        fixture.now += 400;
        fixture.timer->tick();
    }
    QCOMPARE(fixture.timer->sessionSeconds(), 2);
}

void SessionTimerServiceTest::test_tick_afterTimeout_shouldGoIdleAndStop()
{
    Fixture fixture;
    QSignalSpy idleSpy { fixture.timer.get(), &SessionTimerService::idleChanged };
    fixture.run(60);
    QVERIFY(fixture.timer->idle());
    QCOMPARE(idleSpy.count(), 1);
    QCOMPARE(fixture.timer->sessionSeconds(), 60);
    // Idle: nothing more is counted however long it waits
    fixture.run(600);
    QCOMPARE(fixture.timer->sessionSeconds(), 60);
}

void SessionTimerServiceTest::test_tick_timeoutSetting_shouldBeRespected()
{
    Fixture fixture;
    fixture.settings->minutes = 5;
    fixture.run(299);
    QVERIFY(!fixture.timer->idle());
    fixture.run(1);
    QVERIFY(fixture.timer->idle());
}

void SessionTimerServiceTest::test_tick_longGap_shouldNotCount()
{
    // A machine that slept between two ticks did no work in between
    Fixture fixture;
    fixture.run(10);
    fixture.now += 2 * 60 * 60 * 1000;
    fixture.timer->tick();
    QCOMPARE(fixture.timer->sessionSeconds(), 10);
    QVERIFY(fixture.timer->idle());
}

void SessionTimerServiceTest::test_tick_shouldNotMarkModified()
{
    Fixture fixture;
    fixture.editor->setIsModified(false);
    QSignalSpy modifiedSpy { fixture.editor.get(), &EditorService::isModifiedChanged };
    fixture.run(30);
    QVERIFY(!fixture.editor->isModified());
    QCOMPARE(modifiedSpy.count(), 0);
}

void SessionTimerServiceTest::test_notifyActivity_afterIdle_shouldResumeWithoutTheGap()
{
    Fixture fixture;
    fixture.run(60);
    QVERIFY(fixture.timer->idle());
    fixture.now += 10 * 60 * 1000;
    fixture.timer->notifyActivity();
    QVERIFY(!fixture.timer->idle());
    fixture.run(5);
    QCOMPARE(fixture.timer->sessionSeconds(), 65);
}

void SessionTimerServiceTest::test_setPlaying_withoutInput_shouldKeepCounting()
{
    // Listening back is work
    Fixture fixture;
    fixture.timer->setPlaying(true);
    fixture.run(180);
    QVERIFY(!fixture.timer->idle());
    QCOMPARE(fixture.timer->sessionSeconds(), 180);
    // Stopped, it waits out the timeout from there
    fixture.timer->setPlaying(false);
    fixture.run(60);
    QVERIFY(fixture.timer->idle());
}

void SessionTimerServiceTest::test_songChanged_shouldStartANewSessionWithTheSongsTotal()
{
    Fixture fixture;
    fixture.run(30);
    auto song = std::make_shared<Song>();
    song->setTimeSpentSeconds(500);
    fixture.editor->setSong(song);
    QCOMPARE(fixture.timer->sessionSeconds(), 0);
    QCOMPARE(fixture.timer->totalSeconds(), 500);
    fixture.run(10);
    QCOMPARE(fixture.timer->totalSeconds(), 510);
}

void SessionTimerServiceTest::test_resetTotal_shouldZeroAndMarkModified()
{
    Fixture fixture;
    fixture.run(30);
    fixture.editor->setIsModified(false);
    fixture.timer->resetTotal();
    QCOMPARE(fixture.timer->totalSeconds(), 0);
    QVERIFY(fixture.editor->isModified());
    // The session goes on
    QCOMPARE(fixture.timer->sessionSeconds(), 30);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SessionTimerServiceTest)
