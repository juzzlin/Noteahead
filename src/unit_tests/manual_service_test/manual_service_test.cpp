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

#include "manual_service_test.hpp"

#include "../../application/service/manual_service.hpp"
#include "../../application/service/theme_service.hpp"

#include <QFile>
#include <QSet>
#include <QSignalSpy>
#include <QTest>

namespace noteahead {

namespace {

QString bundledManual()
{
    QFile file { MANUAL_HTML_PATH };
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

} // namespace

void ManualServiceTest::test_anchorFor_title_shouldBeSlugified()
{
    QCOMPARE(ManualService::anchorFor("The Toolbar"), QString { "the-toolbar" });
    QCOMPARE(ManualService::anchorFor("Navigation & Editing"), QString { "navigation-editing" });
    QCOMPARE(ManualService::anchorFor("Tracks and Note Columns"), QString { "tracks-and-note-columns" });
    // Leading and trailing separators would otherwise leave dangling dashes
    QCOMPARE(ManualService::anchorFor("  Spaced  "), QString { "spaced" });
}

void ManualServiceTest::test_parseHeadings_markup_shouldReturnLevelsInDocumentOrder()
{
    const QString manual = "<h1>Title</h1><p>Intro</p><h2>First</h2><p>Body</p><h3>Nested</h3><h2>Second</h2>";

    const auto headings = ManualService::parseHeadings(manual);

    QCOMPARE(headings.size(), static_cast<size_t>(4));
    QCOMPARE(headings.at(0).title, QString { "Title" });
    QCOMPARE(headings.at(0).level, 1);
    QCOMPARE(headings.at(1).title, QString { "First" });
    QCOMPARE(headings.at(1).level, 2);
    QCOMPARE(headings.at(2).title, QString { "Nested" });
    QCOMPARE(headings.at(2).level, 3);
    QCOMPARE(headings.at(3).title, QString { "Second" });
    QCOMPARE(headings.at(3).anchor, QString { "second" });
}

void ManualServiceTest::test_parseHeadings_entities_shouldBeDecoded()
{
    // The dialog matches these titles against the rendered document, where "&amp;" reads as "&"
    const auto headings = ManualService::parseHeadings("<h3>String &amp; Voice</h3>");

    QCOMPARE(headings.size(), static_cast<size_t>(1));
    QCOMPARE(headings.at(0).title, QString { "String & Voice" });
    QCOMPARE(headings.at(0).anchor, QString { "string-voice" });
}

void ManualServiceTest::test_parseHeadings_noHeadings_shouldReturnEmpty()
{
    QVERIFY(ManualService::parseHeadings("<p>Just prose</p>").empty());
    QVERIFY(ManualService::parseHeadings({}).empty());
}

void ManualServiceTest::test_injectAnchors_markup_shouldAddNamedAnchorsAndKeepTitles()
{
    const QString manual = "<h1>Title</h1><h2>The Toolbar</h2>";

    const auto anchored = ManualService::injectAnchors(manual);

    QCOMPARE(anchored, QString { "<h1><a name=\"title\"></a>Title</h1><h2><a name=\"the-toolbar\"></a>The Toolbar</h2>" });
}

void ManualServiceTest::test_injectAnchors_markup_shouldPreserveEverythingElse()
{
    const QString manual = "<p>Before</p><h2>Section</h2><ul><li>Item</li></ul>";

    const auto anchored = ManualService::injectAnchors(manual);

    QVERIFY(anchored.contains("<p>Before</p>"));
    QVERIFY(anchored.contains("<ul><li>Item</li></ul>"));
    QVERIFY(anchored.contains("<a name=\"section\"></a>"));
}

void ManualServiceTest::test_load_missingFile_shouldEmitLoadFailed()
{
    ManualService manualService { nullptr };
    QSignalSpy spy { &manualService, &ManualService::loadFailed };

    manualService.load("/nonexistent/manual.html");

    QCOMPARE(spy.count(), 1);
    QVERIFY(manualService.html().isEmpty());
}

void ManualServiceTest::test_bundledManual_headings_shouldHaveUniqueTitles()
{
    const auto manual = bundledManual();
    QVERIFY(!manual.isEmpty());

    const auto headings = ManualService::parseHeadings(manual);
    QVERIFY(headings.size() > 1);

    // The dialog locates a section by searching the rendered text for its title, so two sections
    // sharing a title would make one of them unreachable
    QSet<QString> titles;
    for (auto && heading : headings) {
        QVERIFY2(!titles.contains(heading.title), qPrintable("Duplicate heading: " + heading.title));
        titles.insert(heading.title);
    }
}

void ManualServiceTest::test_bundledManual_headings_shouldAllBeReachableByAnchor()
{
    const auto manual = bundledManual();
    QVERIFY(!manual.isEmpty());

    const auto anchored = ManualService::injectAnchors(manual);
    const auto headings = ManualService::parseHeadings(manual);

    QSet<QString> anchors;
    for (auto && heading : headings) {
        QVERIFY2(!heading.anchor.isEmpty(), qPrintable("Empty anchor for: " + heading.title));
        QVERIFY2(!anchors.contains(heading.anchor), qPrintable("Duplicate anchor: " + heading.anchor));
        anchors.insert(heading.anchor);
        QVERIFY2(anchored.contains("<a name=\"" + heading.anchor + "\"></a>"), qPrintable("Missing anchor: " + heading.anchor));
    }
}

void ManualServiceTest::test_bundledManual_html_shouldCarryNoHardCodedColors()
{
    const auto manual = bundledManual();
    QVERIFY(!manual.isEmpty());

    // The colors come from the theme now. An inline style in the markup would quietly opt that
    // section out of following the accent.
    QVERIFY2(!manual.contains("style=\""), "Manual.html must not carry inline styles");
    QVERIFY2(!manual.contains("<style>"), "Manual.html must not carry its own stylesheet");
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ManualServiceTest)
