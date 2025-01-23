// This file is part of Cacophony.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "midi_service.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/instrument.hpp"
#include "../infra/midi_backend_rt_midi.hpp"
#include "instrument_request.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace cacophony {

static const auto TAG = "MidiService";

MidiService::MidiService(QObject * parent)
  : QObject { parent }
  , m_midiBackend { std::make_unique<MidiBackendRtMidi>() }
  , m_midiScanTimer { std::make_unique<QTimer>() }
{
    m_midiScanTimer->setInterval(2500ms);
    connect(m_midiScanTimer.get(), &QTimer::timeout, this, [this] {
        if (!m_isPlaying) {
            m_midiBackend->updateAvailableDevices();
            QStringList updatedDeviceList;
            std::ranges::transform(m_midiBackend->listDevices(), std::back_inserter(updatedDeviceList),
                                   [](const auto & device) { return QString::fromStdString(device->portName()); });
            if (m_availableMidiPorts != updatedDeviceList) {
                QStringList newDevices;
                for (auto && port : updatedDeviceList) {
                    if (!m_availableMidiPorts.contains(port)) {
                        newDevices << port;
                    }
                }
                QStringList offDevices;
                for (auto && port : m_availableMidiPorts) {
                    if (!updatedDeviceList.contains(port)) {
                        offDevices << port;
                    }
                }
                if (!newDevices.isEmpty()) {
                    if (newDevices.size() <= 3) {
                        emit statusTextRequested(tr("New MIDI devices found: ") + newDevices.join(","));
                    } else {
                        emit statusTextRequested(tr("New MIDI device(s) found"));
                    }
                }
                if (!offDevices.isEmpty()) {
                    if (newDevices.size() <= 3) {
                        emit statusTextRequested(tr("MIDI devices went offline: ") + offDevices.join(","));
                    } else {
                        emit statusTextRequested(tr("MIDI device(s) went offline "));
                    }
                }
                m_availableMidiPorts = updatedDeviceList;
                emit availableMidiPortsChanged();

                processFailedInstrumentRequests();
            }
        }
    });
    m_midiScanTimer->start();
}

QStringList MidiService::availableMidiPorts() const
{
    return m_availableMidiPorts;
}

void MidiService::processFailedInstrumentRequests()
{
    juzzlin::L(TAG).info() << "Processing previously failed instrument requests..";
    auto failedInstrumentRequests = m_failedInstrumentRequests;

    for (auto && [portName, instrumentRequest] : failedInstrumentRequests) {
        if (m_availableMidiPorts.contains(portName)) {
            m_failedInstrumentRequests.erase(portName);
            handleInstrumentRequest(instrumentRequest);
        }
    }
}

void MidiService::handleInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    if (instrumentRequest.type() == InstrumentRequest::Type::Apply) {
        try {
            if (const auto instrument = instrumentRequest.instrument(); instrument) {
                juzzlin::L(TAG).info() << "Applying instrument " << instrument->toString().toStdString();
                const auto portName = instrument->portName;
                if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
                    m_midiBackend->openDevice(device);
                    if (instrument->bank.has_value()) {
                        m_midiBackend->sendBankChange(device, instrument->channel,
                                                      instrument->bank->byteOrderSwapped ? instrument->bank->lsb : instrument->bank->msb,
                                                      instrument->bank->byteOrderSwapped ? instrument->bank->msb : instrument->bank->lsb);
                    }
                    if (instrument->patch.has_value()) {
                        m_midiBackend->sendPatchChange(device, instrument->channel, *instrument->patch);
                    }
                } else {
                    juzzlin::L(TAG).error() << "No device found for portName '" << portName.toStdString() << "'";
                    m_failedInstrumentRequests[portName] = instrumentRequest;
                }
            }
        } catch (const std::runtime_error & e) {
            juzzlin::L(TAG).error() << e.what();
        }
    }
}

void MidiService::requestPatchChange(QString portName, uint8_t channel, uint8_t patch)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(device);
            m_midiBackend->sendPatchChange(device, channel, patch);
        } else {
            juzzlin::L(TAG).error() << "No device found for portName '" << portName.toStdString() << "'";
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiService::setIsPlaying(bool isPlaying)
{
    m_isPlaying = isPlaying;
}

} // namespace cacophony
