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

#ifndef MIDI_CC_SETTING_HPP
#define MIDI_CC_SETTING_HPP

#include <QString>
#include <cstdint>

class QXmlStreamWriter;
class QXmlStreamReader;

namespace noteahead {

class MidiCcSetting
{
public:
    MidiCcSetting(bool enabled, uint8_t controller, uint8_t value);
    MidiCcSetting();

    bool enabled() const;
    void setEnabled(bool enabled);

    uint8_t controller() const;
    void setController(uint8_t controller);

    uint8_t value() const;
    void setValue(uint8_t value);

    void serializeToXml(QXmlStreamWriter & writer) const;
    static std::unique_ptr<MidiCcSetting> deserializeFromXml(QXmlStreamReader & reader);

    QString toString() const;

private:
    bool m_enabled = false;
    uint8_t m_controller = 0;
    uint8_t m_value = 0;
};

} // namespace noteahead

#endif // MIDI_CC_SETTING_HPP
