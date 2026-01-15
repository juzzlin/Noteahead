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

#include "automation.hpp"

#include <cstdint>
#include <memory>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class MidiCcAutomation : public Automation
{
public:
    struct InterpolationParameters
    {
        size_t line0 = 0;
        size_t line1 = 0;
        uint8_t value0 = 0;
        uint8_t value1 = 0;

        bool operator==(const InterpolationParameters & other) const
        {
            return line0 == other.line0 && line1 == other.line1 && value0 == other.value0 && value1 == other.value1;
        }

        bool operator!=(const InterpolationParameters & other) const
        {
            return !(*this == other);
        }
    };

    struct ModulationParameters
    {
        float cycles = 0.f;
        float amplitude = 0.f;
        float offset = 0.f;
        bool inverted = false;

        bool operator==(const ModulationParameters & other) const
        {
            return cycles == other.cycles && amplitude == other.amplitude && offset == other.offset && inverted == other.inverted;
        }

        bool operator!=(const ModulationParameters & other) const
        {
            return !(*this == other);
        }
    };

    MidiCcAutomation(size_t id, AutomationLocation location, uint8_t controller, InterpolationParameters interpolation, ModulationParameters modulation, QString comment, bool enabled);
    MidiCcAutomation(size_t id, AutomationLocation location, uint8_t controller, InterpolationParameters interpolation, QString comment, bool enabled);
    MidiCcAutomation(size_t id, AutomationLocation location, uint8_t controller, InterpolationParameters interpolation, QString comment);
    MidiCcAutomation();

    bool operator==(const MidiCcAutomation & other) const;
    bool operator!=(const MidiCcAutomation & other) const;
    bool operator<(const MidiCcAutomation & other) const;

    const InterpolationParameters & interpolation() const;
    void setInterpolation(const InterpolationParameters & parameters);

    const ModulationParameters & modulation() const;
    void setModulation(const ModulationParameters & modulation);

    uint8_t controller() const;
    void setController(uint8_t controller);

    uint8_t eventsPerBeat() const;
    void setEventsPerBeat(uint8_t eventsPerBeat);

    uint8_t lineOffset() const;
    void setLineOffset(uint8_t lineOffset);

    void serializeToXml(QXmlStreamWriter & writer) const;
    using MidiCcAutomationU = std::unique_ptr<MidiCcAutomation>;
    static MidiCcAutomationU deserializeFromXml(QXmlStreamReader & reader);

    QString toString() const;

private:
    uint8_t m_controller = 0;
    uint8_t m_eventsPerBeat = 0;
    uint8_t m_lineOffset = 0;

    InterpolationParameters m_interpolation;
    ModulationParameters m_modulation;
};

} // namespace noteahead

#endif // MIDI_CC_AUTOMATION_HPP
