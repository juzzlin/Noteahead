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

#include "midi_in_worker.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/midi/implementation/librtmidi/midi_in_rt_midi.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace noteahead {

static const auto TAG = "MidiInWorker";

MidiInWorker::MidiInWorker(QObject * parent)
  : MidiWorker { parent }
  , m_midiIn { std::make_unique<MidiInRtMidi>() }
{
    juzzlin::L(TAG).info() << "Midi API name: " << m_midiIn->midiApiName();

    initializeScanTimer();
}

void MidiInWorker::setControllerPort(QString portName)
{
    if (!portName.isEmpty()) {
        m_controllerPort = portName;
        try {
            if (const auto device = m_midiIn->deviceByPortName(portName.toStdString()); device) {
                juzzlin::L(TAG).info() << "Opening controller input port: " << portName.toStdString();
                m_midiIn->openDevice(*device);
                m_midiIn->setCallbackForPort(*device,
                                             [this](double deltaTime, MessageCR message) {
                                                 handleIncomingMessage(deltaTime, message);
                                             });
            } else {
                juzzlin::L(TAG).warning() << "No device found for port name: " << portName.toStdString();
            }
        } catch (std::runtime_error & e) {
            juzzlin::L(TAG).error() << e.what();
        }
    }
}

void MidiInWorker::handleIncomingMessage(double deltaTime, MessageCR message)
{
    if (!message.empty()) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (auto byte : message) {
            oss << "0x" << std::setw(2) << static_cast<int>(byte) << " ";
        }
        juzzlin::L(TAG).debug() << "Received MIDI message (" << deltaTime << "s): " << oss.str();
        // emit midiMessageReceived(QByteArray(reinterpret_cast<const char *>(message.data()), int(message.size())));
    }
}

void MidiInWorker::initializeScanTimer()
{
    if (!m_midiScanTimer) {
        m_midiScanTimer = std::make_unique<QTimer>();
        m_midiScanTimer->setInterval(2500ms);
        connect(m_midiScanTimer.get(), &QTimer::timeout, this, [this] {
            if (!isPlaying()) {
                m_midiIn->updateAvailableDevices();
                QStringList updatedDeviceList;
                std::ranges::transform(m_midiIn->listDevices(), std::back_inserter(updatedDeviceList),
                                       [](const auto & device) { return QString::fromStdString(device->portName()); });
                if (m_availablePorts != updatedDeviceList) {
                    QStringList newDevices;
                    for (auto && port : updatedDeviceList) {
                        if (!m_availablePorts.contains(port)) {
                            newDevices << port;
                        }
                    }
                    QStringList offDevices;
                    for (auto && port : m_availablePorts) {
                        if (!updatedDeviceList.contains(port)) {
                            offDevices << port;
                        }
                    }
                    if (!newDevices.isEmpty()) {
                        for (auto && portName : newDevices) {
                            if (const auto device = m_midiIn->deviceByPortName(portName.toStdString()); device) {
                                juzzlin::L(TAG).info() << "Detected MIDI IN device " << portName.toStdString();
                            }
                        }
                        if (newDevices.size() <= 3) {
                            emit statusTextRequested(tr("New MIDI IN devices found: ") + newDevices.join(","));
                        } else {
                            emit statusTextRequested(tr("New MIDI IN device(s) found"));
                        }
                    }
                    if (!offDevices.isEmpty()) {
                        for (auto && portName : offDevices) {
                            if (const auto device = m_midiIn->deviceByPortName(portName.toStdString()); device) {
                                juzzlin::L(TAG).info() << "Closing MIDI IN device " << portName.toStdString();
                                m_midiIn->closeDevice(*device);
                            }
                        }
                        if (newDevices.size() <= 3) {
                            emit statusTextRequested(tr("MIDI IN devices went offline: ") + offDevices.join(","));
                        } else {
                            emit statusTextRequested(tr("MIDI IN device(s) went offline "));
                        }
                    }
                    m_availablePorts = updatedDeviceList;
                    emit availablePortsChanged(m_availablePorts);
                    emit portsAppeared(newDevices);
                    emit portsDisappeared(offDevices);
                }

                setControllerPort(m_controllerPort);
            }
        });
        m_midiScanTimer->start();
    }
}

} // namespace noteahead
