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

#include "midi_settings_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../service/settings_service.hpp"

#include <iomanip>

namespace noteahead {

static const auto TAG = "MidiSettingsModel";

MidiSettingsModel::MidiSettingsModel(SettingsServiceS settingsService, QObject * parent)
  : QObject { parent }
  , m_settingsService { settingsService }
  , m_controllerPort { m_settingsService->controllerPort() }
{
}

QStringList MidiSettingsModel::midiInPorts() const
{
    return m_midiInPorts;
}

void MidiSettingsModel::setMidiInPorts(QStringList portNames)
{
    m_settingPorts = true;

    const auto oldMidiPorts = m_midiInPorts;
    m_midiInPorts = portNames;
    if (!m_controllerPort.isEmpty() && !m_midiInPorts.contains(m_controllerPort)) {
        m_midiInPorts.append(m_controllerPort);
    }

    if (m_midiInPorts != oldMidiPorts) {
        juzzlin::L(TAG).info() << "Setting available MIDI IN ports to '" << m_midiInPorts.join(", ").toStdString() << "'";
        emit midiInPortsChanged();
    }

    m_settingPorts = false;
}

QString MidiSettingsModel::controllerPort() const
{
    return m_controllerPort;
}

void MidiSettingsModel::setControllerPort(const QString & name)
{
    if (m_settingPorts) {
        return;
    }

    juzzlin::L(TAG).debug() << "Setting port name to " << std::quoted(name.toStdString());
    if (m_controllerPort != name) {
        m_controllerPort = name;
        m_settingsService->setControllerPort(name);
        emit controllerPortChanged(name);
    }
}

QString MidiSettingsModel::debugData() const
{
    return m_debugData;
}

void MidiSettingsModel::setDebugData(const QString & data)
{
    if (m_debugData != data) {
        m_debugData = data;
        emit debugDataChanged();
    }
}

MidiSettingsModel::~MidiSettingsModel() = default;

} // namespace noteahead
