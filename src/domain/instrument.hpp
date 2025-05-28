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

#ifndef INSTRUMENT_HPP
#define INSTRUMENT_HPP

#include <cstdint>
#include <memory>

#include <QString>

#include "instrument_settings.hpp"
#include "midi_address.hpp"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class Instrument
{
public:
    explicit Instrument(const QString & portName);
    explicit Instrument(const MidiAddress & midiAddress);

    const MidiAddress & midiAddress() const;
    void setMidiAddress(const MidiAddress & midiAddress);

    const InstrumentSettings & settings() const;
    void setSettings(const InstrumentSettings & settings);

    void serializeToXml(QXmlStreamWriter & writer) const;
    using InstrumentU = std::unique_ptr<Instrument>;
    static InstrumentU deserializeFromXml(QXmlStreamReader & reader);

    QString toString() const;

private:
    MidiAddress m_midiAddress;

    InstrumentSettings m_settings;
};

} // namespace noteahead

#endif // INSTRUMENT_HPP
