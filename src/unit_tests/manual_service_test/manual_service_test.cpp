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

void ManualServiceTest::test_parseSections_markup_shouldSplitAtEveryHeading()
{
    const auto sections = ManualService::parseSections("<h1>Title</h1><p>One</p><h2>Second</h2><p>Two</p><h3>Third</h3><p>Three</p>");

    QCOMPARE(sections.size(), 3u);
    QCOMPARE(sections[0].heading.title, QString { "Title" });
    QCOMPARE(sections[0].markup, QString { "<h1>Title</h1><p>One</p>" });
    QCOMPARE(sections[1].heading.title, QString { "Second" });
    QCOMPARE(sections[1].heading.level, 2);
    QCOMPARE(sections[1].markup, QString { "<h2>Second</h2><p>Two</p>" });
    QCOMPARE(sections[2].heading.anchor, QString { "third" });
    QCOMPARE(sections[2].markup, QString { "<h3>Third</h3><p>Three</p>" });
}

void ManualServiceTest::test_parseSections_preamble_shouldGoToTheFirstSection()
{
    const auto sections = ManualService::parseSections("<p>Before</p><h1>Title</h1><p>After</p>");

    // Nothing may be dropped: the sections are the whole manual, in order.
    QCOMPARE(sections.size(), 1u);
    QCOMPARE(sections[0].markup, QString { "<p>Before</p><h1>Title</h1><p>After</p>" });
}

void ManualServiceTest::test_parseSections_noHeadings_shouldReturnEmpty()
{
    QVERIFY(ManualService::parseSections("<p>Nothing to see here</p>").empty());
}

void ManualServiceTest::test_bundledManual_sections_shouldCoverTheWholeManual()
{
    const auto manual = bundledManual();
    QVERIFY(!manual.isEmpty());

    const auto sections = ManualService::parseSections(manual);
    const auto headings = ManualService::parseHeadings(manual);
    QCOMPARE(sections.size(), headings.size());

    // The dialog scrolls to a section by the position of the item rendering it, so every table of
    // contents entry has to have a section of its own, and the sections put back together have to
    // be the manual again or something would be missing from the page.
    QString rejoined;
    for (size_t i = 0; i < sections.size(); i++) {
        QCOMPARE(sections[i].heading.anchor, headings[i].anchor);
        QVERIFY2(!sections[i].markup.isEmpty(), qPrintable("Empty section: " + sections[i].heading.title));
        rejoined += sections[i].markup;
    }
    QCOMPARE(rejoined, manual);
}

void ManualServiceTest::test_load_missingFile_shouldEmitLoadFailed()
{
    ManualService manualService { nullptr };
    QSignalSpy spy { &manualService, &ManualService::loadFailed };

    manualService.load("/nonexistent/manual.html");

    QCOMPARE(spy.count(), 1);
    QVERIFY(manualService.sections().isEmpty());
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

void ManualServiceTest::test_bundledManual_sections_shouldAllBeReachableByAnchor()
{
    const auto manual = bundledManual();
    QVERIFY(!manual.isEmpty());

    const auto sections = ManualService::parseSections(manual);
    QVERIFY(sections.size() > 1);

    // A cross reference of the form "#anchor" is resolved to the section carrying that anchor, so
    // an empty or repeated anchor would leave a section unreachable.
    QSet<QString> anchors;
    for (auto && section : sections) {
        QVERIFY2(!section.heading.anchor.isEmpty(), qPrintable("Empty anchor for: " + section.heading.title));
        QVERIFY2(!anchors.contains(section.heading.anchor), qPrintable("Duplicate anchor: " + section.heading.anchor));
        anchors.insert(section.heading.anchor);
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
