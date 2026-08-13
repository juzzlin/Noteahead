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

#ifndef MANUAL_SERVICE_TEST_HPP
#define MANUAL_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class ManualServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_anchorFor_title_shouldBeSlugified();

    void test_parseHeadings_markup_shouldReturnLevelsInDocumentOrder();
    void test_parseHeadings_entities_shouldBeDecoded();
    void test_parseHeadings_noHeadings_shouldReturnEmpty();

    void test_parseSections_markup_shouldSplitAtEveryHeading();
    void test_parseSections_preamble_shouldGoToTheFirstSection();
    void test_parseSections_noHeadings_shouldReturnEmpty();
    void test_bundledManual_sections_shouldCoverTheWholeManual();


    void test_load_missingFile_shouldEmitLoadFailed();

    void test_bundledManual_headings_shouldHaveUniqueTitles();
    void test_bundledManual_sections_shouldAllBeReachableByAnchor();
    void test_bundledManual_html_shouldCarryNoHardCodedColors();
};

} // namespace noteahead

#endif // MANUAL_SERVICE_TEST_HPP
