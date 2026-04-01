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

#include "arpeggiator_test.hpp"
#include "../../domain/arpeggiator.hpp"

#include <algorithm>

namespace noteahead {

void ArpeggiatorTest::test_generate_singleNote_shouldReturnInput()
{
    const Arpeggiator::NoteInfoList input = { { 60, 100 } };

    // All patterns should just return the same single note
    QCOMPARE(Arpeggiator::generate(Arpeggiator::Pattern::Up, input).size(), 1u);
    QCOMPARE(Arpeggiator::generate(Arpeggiator::Pattern::Down, input).size(), 1u);
    QCOMPARE(Arpeggiator::generate(Arpeggiator::Pattern::UpDown, input).size(), 1u);
    QCOMPARE(Arpeggiator::generate(Arpeggiator::Pattern::DownUp, input).size(), 1u);
    QCOMPARE(Arpeggiator::generate(Arpeggiator::Pattern::Random, input).size(), 1u);
    
    QCOMPARE(Arpeggiator::generate(Arpeggiator::Pattern::Up, input)[0].note, 60);
}

void ArpeggiatorTest::test_generate_up_shouldSortNotes()
{
    const Arpeggiator::NoteInfoList input = { { 64, 80 }, { 60, 100 }, { 67, 90 } };

    const auto result = Arpeggiator::generate(Arpeggiator::Pattern::Up, input);

    QCOMPARE(result.size(), 3u);
    QCOMPARE(result[0].note, 60);
    QCOMPARE(result[1].note, 64);
    QCOMPARE(result[2].note, 67);
}

void ArpeggiatorTest::test_generate_down_shouldSortNotesDescending()
{
    const Arpeggiator::NoteInfoList input = { { 64, 80 }, { 60, 100 }, { 67, 90 } };

    const auto result = Arpeggiator::generate(Arpeggiator::Pattern::Down, input);

    QCOMPARE(result.size(), 3u);
    QCOMPARE(result[0].note, 67);
    QCOMPARE(result[1].note, 64);
    QCOMPARE(result[2].note, 60);
}

void ArpeggiatorTest::test_generate_upDown_shouldFollowPattern()
{
    const Arpeggiator::NoteInfoList input = { { 60, 100 }, { 64, 80 }, { 67, 90 } };

    const auto result = Arpeggiator::generate(Arpeggiator::Pattern::UpDown, input);

    QCOMPARE(result.size(), 4u);
    QCOMPARE(result[0].note, 60);
    QCOMPARE(result[1].note, 64);
    QCOMPARE(result[2].note, 67);
    QCOMPARE(result[3].note, 64);
}

void ArpeggiatorTest::test_generate_downUp_shouldFollowPattern()
{
    const Arpeggiator::NoteInfoList input = { { 60, 100 }, { 64, 80 }, { 67, 90 } };

    const auto result = Arpeggiator::generate(Arpeggiator::Pattern::DownUp, input);

    QCOMPARE(result.size(), 4u);
    QCOMPARE(result[0].note, 67);
    QCOMPARE(result[1].note, 64);
    QCOMPARE(result[2].note, 60);
    QCOMPARE(result[3].note, 64);
}

void ArpeggiatorTest::test_generate_random_shouldReturnPermutation()
{
    const Arpeggiator::NoteInfoList input = { { 60, 100 }, { 64, 80 }, { 67, 90 } };

    const auto result = Arpeggiator::generate(Arpeggiator::Pattern::Random, input);

    QCOMPARE(result.size(), 3u);

    // Should contain all unique notes from input
    std::vector<uint8_t> resultNotes;
    for (const auto & ni : result) resultNotes.push_back(ni.note);
    std::sort(resultNotes.begin(), resultNotes.end());
    
    QCOMPARE(resultNotes[0], 60);
    QCOMPARE(resultNotes[1], 64);
    QCOMPARE(resultNotes[2], 67);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ArpeggiatorTest)
