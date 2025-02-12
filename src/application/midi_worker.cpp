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

#include "midi_worker.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/instrument.hpp"
#include "../infra/midi_backend_rt_midi.hpp"
#include "instrument_request.hpp"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace noteahead {

static const auto TAG = "MidiWorker";

MidiWorker::MidiWorker(QObject * parent)
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
                    for (auto && portName : newDevices) {
                        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
                            juzzlin::L(TAG).info() << "Detected MIDI device " << portName.toStdString();
                        }
                    }
                    if (newDevices.size() <= 3) {
                        emit statusTextRequested(tr("New MIDI devices found: ") + newDevices.join(","));
                    } else {
                        emit statusTextRequested(tr("New MIDI device(s) found"));
                    }
                }
                if (!offDevices.isEmpty()) {
                    for (auto && portName : offDevices) {
                        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
                            juzzlin::L(TAG).info() << "Closing MIDI device " << portName.toStdString();
                            m_midiBackend->closeDevice(device);
                        }
                    }
                    if (newDevices.size() <= 3) {
                        emit statusTextRequested(tr("MIDI devices went offline: ") + offDevices.join(","));
                    } else {
                        emit statusTextRequested(tr("MIDI device(s) went offline "));
                    }
                }
                m_availableMidiPorts = updatedDeviceList;
                emit availableMidiPortsChanged(m_availableMidiPorts);
                emit midiPortsAppeared(newDevices);
                emit midiPortsDisappeared(offDevices);
            }
        }
    });
    m_midiScanTimer->start();
}

void MidiWorker::handleInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    if (instrumentRequest.type() == InstrumentRequest::Type::None) {
        return;
    }

    try {
        auto && instrument = instrumentRequest.instrument();
        juzzlin::L(TAG).info() << "Applying instrument " << instrument.toString().toStdString() << " for requested port " << instrument.device.portName.toStdString();
        const auto requestedPortName = instrument.device.portName;
        if (const auto device = m_midiBackend->deviceByPortName(requestedPortName.toStdString()); device) {
            m_midiBackend->openDevice(device);
            if (instrumentRequest.type() == InstrumentRequest::Type::ApplyAll) {
                if (instrument.settings.bank.has_value()) {
                    m_midiBackend->sendBankChange(device, instrument.device.channel,
                                                  instrument.settings.bank->byteOrderSwapped ? instrument.settings.bank->lsb : instrument.settings.bank->msb,
                                                  instrument.settings.bank->byteOrderSwapped ? instrument.settings.bank->msb : instrument.settings.bank->lsb);
                }
                if (instrument.settings.patch.has_value()) {
                    m_midiBackend->sendPatchChange(device, instrument.device.channel, *instrument.settings.patch);
                }
                if (instrument.settings.pan.has_value()) {
                    m_midiBackend->sendCC(device, instrument.device.channel, MidiCc::PanMSB, *instrument.settings.pan);
                    m_midiBackend->sendCC(device, instrument.device.channel, MidiCc::PanLSB, 0);
                }
                if (instrument.settings.volume.has_value()) {
                    m_midiBackend->sendCC(device, instrument.device.channel, MidiCc::ChannelVolumeMSB, *instrument.settings.volume);
                    m_midiBackend->sendCC(device, instrument.device.channel, MidiCc::ChannelVolumeLSB, 0);
                }
                if (instrument.settings.cutoff.has_value()) {
                    m_midiBackend->sendCC(device, instrument.device.channel, MidiCc::SoundController5, *instrument.settings.cutoff);
                }
            } else if (instrumentRequest.type() == InstrumentRequest::Type::ApplyPatch) {
                if (instrument.settings.patch.has_value()) {
                    m_midiBackend->sendPatchChange(device, instrument.device.channel, *instrument.settings.patch);
                }
            }
        } else {
            juzzlin::L(TAG).error() << "No device found for portName '" << requestedPortName.toStdString() << "'";
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::playAndStopMiddleC(QString portName, uint8_t channel, uint8_t velocity)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(device);
            m_midiBackend->sendNoteOn(device, channel, 60, velocity);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            m_midiBackend->sendNoteOff(device, channel, 60);
        } else {
            juzzlin::L(TAG).error() << "No device found for portName '" << portName.toStdString() << "'";
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::playNote(QString portName, uint8_t channel, uint8_t midiNote, uint8_t velocity)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(device);
            m_midiBackend->sendNoteOn(device, channel, midiNote, velocity);
        } else {
            juzzlin::L(TAG).error() << "No device found for portName '" << portName.toStdString() << "'";
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::stopNote(QString portName, uint8_t channel, uint8_t midiNote)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(device);
            m_midiBackend->sendNoteOff(device, channel, midiNote);
        } else {
            juzzlin::L(TAG).error() << "No device found for portName '" << portName.toStdString() << "'";
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::stopAllNotes(QString portName, uint8_t channel)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(device);
            m_midiBackend->stopAllNotes(device, channel);
        } else {
            juzzlin::L(TAG).error() << "No device found for portName '" << portName.toStdString() << "'";
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::requestPatchChange(QString portName, uint8_t channel, uint8_t patch)
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

void MidiWorker::setIsPlaying(bool isPlaying)
{
    m_isPlaying = isPlaying;
}

} // namespace noteahead
