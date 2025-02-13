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

#ifndef MIDI_CC_SELECTION_MODEL_HPP
#define MIDI_CC_SELECTION_MODEL_HPP

#include <QObject>

#include "../../domain/midi_cc_setting.hpp"

namespace noteahead {

class MidiCcSelectionModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(uint8_t midiCcSlots READ midiCcSlots NOTIFY midiCcSlotsChanged)

public:
    explicit MidiCcSelectionModel(QObject * parent = nullptr);

    Q_INVOKABLE uint8_t midiCcController(uint8_t index) const;
    Q_INVOKABLE void setMidiCcController(uint8_t index, uint8_t controller);

    Q_INVOKABLE uint8_t midiCcValue(uint8_t index) const;
    Q_INVOKABLE void setMidiCcValue(uint8_t index, uint8_t value);

    Q_INVOKABLE bool midiCcEnabled(uint8_t index) const;
    Q_INVOKABLE void setMidiCcEnabled(uint8_t index, bool enabled);

    Q_INVOKABLE QString midiCcToString(uint8_t controller) const;

    uint8_t midiCcSlots() const;

    using MidiCcSettingList = std::vector<MidiCcSetting>;
    MidiCcSettingList midiCcSettings() const;
    void setMidiCcSettings(const MidiCcSettingList & midiCcSettings);

signals:
    void midiCcSlotsChanged();

private:
    std::map<uint8_t, MidiCcSetting> m_indexToSetting;
};

} // namespace noteahead

#endif // MIDI_CC_SELECTION_MODEL_HPP
