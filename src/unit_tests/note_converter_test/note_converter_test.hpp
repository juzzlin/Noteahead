// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef NOTE_CONVERTER_TEST_HPP
#define NOTE_CONVERTER_TEST_HPP

#include <QTest>

namespace cacophony {

class NoteConverterTest : public QObject
{
    Q_OBJECT

private slots:

    void test_midiToString_shouldReturnCorrectString_data();
    void test_midiToString_shouldReturnCorrectString();

    void test_stringToMidi_shouldReturnCorrectMidiNote_data();
    void test_stringToMidi_shouldReturnCorrectMidiNote();

    void test_midiToString_shouldThrowOnInvalidInput();
    void test_stringToMidi_shouldThrowOnInvalidInput();
};

} // namespace cacophony

#endif // NOTE_CONVERTER_TEST_HPP
