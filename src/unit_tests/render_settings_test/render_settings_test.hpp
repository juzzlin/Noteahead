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

#ifndef RENDER_SETTINGS_TEST_HPP
#define RENDER_SETTINGS_TEST_HPP

#include <QObject>

namespace noteahead {

class RenderSettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_defaults_shouldBeTheOnesANewSongStartsFrom();
    void test_serialization_shouldRoundTripThroughMetadata();
    void test_serialization_commaDecimalLocale_shouldRoundTrip();
    void test_metadataWithoutRenderSettings_shouldKeepDefaults();
    void test_renderSettingsWithoutFastRender_shouldStayReproducible();
    void test_exportTags_unset_shouldFallBackOnSongTags();
    void test_exportTags_set_shouldOverrideSongTags();
    void test_exportTags_empty_shouldNotBeSerialized();
    void test_exportTags_serialization_shouldRoundTrip();
    void test_exportTags_cleared_shouldLeaveNoTrace();
    void test_notes_empty_shouldNotBeSerialized();
    void test_notes_multiLine_shouldRoundTrip();
    void test_notes_xmlHostileCharacters_shouldRoundTrip();
    void test_clear_shouldResetEverything();
};

} // namespace noteahead

#endif // RENDER_SETTINGS_TEST_HPP
