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

#include "midi_cc_selection_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

namespace noteahead {

static const auto TAG = "MidiCcSelectionModel";

MidiCcSelectionModel::MidiCcSelectionModel(QObject * parent)
  : QObject { parent }
{
}

quint8 MidiCcSelectionModel::midiCcController(quint8 index) const
{
    return m_indexToSetting.contains(index) ? m_indexToSetting.at(index).controller() : 0;
}

void MidiCcSelectionModel::setMidiCcController(quint8 index, quint8 controller)
{
    juzzlin::L(TAG).info() << "Setting controller number " << static_cast<int>(controller) << " for slot " << static_cast<int>(index);

    m_indexToSetting[index].setController(controller);
}

quint8 MidiCcSelectionModel::midiCcValue(quint8 index) const
{
    return m_indexToSetting.contains(index) ? m_indexToSetting.at(index).value() : 0;
}

void MidiCcSelectionModel::setMidiCcValue(quint8 index, quint8 value)
{
    juzzlin::L(TAG).info() << "Setting value " << static_cast<int>(value) << " for slot " << static_cast<int>(index);

    m_indexToSetting[index].setValue(value);
}

bool MidiCcSelectionModel::midiCcEnabled(quint8 index) const
{
    return m_indexToSetting.contains(index) && m_indexToSetting.at(index).enabled();
}

void MidiCcSelectionModel::setMidiCcEnabled(quint8 index, bool enabled)
{
    juzzlin::L(TAG).info() << "Setting slot " << static_cast<int>(index) << " enabled: " << static_cast<int>(enabled);

    if (!m_indexToSetting.contains(index)) {
        m_indexToSetting[index] = { enabled, 0, 0 };
    } else {
        m_indexToSetting.at(index).setEnabled(enabled);
    }
}

QString MidiCcSelectionModel::midiCcToString(quint8 controller) const
{
    return MidiCcMapping::controllerToString(static_cast<MidiCcMapping::Controller>(controller));
}

quint8 MidiCcSelectionModel::midiCcSlots() const
{
    return 8;
}

MidiCcSelectionModel::MidiCcSettingList MidiCcSelectionModel::midiCcSettings() const
{
    MidiCcSettingList settings;
    std::ranges::transform(m_indexToSetting, std::back_inserter(settings),
                           [](const auto & pair) { return pair.second; });
    return settings;
}

void MidiCcSelectionModel::setMidiCcSettings(const MidiCcSettingList & midiCcSettings)
{
    m_indexToSetting.clear();
    for (auto && setting : midiCcSettings) {
        m_indexToSetting[static_cast<quint8>(m_indexToSetting.size())] = setting;
    }
}

} // namespace noteahead
