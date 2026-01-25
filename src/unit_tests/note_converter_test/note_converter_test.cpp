// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#include "note_converter_test.hpp"

#include "../../application/note_converter.hpp"

#include <QTest>

namespace noteahead {

void NoteConverterTest::test_midiToString_shouldReturnCorrectString_data()
{
    QTest::addColumn<uint8_t>("midiNote");
    QTest::addColumn<QString>("expectedString");

    QTest::newRow("Middle C") << static_cast<uint8_t>(60) << "C-5";
    QTest::newRow("A5") << static_cast<uint8_t>(69) << "A-5";
    QTest::newRow("Lowest note") << static_cast<uint8_t>(0) << "C-0";
    QTest::newRow("Highest note") << static_cast<uint8_t>(127) << "G-A";
    QTest::newRow("C#5") << static_cast<uint8_t>(61) << "C#5";
}

void NoteConverterTest::test_midiToString_shouldReturnCorrectString()
{
    QFETCH(uint8_t, midiNote);
    QFETCH(QString, expectedString);

    QCOMPARE(QString::fromStdString(NoteConverter::midiToString(midiNote)), expectedString);
}

void NoteConverterTest::test_stringToMidi_shouldReturnCorrectMidiNote_data()
{
    QTest::addColumn<QString>("noteString");
    QTest::addColumn<uint8_t>("expectedMidiNote");

    QTest::newRow("Middle C") << "C-5" << static_cast<uint8_t>(60);
    QTest::newRow("A5") << "A-5" << static_cast<uint8_t>(69);
    QTest::newRow("Lowest note") << "C-0" << static_cast<uint8_t>(0);
    QTest::newRow("Highest note") << "G-A" << static_cast<uint8_t>(127);
    QTest::newRow("C#5") << "C#5" << static_cast<uint8_t>(61);
}

void NoteConverterTest::test_stringToMidi_shouldReturnCorrectMidiNote()
{
    QFETCH(QString, noteString);
    QFETCH(uint8_t, expectedMidiNote);

    QCOMPARE(NoteConverter::stringToMidi(noteString.toStdString()), expectedMidiNote);
}

void NoteConverterTest::test_midiToString_shouldThrowOnInvalidInput()
{
    // Check for out_of_range exception when MIDI note is out of valid range
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, NoteConverter::midiToString(128));
}

void NoteConverterTest::test_stringToMidi_shouldThrowOnInvalidInput()
{
    // Check for invalid_argument exception for malformed strings
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, NoteConverter::stringToMidi("H-3"));
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, NoteConverter::stringToMidi("C"));
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, NoteConverter::stringToMidi(""));
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, NoteConverter::stringToMidi("C-3-4"));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::NoteConverterTest)
