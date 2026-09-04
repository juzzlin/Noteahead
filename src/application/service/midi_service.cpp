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

#include "midi_service.hpp"

#include <algorithm>

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/midi/midi_address.hpp"
#include "../../domain/midi/midi_cc_data.hpp"
#include "../../domain/midi/midi_note_data.hpp"
#include "../../domain/midi/pitch_bend_data.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../instrument_request.hpp"
#include "device_service.hpp"
#include "midi_worker_in.hpp"
#include "midi_worker_out.hpp"

namespace noteahead {

static const auto TAG = "MidiService";

MidiService::MidiService(DeviceServiceS deviceService, QObject * parent)
  : MidiService { std::move(deviceService), parent, true }
{
}

MidiService::MidiService(DeviceServiceS deviceService, QObject * parent, bool initializeRealWorkers)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
{
    if (m_deviceService) {
        connect(m_deviceService.get(), &DeviceService::dataChanged, this, [this]() {
            emit outputPortsChanged(outputPorts());
        });
    }

    if (initializeRealWorkers) {
        m_outputWorker = std::make_unique<MidiWorkerOut>();
        m_inputWorker = std::make_unique<MidiWorkerIn>();
        initializeWorkers();
    }
}

void MidiService::initializeWorkers()
{
    initializeInputWorker();
    initializeOutputWorker();
}

void MidiService::initializeInputWorker()
{
    connect(this, &MidiService::controllerPortChanged, m_inputWorker.get(), &MidiWorkerIn::setControllerPort);

    connect(m_inputWorker.get(), &MidiWorkerIn::portsChanged, this, [this](const auto & midiPorts) {
        m_inputPorts = { "" };
        m_inputPorts.append(midiPorts);
        emit inputPortsChanged(m_inputPorts);
    });

    connect(m_inputWorker.get(), &MidiWorkerIn::portsAppeared, this, &MidiService::inputPortsAppeared);
    connect(m_inputWorker.get(), &MidiWorkerIn::portsDisappeared, this, &MidiService::inputPortsDisappeared);
    connect(m_inputWorker.get(), &MidiWorkerIn::statusTextRequested, this, &MidiService::statusTextRequested);

    connect(m_inputWorker.get(), &MidiWorkerIn::startReceived, this, &MidiService::startReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::stopReceived, this, &MidiService::stopReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::continueReceived, this, &MidiService::continueReceived);

    connect(m_inputWorker.get(), &MidiWorkerIn::noteOnReceived, this, &MidiService::noteOnReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::noteOffReceived, this, &MidiService::noteOffReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::pitchBendReceived, this, &MidiService::pitchBendReceived);

    connect(m_inputWorker.get(), &MidiWorkerIn::polyAftertouchReceived, this, &MidiService::polyAftertouchReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::aftertouchReceived, this, &MidiService::aftertouchReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::controlChangeReceived, this, &MidiService::controlChangeReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::programChangeReceived, this, &MidiService::programChangeReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::rpnReceived, this, &MidiService::rpnReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::nrpnReceived, this, &MidiService::nrpnReceived);
    connect(m_inputWorker.get(), &MidiWorkerIn::sysExReceived, this, &MidiService::sysExReceived);

    connect(m_inputWorker.get(), &MidiWorkerIn::dataReceived, this, &MidiService::dataReceived);

    m_inputWorker->moveToThread(&m_inputWorkerThread);
    m_inputWorkerThread.start(QThread::NormalPriority);
}

void MidiService::initializeOutputWorker()
{
    // Direct calls now, no need for signals/slots for requests, but we keep signals for port updates
    connect(m_outputWorker.get(), &MidiWorkerOut::portsChanged, this, [this](const auto & midiPorts) {
        m_outputPorts = { "" };
        m_outputPorts.append(midiPorts);
        if (m_deviceService) {
            m_outputPorts.append(m_deviceService->internalDeviceNamesQt());
        }
        emit outputPortsChanged(m_outputPorts);
    });

    connect(m_outputWorker.get(), &MidiWorkerOut::portsAppeared, this, &MidiService::outputPortsAppeared);
    connect(m_outputWorker.get(), &MidiWorkerOut::portsDisappeared, this, &MidiService::outputPortsDisappeared);
    connect(m_outputWorker.get(), &MidiWorkerOut::statusTextRequested, this, &MidiService::statusTextRequested);

    // No thread moving for output worker
}

QStringList MidiService::outputPorts() const
{
    auto ports = m_outputPorts;
    if (m_deviceService) {
        const auto internalPorts = m_deviceService->internalDeviceNamesQt();
        for (const auto & port : internalPorts) {
            if (!ports.contains(port)) {
                ports.append(port);
            }
        }
    }
    return ports;
}

void MidiService::handleInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    if (m_deviceService && m_deviceService->isInternalDevice(instrumentRequest.instrument().midiAddress().portName())) {
        handleInternalDeviceInstrumentRequest(instrumentRequest);
        return;
    }
    m_outputWorker->handleInstrumentRequest(instrumentRequest);
}

