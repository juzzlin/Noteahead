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

#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

//! Mirrors the legibility floor the generator enforces, so the palettes stay readable as track
//! names and automation traces against the near-black editor background.
constexpr auto minContrastOnBlack = 4.5;

double contrastOnBlack(const QColor & color)
{
    const auto linearize = [](double channel) {
        return channel <= 0.03928 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
    };
    const auto luminance = 0.2126 * linearize(color.redF()) + 0.7152 * linearize(color.greenF()) + 0.0722 * linearize(color.blueF());
    return (luminance + 0.05) / 0.05;
}

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

void ThemeServiceTest::test_paletteAccentBlend_setter_shouldUpdateGetterAndEmitSignals()
{
    ThemeService themeService;
    themeService.setPaletteAccentBlend(20);
    QSignalSpy blendSpy { &themeService, &ThemeService::paletteAccentBlendChanged };
    QSignalSpy trackSpy { &themeService, &ThemeService::trackHeaderTextColorsChanged };
    QSignalSpy automationSpy { &themeService, &ThemeService::automationCurveColorsChanged };

    themeService.setPaletteAccentBlend(80);

    QCOMPARE(themeService.paletteAccentBlend(), 80);
    QCOMPARE(blendSpy.count(), 1);
    // Both palettes are derived from the blend, so both must be re-read by the views
    QCOMPARE(trackSpy.count(), 1);
    QCOMPARE(automationSpy.count(), 1);
}

void ThemeServiceTest::test_paletteAccentBlend_outOfRange_shouldBeClamped()
{
    ThemeService themeService;

    themeService.setPaletteAccentBlend(-10);
    QCOMPARE(themeService.paletteAccentBlend(), 0);

    themeService.setPaletteAccentBlend(250);
    QCOMPARE(themeService.paletteAccentBlend(), 100);
}

void ThemeServiceTest::test_paletteAccentBlend_setter_shouldPersistAcrossInstances()
{
    {
        ThemeService themeService;
        themeService.setPaletteAccentBlend(35);
    }

    ThemeService reloaded;

    QCOMPARE(reloaded.paletteAccentBlend(), 35);
}

void ThemeServiceTest::test_trackHeaderTextColors_zeroBlend_shouldReturnTheOriginalPalette()
{
    ThemeService themeService;
    const QColor accentColor { "#3366cc" };
    themeService.setAccentColor(accentColor);
    themeService.setPaletteAccentBlend(0);

    const auto colors = themeService.trackHeaderTextColors();

    // The palette the editor shipped with, to the byte: the accent, then the fixed sixteen
    QCOMPARE(colors.size(), 16);
    QCOMPARE(qvariant_cast<QColor>(colors.at(0)), accentColor);
    QCOMPARE(qvariant_cast<QColor>(colors.at(1)), QColor { "white" });
    QCOMPARE(qvariant_cast<QColor>(colors.at(2)), QColor { "#ff5555" });
    QCOMPARE(qvariant_cast<QColor>(colors.at(15)), QColor { "#888888" });
}

void ThemeServiceTest::test_trackHeaderTextColors_fullBlend_shouldShareTheAccentHue()
{
    ThemeService themeService;
    const QColor accentColor { "#cc4400" };
    themeService.setAccentColor(accentColor);
    themeService.setPaletteAccentBlend(100);

    for (auto && color : themeService.trackHeaderTextColors()) {
        QCOMPARE(qvariant_cast<QColor>(color).hue(), accentColor.hue());
    }
}

void ThemeServiceTest::test_trackHeaderTextColors_blended_shouldBeUnique()
{
    ThemeService themeService;

    // A blend of 0 is excluded on purpose: it hands back the original palette verbatim, and that
    // one has always put the accent at index 0 next to a fixed white at index 1, so a white accent
    // collides there. Blending separates them, because their accent counterparts differ in lightness.
    for (auto && accentColor : { QColor { "orange" }, QColor { "#2244ff" }, QColor { "#ff00aa" }, QColor { "white" } }) {
        themeService.setAccentColor(accentColor);
        for (auto && blend : { 25, 70, 100 }) {
            themeService.setPaletteAccentBlend(blend);
            const auto colors = themeService.trackHeaderTextColors();
            QSet<QRgb> unique;
            for (auto && color : colors) {
                unique.insert(qvariant_cast<QColor>(color).rgb());
            }
            QCOMPARE(unique.size(), colors.size());
        }
    }
}

void ThemeServiceTest::test_trackHeaderTextColors_fullBlend_shouldBeLegibleOnBlack()
{
    ThemeService themeService;
    themeService.setPaletteAccentBlend(100);

    // Only the accent end is guaranteed legible. At a blend of 0 the original palette is handed
    // back as-is, and two of its entries have always been dimmer than this floor.
    for (auto && accentColor : { QColor { "orange" }, QColor { "#2244ff" }, QColor { "#ff00aa" }, QColor { "black" } }) {
        themeService.setAccentColor(accentColor);
        for (auto && color : themeService.trackHeaderTextColors()) {
            QVERIFY(contrastOnBlack(qvariant_cast<QColor>(color)) >= minContrastOnBlack);
        }
    }
}

void ThemeServiceTest::test_automationCurveColors_zeroBlend_shouldReturnTheOriginalPalette()
{
    ThemeService themeService;
    themeService.setPaletteAccentBlend(0);

    const auto colors = themeService.automationCurveColors();

    QCOMPARE(colors.size(), 6);
    QCOMPARE(qvariant_cast<QColor>(colors.at(0)), QColor { "#ffffff" });
    QCOMPARE(qvariant_cast<QColor>(colors.at(5)), QColor { "#b78cff" });
}

void ThemeServiceTest::test_automationCurveColors_fullBlend_shouldBeUniqueAndLegible()
{
    ThemeService themeService;
    themeService.setAccentColor(QColor { "#22aa88" });
    themeService.setPaletteAccentBlend(100);

    const auto colors = themeService.automationCurveColors();

    QCOMPARE(colors.size(), 6);
    QSet<QRgb> unique;
    for (auto && color : colors) {
        const auto curveColor = qvariant_cast<QColor>(color);
        unique.insert(curveColor.rgb());
        QVERIFY(contrastOnBlack(curveColor) >= minContrastOnBlack);
    }
    QCOMPARE(unique.size(), colors.size());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ThemeServiceTest)
