// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef MIDI_CC_AUTOMATION_HPP
#define MIDI_CC_AUTOMATION_HPP

#include <QString>

#include <cstdint>

class QXmlStreamWriter;

namespace noteahead {

class MidiCcAutomation
{
    MidiCcAutomation(uint8_t controller, size_t startLine, size_t endLine, double startValue, double endValue);
    MidiCcAutomation();

    uint8_t controller() const;
    void setController(uint8_t controller);

    void serializeToXml(QXmlStreamWriter & writer) const;

    QString toString() const;

private:
    uint8_t m_controller = 0;

    size_t m_startLine = 0;
    size_t m_endLine = 0;

    double m_startValue = 0;
    double m_endValue = 0;
};

} // namespace noteahead

#endif // MIDI_CC_AUTOMATION_HPP
