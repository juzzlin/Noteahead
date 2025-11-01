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
#include "midi_in_worker.hpp"
#include "midi_out_worker.hpp"

namespace noteahead {

static const auto TAG = "MidiService";

MidiService::MidiService(QObject * parent)
  : QObject { parent }
  , m_outputWorker { std::make_unique<MidiOutWorker>() }
  , m_inputWorker { std::make_unique<MidiInWorker>() }
{
    initializeWorkers();
}

void MidiService::initializeWorkers()
{
    initializeInputWorker();
    initializeOutputWorker();
}

void MidiService::initializeInputWorker()
{
    connect(this, &MidiService::controllerPortChanged, m_inputWorker.get(), &MidiInWorker::setControllerPort);

    connect(m_inputWorker.get(), &MidiInWorker::portsChanged, this, [this](const auto & midiPorts) {
        m_inputPorts = { "" };
        m_inputPorts.append(midiPorts);
        emit inputPortsChanged(m_inputPorts);
    });

    connect(m_inputWorker.get(), &MidiInWorker::portsAppeared, this, &MidiService::inputPortsAppeared);
    connect(m_inputWorker.get(), &MidiInWorker::portsDisappeared, this, &MidiService::inputPortsDisappeared);
    connect(m_inputWorker.get(), &MidiInWorker::statusTextRequested, this, &MidiService::statusTextRequested);

    connect(m_inputWorker.get(), &MidiInWorker::startReceived, this, &MidiService::startReceived);
    connect(m_inputWorker.get(), &MidiInWorker::stopReceived, this, &MidiService::stopReceived);
    connect(m_inputWorker.get(), &MidiInWorker::continueReceived, this, &MidiService::continueReceived);

    connect(m_inputWorker.get(), &MidiInWorker::noteOnReceived, this, &MidiService::noteOnReceived);
    connect(m_inputWorker.get(), &MidiInWorker::noteOffReceived, this, &MidiService::noteOffReceived);
    connect(m_inputWorker.get(), &MidiInWorker::pitchBendReceived, this, &MidiService::pitchBendReceived);

    connect(m_inputWorker.get(), &MidiInWorker::polyAftertouchReceived, this, &MidiService::polyAftertouchReceived);
    connect(m_inputWorker.get(), &MidiInWorker::aftertouchReceived, this, &MidiService::aftertouchReceived);
    connect(m_inputWorker.get(), &MidiInWorker::controlChangeReceived, this, &MidiService::controlChangeReceived);
    connect(m_inputWorker.get(), &MidiInWorker::programChangeReceived, this, &MidiService::programChangeReceived);
    connect(m_inputWorker.get(), &MidiInWorker::rpnReceived, this, &MidiService::rpnReceived);
    connect(m_inputWorker.get(), &MidiInWorker::nrpnReceived, this, &MidiService::nrpnReceived);
    connect(m_inputWorker.get(), &MidiInWorker::sysExReceived, this, &MidiService::sysExReceived);

    connect(m_inputWorker.get(), &MidiInWorker::dataReceived, this, &MidiService::dataReceived);

    m_inputWorker->moveToThread(&m_inputWorkerThread);
    m_inputWorkerThread.start(QThread::NormalPriority);
}

void MidiService::initializeOutputWorker()
{
    connect(this, &MidiService::instrumentRequestHandlingRequested, m_outputWorker.get(), &MidiOutWorker::handleInstrumentRequest);

    connect(m_outputWorker.get(), &MidiOutWorker::portsChanged, this, [this](const auto & midiPorts) {
        m_outputPorts = { "" };
        m_outputPorts.append(midiPorts);
        emit outputPortsChanged(m_outputPorts);
    });

    connect(m_outputWorker.get(), &MidiOutWorker::portsAppeared, this, &MidiService::outputPortsAppeared);
    connect(m_outputWorker.get(), &MidiOutWorker::portsDisappeared, this, &MidiService::outputPortsDisappeared);
    connect(m_outputWorker.get(), &MidiOutWorker::statusTextRequested, this, &MidiService::statusTextRequested);

    m_outputWorker->moveToThread(&m_outputWorkerThread);
    m_outputWorkerThread.start(QThread::HighPriority);
}

QStringList MidiService::outputPorts() const
{
    return m_outputPorts;
}

void MidiService::handleInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    Lock lock { m_outputWorkerMutex };

