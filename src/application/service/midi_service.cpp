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
#include "../../domain/midi_cc_data.hpp"
#include "../../domain/midi_note_data.hpp"
#include "../../domain/pitch_bend_data.hpp"
#include "../instrument_request.hpp"
#include "midi_output_worker.hpp"

namespace noteahead {

static const auto TAG = "MidiService";

MidiService::MidiService(QObject * parent)
  : QObject { parent }
  , m_outputWorker { std::make_unique<MidiOutputWorker>() }
{
    initializeOutputWorker();
}

void MidiService::initializeOutputWorker()
{
    connect(this, &MidiService::instrumentRequestHandlingRequested, m_outputWorker.get(), &MidiOutputWorker::handleInstrumentRequest);

    connect(m_outputWorker.get(), &MidiOutputWorker::availableMidiPortsChanged, this, [this](const auto & midiPorts) {
        m_availableMidiOutputPorts = { "" };
        m_availableMidiOutputPorts.append(midiPorts);
        emit availableMidiOutputPortsChanged(m_availableMidiOutputPorts);
    });

    connect(m_outputWorker.get(), &MidiOutputWorker::midiPortsAppeared, this, &MidiService::midiOutputPortsAppeared);
    connect(m_outputWorker.get(), &MidiOutputWorker::midiPortsDisappeared, this, &MidiService::midiOutputPortsDisappeared);
    connect(m_outputWorker.get(), &MidiOutputWorker::statusTextRequested, this, &MidiService::statusTextRequested);

    m_outputWorker->moveToThread(&m_outputWorkerThread);
    m_outputWorkerThread.start(QThread::HighPriority);
}

QStringList MidiService::availableMidiPorts() const
{
    return m_availableMidiOutputPorts;
}

void MidiService::handleInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    std::lock_guard<std::mutex> lock { m_outputWorkerMutex };

    emit instrumentRequestHandlingRequested(instrumentRequest);
}

void MidiService::setControllerPort(QString portName)
{
    emit controllerPortChanged(portName);
}

void MidiService::setIsPlaying(bool isPlaying)
{
    m_outputWorker->setIsPlaying(isPlaying);
}

void MidiService::playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity)
{
    std::lock_guard<std::mutex> lock { m_outputWorkerMutex };

    if (const bool invoked = QMetaObject::invokeMethod(m_outputWorker.get(), "playAndStopMiddleC", Q_ARG(QString, portName), Q_ARG(quint8, channel), Q_ARG(quint8, velocity)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::playNote(InstrumentW instrument, MidiNoteDataCR data)
{
    std::lock_guard<std::mutex> lock { m_outputWorkerMutex };

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
    std::lock_guard<std::mutex> lock { m_outputWorkerMutex };

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
    std::lock_guard<std::mutex> lock { m_outputWorkerMutex };

    if (const bool invoked = QMetaObject::invokeMethod(m_outputWorker.get(), "stopAllNotes",
                                                       Q_ARG(QString, instrument.lock()->midiAddress().portName()),
                                                       Q_ARG(quint8, instrument.lock()->midiAddress().channel()));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::sendCcData(InstrumentW instrument, MidiCcDataCR data)
{
    std::lock_guard<std::mutex> lock { m_outputWorkerMutex };

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
    std::lock_guard<std::mutex> lock { m_outputWorkerMutex };
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
    std::lock_guard<std::mutex> lock { m_outputWorkerMutex };

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
    m_outputWorkerThread.exit();
    m_outputWorkerThread.wait();
}

} // namespace noteahead
