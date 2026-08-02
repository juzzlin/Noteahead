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

#include "theme_service_test.hpp"

#include "../../application/service/theme_service.hpp"

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

void ThemeServiceTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("ThemeServiceTest");

    QVERIFY(settingsDirectory().isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory().path());
}

void ThemeServiceTest::cleanupTestCase()
{
    QSettings settings {};
    settings.clear();
}

void ThemeServiceTest::test_cursorColor_unset_shouldReturnDefault()
{
    ThemeService themeService;

    QCOMPARE(themeService.cursorColor(), ThemeService::defaultCursorColor());
}

void ThemeServiceTest::test_cursorColor_setter_shouldUpdateGetterAndEmitSignal()
{
    ThemeService themeService;
    QSignalSpy spy { &themeService, &ThemeService::cursorColorChanged };

    const QColor cursorColor { "#00ff88" };
    themeService.setCursorColor(cursorColor);

    QCOMPARE(themeService.cursorColor(), cursorColor);
    QCOMPARE(spy.count(), 1);
}

void ThemeServiceTest::test_cursorColor_sameValue_shouldNotEmitSignal()
{
    ThemeService themeService;
    const QColor cursorColor { "#5555ff" };
    themeService.setCursorColor(cursorColor);

    QSignalSpy spy { &themeService, &ThemeService::cursorColorChanged };
    themeService.setCursorColor(cursorColor);

    QCOMPARE(spy.count(), 0);
}

void ThemeServiceTest::test_cursorColor_setter_shouldPersistAcrossInstances()
{
    const QColor cursorColor { "#ffb000" };
    {
        ThemeService themeService;
        themeService.setCursorColor(cursorColor);
    }

    ThemeService reloaded;

    QCOMPARE(reloaded.cursorColor(), cursorColor);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ThemeServiceTest)