void MidiService::handleInternalDeviceInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    juzzlin::L(TAG).info() << "Applying instrument on internal device: " << instrumentRequest.instrument().toString().toStdString();

    // What an instrument's settings mean for an internal device is DeviceService's to define, so
    // that a render applies them the same way playback does rather than growing its own version.
    switch (instrumentRequest.type()) {
    case InstrumentRequest::Type::ApplyAll:
        m_deviceService->applyInstrumentSettings(instrumentRequest.instrument());
        break;
    case InstrumentRequest::Type::ApplyPatch:
        m_deviceService->applyInstrumentPatch(instrumentRequest.instrument());
        break;
    case InstrumentRequest::Type::ApplyMidiCc:
        m_deviceService->applyInstrumentMidiCcSettings(instrumentRequest.instrument());
        break;
    case InstrumentRequest::Type::None:
        break;
    }
}

void MidiService::setControllerPort(QString portName)
{
    emit controllerPortChanged(portName);
}

void MidiService::setMidiSyncEnabled(bool enabled)
{
    m_inputWorker->setMidiSyncEnabled(enabled);
}

void MidiService::setIsPlaying(bool isPlaying)
{
    m_inputWorker->setIsPlaying(isPlaying);
    m_outputWorker->setIsPlaying(isPlaying);
}

void MidiService::playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity)
{
    if (m_deviceService && m_deviceService->isInternalDevice(portName)) {
        m_deviceService->processMidiNoteOn(portName, 60, velocity);
        // FIXME: need a way to stop it after a delay for internal devices if wanted
        return;
    }
    m_outputWorker->playAndStopMiddleC(portName, channel, velocity);
}

void MidiService::playNote(InstrumentW instrument, MidiNoteDataCR data)
{
    if (const auto instr = instrument.lock()) {
        const auto portName = instr->midiAddress().portName();
        if (m_deviceService && m_deviceService->isInternalDevice(portName)) {
            m_deviceService->processMidiNoteOn(portName, data.note(), data.velocity());
        } else {
            m_outputWorker->playNote(portName, instr->midiAddress().channel(), data.note(), data.velocity());
        }
    }
}

bool MidiService::isInternalInstrument(InstrumentW instrument) const
{
    if (const auto instr = instrument.lock(); instr && m_deviceService) {
        return m_deviceService->isInternalDevice(instr->midiAddress().portName());
    }
    return false;
}

void MidiService::clearScheduledEvents()
{
    if (m_deviceService) {
        m_deviceService->clearScheduledEvents();
    }
}

void MidiService::playNoteAt(InstrumentW instrument, MidiNoteDataCR data, std::chrono::steady_clock::time_point when)
{
    if (const auto instr = instrument.lock(); instr && m_deviceService) {
        const auto portName = instr->midiAddress().portName();
        if (m_deviceService->isInternalDevice(portName)) {
            if (const auto frame = m_deviceService->frameForTime(when); frame) {
                m_deviceService->scheduleMidiNoteOn(portName, data.note(), data.velocity(), *frame);
                return;
            }
        }
    }
    playNote(instrument, data);
}

