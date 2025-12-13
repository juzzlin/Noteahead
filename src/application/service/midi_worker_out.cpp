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

#include "midi_worker_out.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/instrument.hpp"
#ifdef __linux__
#include "../../infra/midi/implementation/alsa/midi_out_alsa.hpp"
#else
#include "../../infra/midi/implementation/librtmidi/midi_out_rt_midi.hpp"
#endif
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../instrument_request.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace noteahead {

static const auto TAG = "MidiOutWorker";

MidiWorkerOut::MidiWorkerOut(QObject * parent)
#ifdef __linux__
  : MidiWorker { std::make_unique<MidiOutAlsa>(), "OUT", parent }
#else
  : MidiWorker { std::make_unique<MidiOutRtMidi>(), "OUT", parent }
#endif
  , m_midiBackendOut { std::dynamic_pointer_cast<MidiBackendOut>(midiBackend()) }
{
    juzzlin::L(TAG).info() << "Midi API name: " << m_midiBackendOut->midiApiName();
}

void portError(const std::string_view function, const std::string_view message)
{
    juzzlin::L(TAG).error() << function << ": No port found for portName '" << message << "'";
}

void MidiWorkerOut::initializeStopTimer()
{
    if (!m_midiStopTimer) {
        m_midiStopTimer = std::make_unique<QTimer>();
        m_midiStopTimer->setInterval(500ms);
        m_midiStopTimer->setSingleShot(true);
        connect(m_midiStopTimer.get(), &QTimer::timeout, this, [this]() {
            for (auto && stopTask : m_stopTasks) {
                try {
                    if (const auto port = m_midiBackendOut->portByName(stopTask.portName.toStdString()); port) {
                        m_midiBackendOut->openPort(*port);
                        m_midiBackendOut->sendNoteOff(*port, stopTask.channel, 60);
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

void MidiWorkerOut::sendMidiCcSettings(const MidiPort & port, const Instrument & instrument)
{
    const auto channel = instrument.midiAddress().channel();
    const auto predefinedMidiCcSettings = instrument.settings().standardMidiCcSettings;
    m_midiBackendOut->sendCcData(port, channel, static_cast<quint8>(MidiCcMapping::Controller::ResetAllControllers), 127);
    if (predefinedMidiCcSettings.pan.has_value()) {
        m_midiBackendOut->sendCcData(port, channel, static_cast<quint8>(MidiCcMapping::Controller::PanMSB), *predefinedMidiCcSettings.pan);
        m_midiBackendOut->sendCcData(port, channel, static_cast<quint8>(MidiCcMapping::Controller::PanLSB), 0);
    }
    if (predefinedMidiCcSettings.volume.has_value()) {
        m_midiBackendOut->sendCcData(port, channel, static_cast<quint8>(MidiCcMapping::Controller::ChannelVolumeMSB), *predefinedMidiCcSettings.volume);
        m_midiBackendOut->sendCcData(port, channel, static_cast<quint8>(MidiCcMapping::Controller::ChannelVolumeLSB), 0);
    }
    if (predefinedMidiCcSettings.cutoff.has_value()) {
        m_midiBackendOut->sendCcData(port, channel, static_cast<quint8>(MidiCcMapping::Controller::SoundController5), *predefinedMidiCcSettings.cutoff);
    }
    for (auto && midiCcSetting : instrument.settings().midiCcSettings) {
        if (midiCcSetting.enabled()) {
            m_midiBackendOut->sendCcData(port, channel, midiCcSetting.controller(), midiCcSetting.value());
        }
    }
}

void MidiWorkerOut::applyBank(const Instrument & instrument, MidiPortS port)
{
    if (instrument.settings().bank.has_value()) {
        juzzlin::L(TAG).info() << "Setting bank to " << static_cast<int>(instrument.settings().bank->msb) << ":" << static_cast<int>(instrument.settings().bank->lsb);
        m_midiBackendOut->sendBankChange(*port, instrument.midiAddress().channel(),
                                  instrument.settings().bank->byteOrderSwapped ? instrument.settings().bank->lsb : instrument.settings().bank->msb,
                                  instrument.settings().bank->byteOrderSwapped ? instrument.settings().bank->msb : instrument.settings().bank->lsb);
    }
}

void MidiWorkerOut::applyPatch(const Instrument & instrument, MidiPortS port)
{
    if (instrument.settings().patch.has_value()) {
        juzzlin::L(TAG).info() << "Setting patch to " << static_cast<int>(*instrument.settings().patch);
        m_midiBackendOut->sendPatchChange(*port, instrument.midiAddress().channel(), *instrument.settings().patch);
    }
}

void MidiWorkerOut::handleInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    if (instrumentRequest.type() == InstrumentRequest::Type::None) {
        return;
    }

    try {
        auto && instrument = instrumentRequest.instrument();
        juzzlin::L(TAG).info() << "Applying instrument " << instrument.toString().toStdString() << " for requested port " << instrument.midiAddress().portName().toStdString();
        const auto requestedPortName = instrument.midiAddress().portName();
        if (const auto port = m_midiBackendOut->portByName(requestedPortName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            if (instrumentRequest.type() == InstrumentRequest::Type::ApplyAll) {
                applyBank(instrument, port);
                applyPatch(instrument, port);
                sendMidiCcSettings(*port, instrument);
            } else if (instrumentRequest.type() == InstrumentRequest::Type::ApplyPatch) {
                applyPatch(instrument, port);
            } else if (instrumentRequest.type() == InstrumentRequest::Type::ApplyMidiCc) {
                sendMidiCcSettings(*port, instrument);
            }
        } else {
            portError(__func__, requestedPortName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorkerOut::playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->sendNoteOn(*port, channel, 60, velocity);
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

void MidiWorkerOut::playNote(QString portName, quint8 channel, quint8 midiNote, quint8 velocity)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->sendNoteOn(*port, channel, midiNote, velocity);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorkerOut::stopNote(QString portName, quint8 channel, quint8 midiNote)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->sendNoteOff(*port, channel, midiNote);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorkerOut::stopAllNotes(QString portName, quint8 channel)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->stopAllNotes(*port, channel);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorkerOut::sendClock(QString portName)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->sendClockPulse(*port);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorkerOut::sendStart(QString portName)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->sendStart(*port);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorkerOut::sendStop(QString portName)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->sendStop(*port);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorkerOut::sendCcData(QString portName, quint8 channel, quint8 controller, quint8 value)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->sendCcData(*port, channel, controller, value);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorkerOut::sendPitchBendData(QString portName, quint8 channel, quint8 msb, quint8 lsb)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->sendPitchBendData(*port, channel, msb, lsb);
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

void MidiWorkerOut::requestPatchChange(QString portName, quint8 channel, quint8 patch)
{
    try {
        if (const auto port = m_midiBackendOut->portByName(portName.toStdString()); port) {
            m_midiBackendOut->openPort(*port);
            m_midiBackendOut->sendPatchChange(*port, channel, patch);
        } else {
            portError(__func__, portName.toStdString());
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
    }
}

} // namespace noteahead
