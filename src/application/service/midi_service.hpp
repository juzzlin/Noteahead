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

#ifndef MIDI_SERVICE_HPP
#define MIDI_SERVICE_HPP

#include <QObject>
#include <QThread>

#include <memory>
#include <mutex>

#include "../instrument_request.hpp"

namespace noteahead {

class Instrument;
class MidiBackend;
class MidiCcData;
class MidiNoteData;
class MidiInputWorker;
class MidiOutputWorker;
class PitchBendData;

class MidiService : public QObject
{
    Q_OBJECT

public:
    explicit MidiService(QObject * parent = nullptr);

    ~MidiService() override;

    Q_INVOKABLE QStringList availableMidiPorts() const;

    // QML API
    Q_INVOKABLE void setIsPlaying(bool isPlaying);
    Q_INVOKABLE void playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity);

    // Internal API
    using InstrumentW = std::weak_ptr<Instrument>;
    using MidiNoteDataCR = const MidiNoteData &;
    Q_INVOKABLE void playNote(InstrumentW instrument, MidiNoteDataCR data);
    Q_INVOKABLE void stopNote(InstrumentW instrument, MidiNoteDataCR data);
    Q_INVOKABLE void stopAllNotes(InstrumentW instrument);
    using MidiCcDataCR = const MidiCcData &;
    Q_INVOKABLE void sendCcData(InstrumentW instrument, MidiCcDataCR data);
    Q_INVOKABLE void sendClock(InstrumentW instrument);
    Q_INVOKABLE void sendStart(InstrumentW instrument);
    Q_INVOKABLE void sendStop(InstrumentW instrument);
    using PitchBendDataCR = const PitchBendData &;
    Q_INVOKABLE void sendPitchBendData(InstrumentW instrument, PitchBendDataCR data);

public slots:
    void handleInstrumentRequest(const InstrumentRequest & instrumentRequest);
    void setControllerPort(QString portName);

signals:
    void availableMidiOutputPortsChanged(const QStringList & portNames);
    void midiOutputPortsAppeared(const QStringList & portNames);
    void midiOutputPortsDisappeared(const QStringList & portNames);

    void availableMidiInputPortsChanged(const QStringList & portNames);
    void midiInputPortsAppeared(const QStringList & portNames);
    void midiInputPortsDisappeared(const QStringList & portNames);

    void statusTextRequested(QString message);

    void instrumentRequestHandlingRequested(const InstrumentRequest & instrumentRequest);
    void controllerPortChanged(QString portName);

private:
    void initializeWorkers();
    void initializeInputWorker();
    void initializeOutputWorker();
    void invokeSimpleFunction(MidiService::InstrumentW instrument, QString functionName);

    std::mutex m_outputWorkerMutex; // Calls to this service may become directly from PlayerWorker and also live notes from other sources
    std::unique_ptr<MidiOutputWorker> m_outputWorker;
    QThread m_outputWorkerThread;
    QStringList m_availableMidiOutputPorts;

    std::unique_ptr<MidiInputWorker> m_inputWorker;
    QThread m_inputWorkerThread;
    QStringList m_availableMidiInputPorts;
};

} // namespace noteahead

#endif // MIDI_SERVICE_HPP