    emit instrumentRequestHandlingRequested(instrumentRequest);
}

void MidiService::setControllerPort(QString portName)
{
    emit controllerPortChanged(portName);
}

void MidiService::setIsPlaying(bool isPlaying)
{
    m_inputWorker->setIsPlaying(isPlaying);
    m_outputWorker->setIsPlaying(isPlaying);
}

void MidiService::playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity)
{
    Lock lock { m_outputWorkerMutex };

    if (const bool invoked = QMetaObject::invokeMethod(m_outputWorker.get(), "playAndStopMiddleC", Q_ARG(QString, portName), Q_ARG(quint8, channel), Q_ARG(quint8, velocity)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::playNote(InstrumentW instrument, MidiNoteDataCR data)
{
    Lock lock { m_outputWorkerMutex };

    if (const bool invoked = QMetaObject::invokeMethod(m_outputWorker.get(), "playNote",
                                                       Q_ARG(QString, instrument.lock()->midiAddress().portName()),
                                                       Q_ARG(quint8, instrument.lock()->midiAddress().channel()),
                                                       Q_ARG(quint8, data.note()),
                                                       Q_ARG(quint8, data.velocity()));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::stopNote(InstrumentW instrument, MidiNoteDataCR data)
{
    Lock lock { m_outputWorkerMutex };

    if (const bool invoked = QMetaObject::invokeMethod(m_outputWorker.get(), "stopNote",
                                                       Q_ARG(QString, instrument.lock()->midiAddress().portName()),
                                                       Q_ARG(quint8, instrument.lock()->midiAddress().channel()),
                                                       Q_ARG(quint8, data.note()));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::stopAllNotes(InstrumentW instrument)
{
    Lock lock { m_outputWorkerMutex };

    if (const bool invoked = QMetaObject::invokeMethod(m_outputWorker.get(), "stopAllNotes",
                                                       Q_ARG(QString, instrument.lock()->midiAddress().portName()),
                                                       Q_ARG(quint8, instrument.lock()->midiAddress().channel()));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::sendCcData(InstrumentW instrument, MidiCcDataCR data)
{
    Lock lock { m_outputWorkerMutex };

    if (const bool invoked = QMetaObject::invokeMethod(m_outputWorker.get(), "sendCcData",
                                                       Q_ARG(QString, instrument.lock()->midiAddress().portName()),
                                                       Q_ARG(quint8, instrument.lock()->midiAddress().channel()),
                                                       Q_ARG(quint8, data.controller()),
                                                       Q_ARG(quint8, data.value()));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::invokeSimpleFunction(MidiService::InstrumentW instrument, QString functionName)
{
    Lock lock { m_outputWorkerMutex };
    if (const bool invoked = QMetaObject::invokeMethod(m_outputWorker.get(), functionName.toStdString().c_str(),
                                                       Q_ARG(QString, instrument.lock()->midiAddress().portName()));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName.toStdString();
    }
}

void MidiService::sendClock(MidiService::InstrumentW instrument)
{
    invokeSimpleFunction(instrument, "sendClock");
}

void MidiService::sendStart(MidiService::InstrumentW instrument)
{
    invokeSimpleFunction(instrument, "sendStart");
}

void MidiService::sendStop(MidiService::InstrumentW instrument)
{
    invokeSimpleFunction(instrument, "sendStop");
}

void MidiService::sendPitchBendData(InstrumentW instrument, MidiService::PitchBendDataCR data)
{
    Lock lock { m_outputWorkerMutex };

    if (const bool invoked = QMetaObject::invokeMethod(m_outputWorker.get(), "sendPitchBendData",
                                                       Q_ARG(QString, instrument.lock()->midiAddress().portName()),
                                                       Q_ARG(quint8, instrument.lock()->midiAddress().channel()),
                                                       Q_ARG(quint8, data.msb()),
                                                       Q_ARG(quint8, data.lsb()));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

MidiService::~MidiService()
{
    juzzlin::L(TAG).info() << "Stopping worker threads";

    m_inputWorkerThread.exit();
    m_inputWorkerThread.wait();

    m_outputWorkerThread.exit();
    m_outputWorkerThread.wait();
}

} // namespace noteahead
