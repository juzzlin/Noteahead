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
#include "../infra/midi/midi_backend_rt_midi.hpp"
#include "../infra/midi/midi_cc_mapping.hpp"
#include "instrument_request.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace noteahead {

static const auto TAG = "MidiWorker";

MidiWorker::MidiWorker(QObject * parent)
  : QObject { parent }
  , m_midiBackend { std::make_unique<MidiBackendRtMidi>() }
{
    juzzlin::L(TAG).info() << "Midi API name: " << m_midiBackend->midiApiName();

    initializeScanTimer();
}

void MidiWorker::initializeScanTimer()
{
    if (!m_midiScanTimer) {
        m_midiScanTimer = std::make_unique<QTimer>();
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
                                m_midiBackend->closeDevice(*device);
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
}

void portError(const std::string_view function, const std::string_view message)
{
    juzzlin::L(TAG).error() << function << ": No device found for portName '" << message << "'";
}

void MidiWorker::initializeStopTimer()
{
    if (!m_midiStopTimer) {
        m_midiStopTimer = std::make_unique<QTimer>();
        m_midiStopTimer->setInterval(500ms);
        m_midiStopTimer->setSingleShot(true);
        connect(m_midiStopTimer.get(), &QTimer::timeout, this, [this]() {
            for (auto && stopTask : m_stopTasks) {
                try {
                    if (const auto device = m_midiBackend->deviceByPortName(stopTask.portName.toStdString()); device) {
                        m_midiBackend->openDevice(*device);
                        m_midiBackend->sendNoteOff(*device, stopTask.channel, 60);
                    } else {
                        portError(__func__, stopTask.portName.toStdString());
                    }
                } catch (const std::runtime_error & e) {
                    juzzlin::L(TAG).error() << e.what();
                }
            }
        });
    }
}

void MidiWorker::sendMidiCcSettings(const MidiDevice & midiDevice, const Instrument & instrument)
{
    const auto channel = instrument.device().channel;
    const auto predefinedMidiCcSettings = instrument.settings().predefinedMidiCcSettings;
    m_midiBackend->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::ResetAllControllers), 127);
    if (predefinedMidiCcSettings.pan.has_value()) {
        m_midiBackend->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::PanMSB), *predefinedMidiCcSettings.pan);
        m_midiBackend->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::PanLSB), 0);
    }
    if (predefinedMidiCcSettings.volume.has_value()) {
        m_midiBackend->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::ChannelVolumeMSB), *predefinedMidiCcSettings.volume);
        m_midiBackend->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::ChannelVolumeLSB), 0);
    }
    if (predefinedMidiCcSettings.cutoff.has_value()) {
        m_midiBackend->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::SoundController5), *predefinedMidiCcSettings.cutoff);
    }
    for (auto && midiCcSetting : instrument.settings().midiCcSettings) {
        if (midiCcSetting.enabled()) {
            m_midiBackend->sendCcData(midiDevice, channel, midiCcSetting.controller(), midiCcSetting.value());
        }
    }
}

void MidiWorker::handleInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    if (instrumentRequest.type() == InstrumentRequest::Type::None) {
        return;
    }

    try {
        auto && instrument = instrumentRequest.instrument();
        juzzlin::L(TAG).info() << "Applying instrument " << instrument.toString().toStdString() << " for requested port " << instrument.device().portName.toStdString();
        const auto requestedPortName = instrument.device().portName;
        if (const auto midiDevice = m_midiBackend->deviceByPortName(requestedPortName.toStdString()); midiDevice) {
            m_midiBackend->openDevice(*midiDevice);
            if (instrumentRequest.type() == InstrumentRequest::Type::ApplyAll) {
                if (instrument.settings().bank.has_value()) {
                    m_midiBackend->sendBankChange(*midiDevice, instrument.device().channel,
                                                  instrument.settings().bank->byteOrderSwapped ? instrument.settings().bank->lsb : instrument.settings().bank->msb,
                                                  instrument.settings().bank->byteOrderSwapped ? instrument.settings().bank->msb : instrument.settings().bank->lsb);
                }
                if (instrument.settings().patch.has_value()) {
                    m_midiBackend->sendPatchChange(*midiDevice, instrument.device().channel, *instrument.settings().patch);
                }
                sendMidiCcSettings(*midiDevice, instrument);
            } else if (instrumentRequest.type() == InstrumentRequest::Type::ApplyPatch) {
                if (instrument.settings().patch.has_value()) {
                    m_midiBackend->sendPatchChange(*midiDevice, instrument.device().channel, *instrument.settings().patch);
                }
            } else if (instrumentRequest.type() == InstrumentRequest::Type::ApplyMidiCc) {
                sendMidiCcSettings(*midiDevice, instrument);
            }
        } else {
            portError(__func__, requestedPortName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(*device);
            m_midiBackend->sendNoteOn(*device, channel, 60, velocity);
            m_stopTasks.push_back({ portName, channel, 60 });
            initializeStopTimer(); // Initialize here to end up in the correct thread
            m_midiStopTimer->start();
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::playNote(QString portName, quint8 channel, quint8 midiNote, quint8 velocity)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(*device);
            m_midiBackend->sendNoteOn(*device, channel, midiNote, velocity);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::stopNote(QString portName, quint8 channel, quint8 midiNote)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(*device);
            m_midiBackend->sendNoteOff(*device, channel, midiNote);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::stopAllNotes(QString portName, quint8 channel)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(*device);
            m_midiBackend->stopAllNotes(*device, channel);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::sendClock(QString portName)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(*device);
            m_midiBackend->sendClockPulse(*device);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::sendCcData(QString portName, quint8 channel, quint8 controller, quint8 value)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(*device);
            m_midiBackend->sendCcData(*device, channel, controller, value);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::sendPitchBendData(QString portName, quint8 channel, quint8 msb, quint8 lsb)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(*device);
            m_midiBackend->sendPitchBendData(*device, channel, msb, lsb);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorker::requestPatchChange(QString portName, quint8 channel, quint8 patch)
{
    try {
        if (const auto device = m_midiBackend->deviceByPortName(portName.toStdString()); device) {
            m_midiBackend->openDevice(*device);
            m_midiBackend->sendPatchChange(*device, channel, patch);
        } else {
            portError(__func__, portName.toStdString());
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
