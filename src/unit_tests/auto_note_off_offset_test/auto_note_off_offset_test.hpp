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

#ifndef AUTO_NOTE_OFF_OFFSET_TEST_HPP
#define AUTO_NOTE_OFF_OFFSET_TEST_HPP

#include <QObject>

namespace noteahead {

class AutoNoteOffOffsetTest : public QObject
{
    Q_OBJECT

private slots:
    void test_defaults_shouldBeMilliseconds();
    void test_ticks_milliseconds_shouldScaleWithTempo();
    void test_ticks_milliseconds_zero_shouldBeZeroTicks();
    void test_ticks_sync_shouldBeANoteLength();
    void test_ticks_sync_shouldNotScaleWithTempo();
    void test_ticks_sync_everyDenominator_shouldBeShorterThanThePrevious();
    void test_ticks_sync_invalidDenominator_shouldBeZeroTicks();
    void test_serialization_shouldRoundTrip();
    void test_serialization_sync_shouldKeepMillisecondsValue();
    void test_serialization_commaDecimalLocale_shouldRoundTrip();
    void test_deserialization_legacyAttributesOnly_shouldBeMilliseconds();
    void test_deserialization_noAttributes_shouldKeepDefaults();
};

} // namespace noteahead

#endif // AUTO_NOTE_OFF_OFFSET_TEST_HPP
