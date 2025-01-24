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

#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/instrument.hpp"
#include "instrument_request.hpp"
#include "midi_worker.hpp"

namespace noteahead {

static const auto TAG = "MidiService";

MidiService::MidiService(QObject * parent)
  : QObject { parent }
  , m_midiWorker { std::make_unique<MidiWorker>() }
{
    initializeWorker();
}

void MidiService::initializeWorker()
{
    connect(this, &MidiService::instrumentRequestHandlingRequested, m_midiWorker.get(), &MidiWorker::handleInstrumentRequest);

    connect(m_midiWorker.get(), &MidiWorker::availableMidiPortsChanged, this, [this](const auto & midiPorts) {
        m_availableMidiPorts = midiPorts;
        emit availableMidiPortsChanged();
    });

    m_midiWorker->moveToThread(&m_midiWorkerThread);
    m_midiWorkerThread.start(QThread::HighPriority);
}

QStringList MidiService::availableMidiPorts() const
{
    return m_availableMidiPorts;
}

void MidiService::handleInstrumentRequest(const InstrumentRequest & instrumentRequest)
{
    emit instrumentRequestHandlingRequested(instrumentRequest);
}

void MidiService::setIsPlaying(bool isPlaying)
{
    m_midiWorker->setIsPlaying(isPlaying);
}

void MidiService::playAndStopMiddleC(QString portName, uint8_t channel, uint8_t velocity)
{
    if (const bool invoked = QMetaObject::invokeMethod(m_midiWorker.get(), "playAndStopMiddleC", Q_ARG(QString, portName), Q_ARG(uint8_t, channel), Q_ARG(uint8_t, velocity)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::playNote(InstrumentS instrument, uint8_t midiNote, uint8_t velocity)
{
    if (const bool invoked = QMetaObject::invokeMethod(m_midiWorker.get(), "playNote",
                                                       Q_ARG(QString, instrument->portName),
                                                       Q_ARG(uint8_t, instrument->channel),
                                                       Q_ARG(uint8_t, midiNote),
                                                       Q_ARG(uint8_t, velocity));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::stopNote(InstrumentS instrument, uint8_t midiNote)
{
    if (const bool invoked = QMetaObject::invokeMethod(m_midiWorker.get(), "stopNote",
                                                       Q_ARG(QString, instrument->portName),
                                                       Q_ARG(uint8_t, instrument->channel),
                                                       Q_ARG(uint8_t, midiNote));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

void MidiService::stopAllNotes(InstrumentS instrument)
{
    if (const bool invoked = QMetaObject::invokeMethod(m_midiWorker.get(), "stopAllNotes",
                                                       Q_ARG(QString, instrument->portName),
                                                       Q_ARG(uint8_t, instrument->channel));
        !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!";
    }
}

MidiService::~MidiService()
{
    m_midiWorkerThread.exit();
    m_midiWorkerThread.wait();
}

} // namespace noteahead