void MidiService::stopNoteAt(InstrumentW instrument, MidiNoteDataCR data, std::chrono::steady_clock::time_point when)
{
    if (const auto instr = instrument.lock(); instr && m_deviceService) {
        const auto portName = instr->midiAddress().portName();
        if (m_deviceService->isInternalDevice(portName)) {
            if (const auto frame = m_deviceService->frameForTime(when); frame) {
                m_deviceService->scheduleMidiNoteOff(portName, data.note(), *frame);
                return;
            }
        }
    }
    stopNote(instrument, data);
}

void MidiService::stopNote(InstrumentW instrument, MidiNoteDataCR data)
{
    if (const auto instr = instrument.lock()) {
        const auto portName = instr->midiAddress().portName();
        if (m_deviceService && m_deviceService->isInternalDevice(portName)) {
            m_deviceService->processMidiNoteOff(portName, data.note());
        } else {
            m_outputWorker->stopNote(portName, instr->midiAddress().channel(), data.note());
        }
    }
}

void MidiService::stopAllNotes(InstrumentW instrument)
{
    if (const auto instr = instrument.lock()) {
        const auto portName = instr->midiAddress().portName();
        if (m_deviceService && m_deviceService->isInternalDevice(portName)) {
            m_deviceService->processMidiAllNotesOff(portName);
            m_deviceService->processMidiCc(portName, static_cast<uint8_t>(MidiCcMapping::Controller::ResetAllControllers), 127, instr->midiAddress().channel());
        } else {
            m_outputWorker->stopAllNotes(portName, instr->midiAddress().channel());
        }
    }
}

void MidiService::stopAllNotes()
{
    if (m_deviceService) {
        m_deviceService->processMidiAllNotesOff();
        for (auto && name : m_deviceService->internalDeviceNames()) {
            m_deviceService->processMidiCc(name.c_str(), static_cast<uint8_t>(MidiCcMapping::Controller::ResetAllControllers), 127, 0);
        }
    }
}

void MidiService::sendCcData(InstrumentW instrument, MidiCcDataCR data)
{
    if (const auto instr = instrument.lock()) {
        const auto portName = instr->midiAddress().portName();
        if (m_deviceService && m_deviceService->isInternalDevice(portName)) {
            m_deviceService->processMidiCc(portName, data.controller(), data.value(), instr->midiAddress().channel());
        } else {
            // Last line of defence for the wire: an internal device may accept values past the MIDI
            // 1.0 range, but those are not legal data bytes once they leave the application.
            m_outputWorker->sendCcData(portName, instr->midiAddress().channel(), data.controller(), std::min<uint8_t>(data.value(), 127));
        }
    }
}

void MidiService::sendClock(MidiService::InstrumentW instrument)
{
    if (const auto instr = instrument.lock()) {
        m_outputWorker->sendClock(instr->midiAddress().portName());
    }
}

void MidiService::sendStart(MidiService::InstrumentW instrument)
{
    if (const auto instr = instrument.lock()) {
        m_outputWorker->sendStart(instr->midiAddress().portName());
    }
}

void MidiService::sendStop(MidiService::InstrumentW instrument)
{
    if (const auto instr = instrument.lock()) {
        m_outputWorker->sendStop(instr->midiAddress().portName());
    }
}

void MidiService::sendPitchBendData(InstrumentW instrument, MidiService::PitchBendDataCR data)
{
    if (const auto instr = instrument.lock()) {
        const auto portName = instr->midiAddress().portName();
        if (m_deviceService && m_deviceService->isInternalDevice(portName)) {
            m_deviceService->processMidiPitchBend(portName, (static_cast<uint16_t>(data.msb()) << 7) | data.lsb(), instr->midiAddress().channel());
        } else {
            m_outputWorker->sendPitchBendData(portName, instr->midiAddress().channel(), data.msb(), data.lsb());
        }
    }
}

MidiService::~MidiService()
{
    juzzlin::L(TAG).info() << "Stopping worker threads";

    m_inputWorkerThread.exit();
    m_inputWorkerThread.wait();
}

} // namespace noteahead
