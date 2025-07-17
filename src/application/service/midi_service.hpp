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
class MidiAddress;
class MidiOut;
class MidiCcData;
class MidiNoteData;
class MidiInWorker;
class MidiOutWorker;
class PitchBendData;

class MidiService : public QObject
{
    Q_OBJECT

public:
    explicit MidiService(QObject * parent = nullptr);

    ~MidiService() override;

    Q_INVOKABLE QStringList outputPorts() const;

    // QML API
    Q_INVOKABLE void setIsPlaying(bool isPlaying);
    Q_INVOKABLE void playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity);

    // Internal API
    using InstrumentW = std::weak_ptr<Instrument>;
    using MidiAddressCR = const MidiAddress &;
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

    //! MIDI OUT signals
    void outputPortsChanged(const QStringList & portNames);
    void outputPortsAppeared(const QStringList & portNames);
    void outputPortsDisappeared(const QStringList & portNames);

    //! MIDI IN signals
    void inputPortsChanged(const QStringList & portNames);
    void inputPortsAppeared(const QStringList & portNames);
    void inputPortsDisappeared(const QStringList & portNames);
    void controllerPortChanged(QString portName);
    void noteOnReceived(MidiAddressCR address, MidiNoteDataCR data);
    void noteOffReceived(MidiAddressCR address, MidiNoteDataCR data);
    void pitchBendReceived(MidiAddressCR address, quint16 value); // 0–16383, center = 8192
    void polyAftertouchReceived(MidiAddressCR address, quint8 note, quint8 pressure);
    void aftertouchReceived(MidiAddressCR address, quint8 pressure); // Channel pressure
    void controlChangeReceived(MidiAddressCR address, quint8 controller, quint8 value);
    void programChangeReceived(MidiAddressCR address, quint8 program);
    void rpnReceived(MidiAddressCR address, quint8 msb, quint8 lsb, quint16 value);
    void nrpnReceived(MidiAddressCR address, quint8 msb, quint8 lsb, quint16 value);
    void sysExReceived(const QByteArray & data);

    //! For logging purposes
    void dataReceived(const QString & data);

    //! General signals
    void statusTextRequested(QString message);
    void instrumentRequestHandlingRequested(const InstrumentRequest & instrumentRequest);

private:
    void initializeWorkers();
    void initializeInputWorker();
    void initializeOutputWorker();
    void invokeSimpleFunction(MidiService::InstrumentW instrument, QString functionName);

    std::mutex m_outputWorkerMutex; // Calls to this service may become directly from PlayerWorker and also live notes from other sources
    std::unique_ptr<MidiOutWorker> m_outputWorker;
    QThread m_outputWorkerThread;
    QStringList m_outputPorts;

    std::unique_ptr<MidiInWorker> m_inputWorker;
    QThread m_inputWorkerThread;
    QStringList m_inputPorts;
};

} // namespace noteahead

#endif // MIDI_SERVICE_HPP
