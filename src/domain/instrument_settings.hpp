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

#ifndef INSTRUMENT_SETTINGS_HPP
#define INSTRUMENT_SETTINGS_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include <QString>

#include "event_data.hpp"
#include "instrument_layer.hpp"
#include "midi_cc_setting.hpp"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class InstrumentSettings : public EventData
{
public:
    std::optional<uint8_t> patch;

    struct Bank
    {
        uint8_t lsb = 0;
        uint8_t msb = 0;

        bool byteOrderSwapped = false;
    };

    std::optional<Bank> bank;

    int transpose { 0 };

    struct TimingSettings
    {
        std::optional<bool> sendMidiClock;
        std::optional<bool> sendTransport;

        std::chrono::milliseconds delay { 0 };

        //! Overrides the global setting
        std::optional<std::chrono::milliseconds> autoNoteOffOffset;
    };

    TimingSettings timing;

    struct MidiEffects
    {
        int velocityJitter { 0 };
    };

    MidiEffects midiEffects;

    struct StandardMidiCcSettings
    {
        std::optional<uint8_t> cutoff;
        std::optional<uint8_t> pan;
        std::optional<uint8_t> volume;
    };

    StandardMidiCcSettings standardMidiCcSettings;

    std::vector<MidiCcSetting> midiCcSettings;

    std::vector<InstrumentLayer> layers;

    void serializeToXml(QXmlStreamWriter & writer) const;
    using InstrumentSettingsU = std::unique_ptr<InstrumentSettings>;
    static InstrumentSettingsU deserializeFromXml(QXmlStreamReader & reader);

    QString toString() const;
};

} // namespace noteahead

#endif // INSTRUMENT_SETTINGS_HPP
