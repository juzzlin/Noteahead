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

#include "midi_out_worker.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/instrument.hpp"
#include "../../infra/midi/implementation/librtmidi/midi_out_rt_midi.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../instrument_request.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace noteahead {

static const auto TAG = "MidiOutWorker";

MidiOutWorker::MidiOutWorker(QObject * parent)
  : MidiWorker { std::make_unique<MidiOutRtMidi>(), "OUT", parent }
  , m_midiOut { std::dynamic_pointer_cast<MidiOut>(midi()) }
{
    juzzlin::L(TAG).info() << "Midi API name: " << m_midiOut->midiApiName();
}

void portError(const std::string_view function, const std::string_view message)
{
    juzzlin::L(TAG).error() << function << ": No device found for portName '" << message << "'";
}

void MidiOutWorker::initializeStopTimer()
{
    if (!m_midiStopTimer) {
        m_midiStopTimer = std::make_unique<QTimer>();
        m_midiStopTimer->setInterval(500ms);
        m_midiStopTimer->setSingleShot(true);
        connect(m_midiStopTimer.get(), &QTimer::timeout, this, [this]() {
            for (auto && stopTask : m_stopTasks) {
                try {
                    if (const auto device = m_midiOut->deviceByPortName(stopTask.portName.toStdString()); device) {
                        m_midiOut->openDevice(*device);
                        m_midiOut->sendNoteOff(*device, stopTask.channel, 60);
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

void MidiOutWorker::sendMidiCcSettings(const MidiDevice & midiDevice, const Instrument & instrument)
{
    const auto channel = instrument.midiAddress().channel();
    const auto predefinedMidiCcSettings = instrument.settings().standardMidiCcSettings;
    m_midiOut->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::ResetAllControllers), 127);
    if (predefinedMidiCcSettings.pan.has_value()) {
        m_midiOut->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::PanMSB), *predefinedMidiCcSettings.pan);
        m_midiOut->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::PanLSB), 0);
    }
    if (predefinedMidiCcSettings.volume.has_value()) {
        m_midiOut->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::ChannelVolumeMSB), *predefinedMidiCcSettings.volume);
        m_midiOut->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::ChannelVolumeLSB), 0);
    }
    if (predefinedMidiCcSettings.cutoff.has_value()) {
        m_midiOut->sendCcData(midiDevice, channel, static_cast<quint8>(MidiCcMapping::Controller::SoundController5), *predefinedMidiCcSettings.cutoff);
    }
    for (auto && midiCcSetting : instrument.settings().midiCcSettings) {
        if (midiCcSetting.enabled()) {
            m_midiOut->sendCcData(midiDevice, channel, midiCcSetting.controller(), midiCcSetting.value());
        }
    }
}

void MidiOutWorker::applyBank(const Instrument & instrument, MidiDeviceS midiDevice)
{
    if (instrument.settings().bank.has_value()) {
        juzzlin::L(TAG).info() << "Setting bank to " << static_cast<int>(instrument.settings().bank->msb) << ":" << static_cast<int>(instrument.settings().bank->lsb);
        m_midiOut->sendBankChange(*midiDevice, instrument.midiAddress().channel(),
                                  instrument.settings().bank->byteOrderSwapped ? instrument.settings().bank->lsb : instrument.settings().bank->msb,
                                  instrument.settings().bank->byteOrderSwapped ? instrument.settings().bank->msb : instrument.settings().bank->lsb);
    }
}

void MidiOutWorker::applyPatch(const Instrument & instrument, MidiDeviceS midiDevice)
{
    if (instrument.settings().patch.has_value()) {
        juzzlin::L(TAG).info() << "Setting patch to " << static_cast<int>(*instrument.settings().patch);
        m_midiOut->sendPatchChange(*midiDevice, instrument.midiAddress().channel(), *instrument.settings().patch);
    }
}

void MidiOutWorker::handleInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    if (instrumentRequest.type() == InstrumentRequest::Type::None) {
        return;
    }

    try {
        auto && instrument = instrumentRequest.instrument();
        juzzlin::L(TAG).info() << "Applying instrument " << instrument.toString().toStdString() << " for requested port " << instrument.midiAddress().portName().toStdString();
        const auto requestedPortName = instrument.midiAddress().portName();
        if (const auto midiDevice = m_midiOut->deviceByPortName(requestedPortName.toStdString()); midiDevice) {
            m_midiOut->openDevice(*midiDevice);
            if (instrumentRequest.type() == InstrumentRequest::Type::ApplyAll) {
                applyBank(instrument, midiDevice);
                applyPatch(instrument, midiDevice);
                sendMidiCcSettings(*midiDevice, instrument);
            } else if (instrumentRequest.type() == InstrumentRequest::Type::ApplyPatch) {
                applyPatch(instrument, midiDevice);
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

void MidiOutWorker::playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->sendNoteOn(*device, channel, 60, velocity);
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

void MidiOutWorker::playNote(QString portName, quint8 channel, quint8 midiNote, quint8 velocity)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->sendNoteOn(*device, channel, midiNote, velocity);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiOutWorker::stopNote(QString portName, quint8 channel, quint8 midiNote)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->sendNoteOff(*device, channel, midiNote);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiOutWorker::stopAllNotes(QString portName, quint8 channel)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->stopAllNotes(*device, channel);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiOutWorker::sendClock(QString portName)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->sendClockPulse(*device);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiOutWorker::sendStart(QString portName)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->sendStart(*device);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiOutWorker::sendStop(QString portName)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->sendStop(*device);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiOutWorker::sendCcData(QString portName, quint8 channel, quint8 controller, quint8 value)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->sendCcData(*device, channel, controller, value);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiOutWorker::sendPitchBendData(QString portName, quint8 channel, quint8 msb, quint8 lsb)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->sendPitchBendData(*device, channel, msb, lsb);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiOutWorker::requestPatchChange(QString portName, quint8 channel, quint8 patch)
{
    try {
        if (const auto device = m_midiOut->deviceByPortName(portName.toStdString()); device) {
            m_midiOut->openDevice(*device);
            m_midiOut->sendPatchChange(*device, channel, patch);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

} // namespace noteahead
