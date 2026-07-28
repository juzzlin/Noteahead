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

#include "settings_service_test.hpp"

#include "../../application/service/settings_service.hpp"
#include "../../infra/settings.hpp"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace noteahead {

namespace {

// Keeps the settings written by this test process out of the user scope shared by all
// processes. Without it, concurrent runs of this binary (parallel CI workspaces) race on the
// same settings file.
QTemporaryDir & settingsDirectory()
{
    static QTemporaryDir directory;
    return directory;
}

} // namespace

void SettingsServiceTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("SettingsServiceTest");

    QVERIFY(settingsDirectory().isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory().path());
}

void SettingsServiceTest::cleanupTestCase()
{
    QSettings settings {};
    settings.clear();
}

void SettingsServiceTest::test_patternPeekEnabled_setter_shouldUpdateGetterAndEmitSignal()
{
    SettingsService settingsService;
    settingsService.setPatternPeekEnabled(true);

    QSignalSpy spy { &settingsService, &SettingsService::patternPeekEnabledChanged };
    settingsService.setPatternPeekEnabled(false);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(settingsService.patternPeekEnabled(), false);
}

void SettingsServiceTest::test_patternPeekEnabled_sameValue_shouldNotEmitSignal()
{
    SettingsService settingsService;
    settingsService.setPatternPeekEnabled(true);

    QSignalSpy spy { &settingsService, &SettingsService::patternPeekEnabledChanged };
    settingsService.setPatternPeekEnabled(true);

    QCOMPARE(spy.count(), 0);
    QCOMPARE(settingsService.patternPeekEnabled(), true);
}

void SettingsServiceTest::test_patternPeekEnabled_externalChange_shouldNotAffectCachedValue()
{
    SettingsService settingsService;
    settingsService.setPatternPeekEnabled(true);

    // Bypass the service: a proxying service must not re-read the backing store on every get
    Settings::setPatternPeekEnabled(false);

    QCOMPARE(settingsService.patternPeekEnabled(), true);
}

void SettingsServiceTest::test_getters_shouldBeServedFromMembers()
{
    SettingsService settingsService;
    settingsService.setRecordingEnabled(true);
    settingsService.setMidiSyncEnabled(true);
    settingsService.setRenderSampleRate(44100);
    settingsService.setVisibleLines(24);

    Settings::setRecordingEnabled(false);
    Settings::setMidiSyncEnabled(false);
    Settings::setRenderSampleRate(96000);
    Settings::setVisibleLines(64);

    QCOMPARE(settingsService.recordingEnabled(), true);
    QCOMPARE(settingsService.midiSyncEnabled(), true);
    QCOMPARE(settingsService.renderSampleRate(), 44100);
    QCOMPARE(settingsService.visibleLines(), 24);
}

void SettingsServiceTest::test_step_unset_shouldReturnDefault()
{
    SettingsService settingsService;

    QCOMPARE(settingsService.step(), 1);
}

void SettingsServiceTest::test_velocity_unset_shouldReturnDefault()
{
    SettingsService settingsService;

    QCOMPARE(settingsService.velocity(), 100);
}

void SettingsServiceTest::test_windowSize_unset_shouldReturnGivenDefault()
{
    SettingsService settingsService;

    const QSize defaultSize { 640, 480 };
    QCOMPARE(settingsService.windowSize(defaultSize), defaultSize);
}

void SettingsServiceTest::test_windowSize_stored_shouldOverrideGivenDefault()
{
    const QSize storedSize { 1280, 720 };
    SettingsService settingsService;
    settingsService.setWindowSize(storedSize);

    QCOMPARE(settingsService.windowSize(QSize { 640, 480 }), storedSize);

    SettingsService reloadedSettingsService;
    QCOMPARE(reloadedSettingsService.windowSize(QSize { 640, 480 }), storedSize);
}

void SettingsServiceTest::test_setters_shouldPersistAcrossInstances()
{
    {
        SettingsService settingsService;
        settingsService.setStep(4);
        settingsService.setVelocity(77);
        settingsService.setTrackHeaderFontSize(18);
        settingsService.setRenderNormalizeLevel(-1.5);
        settingsService.setAutoNoteOffOffset(125);
    }

    SettingsService reloadedSettingsService;

    QCOMPARE(reloadedSettingsService.step(), 4);
    QCOMPARE(reloadedSettingsService.velocity(), 77);
    QCOMPARE(reloadedSettingsService.trackHeaderFontSize(), 18);
    QCOMPARE(reloadedSettingsService.renderNormalizeLevel(), -1.5);
    QCOMPARE(reloadedSettingsService.autoNoteOffOffset(), 125);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SettingsServiceTest)
