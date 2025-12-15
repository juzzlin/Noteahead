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

#ifndef MIDI_ADDRESS_HPP
#define MIDI_ADDRESS_HPP

#include <QString>

#include <cstdint>
#include <memory>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class MidiAddress
{
public:
    explicit MidiAddress(QString portName);
    MidiAddress(QString portName, uint8_t channel);
    MidiAddress(QString portName, uint8_t channel, uint8_t group);

    void serializeToXml(QXmlStreamWriter & writer) const;
    using MidiAddressU = std::unique_ptr<MidiAddress>;
    static MidiAddressU deserializeFromXml(QXmlStreamReader & reader);

    const QString & portName() const;
    void setPort(QString portName);

    uint8_t channel() const;
    void setChannel(uint8_t channel);

    uint8_t group() const;
    void setGroup(uint8_t group);

private:
    QString m_portName;
    uint8_t m_channel = 0;
    uint8_t m_group = 0; // MIDI 2.0 addition
};

} // namespace noteahead

#endif // MIDI_ADDRESS_HPP
