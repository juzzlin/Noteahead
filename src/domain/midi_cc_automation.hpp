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
#include <memory>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class MidiCcAutomation
{
public:
    struct Location
    {
        size_t pattern = 0;
        size_t track = 0;
        size_t column = 0;

        bool operator==(const Location & other) const
        {
            return pattern == other.pattern && track == other.track && column == other.column;
        }

        bool operator!=(const Location & other) const
        {
            return !(*this == other);
        }
    };

    struct Interpolation
    {
        size_t line0 = 0;
        size_t line1 = 0;
        uint8_t value0 = 0;
        uint8_t value1 = 0;

        bool operator==(const Interpolation & other) const
        {
            return line0 == other.line0 && line1 == other.line1 && value0 == other.value0 && value1 == other.value1;
        }

        bool operator!=(const Interpolation & other) const
        {
            return !(*this == other);
        }
    };

    MidiCcAutomation(size_t id, Location location, uint8_t controller, Interpolation interpolation, QString comment, bool enabled);
    MidiCcAutomation(size_t id, Location location, uint8_t controller, Interpolation interpolation, QString comment);
    MidiCcAutomation();

    bool operator==(const MidiCcAutomation & other) const;
    bool operator!=(const MidiCcAutomation & other) const;
    bool operator<(const MidiCcAutomation & other) const;

    size_t id() const;
    void setId(size_t id);

    const Location & location() const;
    void setLocation(const Location & location);

    const Interpolation & interpolation() const;
    void setInterpolation(const Interpolation & interpolation);

    uint8_t controller() const;
    void setController(uint8_t controller);

    QString comment() const;
    void setComment(QString comment);

    bool enabled() const;
    void setEnabled(bool enabled);

    void serializeToXml(QXmlStreamWriter & writer) const;
    using MidiCcAutomationU = std::unique_ptr<MidiCcAutomation>;
    static MidiCcAutomationU deserializeFromXml(QXmlStreamReader & reader);

    QString toString() const;

private:
    size_t m_id = 0;

    uint8_t m_controller = 0;

    Location m_location;

    Interpolation m_interpolation;

    QString m_comment;

    bool m_enabled = true;
};

} // namespace noteahead

#endif // MIDI_CC_AUTOMATION_HPP
