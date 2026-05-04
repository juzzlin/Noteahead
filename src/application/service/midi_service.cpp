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

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/instrument.hpp"
#include "../../domain/midi_address.hpp"
#include "../../domain/midi_cc_data.hpp"
#include "../../domain/midi_note_data.hpp"
#include "../../domain/pitch_bend_data.hpp"
#include "../instrument_request.hpp"
#include "device_service.hpp"
#include "midi_worker_in.hpp"
#include "midi_worker_out.hpp"

namespace noteahead {

static const auto TAG = "MidiService";

MidiService::MidiService(DeviceServiceS deviceService, QObject * parent)
  : MidiService(std::move(deviceService), parent, true)
{
}

MidiService::MidiService(DeviceServiceS deviceService, QObject * parent, bool initializeRealWorkers)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
{
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
        return;
    }
    m_outputWorker->handleInstrumentRequest(instrumentRequest);
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
        } else {
            m_outputWorker->stopAllNotes(portName, instr->midiAddress().channel());
        }
    }
}

void MidiService::stopAllNotes()
{
    if (m_deviceService) {
        m_deviceService->processMidiAllNotesOff();
    }
}

void MidiService::sendCcData(InstrumentW instrument, MidiCcDataCR data)
{
    if (const auto instr = instrument.lock()) {
        const auto portName = instr->midiAddress().portName();
        if (m_deviceService && m_deviceService->isInternalDevice(portName)) {
            m_deviceService->processMidiCc(portName, data.controller(), data.value(), instr->midiAddress().channel());
        } else {
            m_outputWorker->sendCcData(portName, instr->midiAddress().channel(), data.controller(), data.value());
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
            // FIXME: processMidiPitchBend(portName, data.msb(), data.lsb())
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