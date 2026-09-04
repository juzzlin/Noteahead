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

#include <chrono>
#include <memory>
#include <optional>

#include "../instrument_request.hpp"

namespace noteahead {

class DeviceService;
class Instrument;
class MidiAddress;
class MidiBackendOut;
class MidiCcData;
class MidiNoteData;
class MidiWorkerIn;
class MidiWorkerOut;
class PitchBendData;

class MidiService : public QObject
{
    Q_OBJECT

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    explicit MidiService(DeviceServiceS deviceService, QObject * parent = nullptr);

    ~MidiService() override;

    virtual Q_INVOKABLE QStringList outputPorts() const;

    // QML API
    Q_INVOKABLE void setIsPlaying(bool isPlaying);
    Q_INVOKABLE void playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity);

    // Internal API
    using InstrumentW = std::weak_ptr<Instrument>;
    using MidiAddressCR = const MidiAddress &;
    using MidiNoteDataCR = const MidiNoteData &;
    virtual Q_INVOKABLE void playNote(InstrumentW instrument, MidiNoteDataCR data);
    virtual Q_INVOKABLE void stopNote(InstrumentW instrument, MidiNoteDataCR data);

    //! Whether this instrument is played by one of the internal devices rather than out of a port.
    virtual bool isInternalInstrument(InstrumentW instrument) const;

    //! How far ahead a note has to be handed over to land on its frame, or nothing when the engine
    //! cannot say. Virtual so a test can state it outright.
    virtual std::optional<std::chrono::steady_clock::duration> scheduleLookahead() const;

    //! Drops what the internal devices are still holding, so a stop leaves nothing to sound later.
    void clearScheduledEvents();

    //! Plays a note at the moment it is written for, rather than whenever the next block starts.
    //!
    //! Only an internal device can be told this, and only while the engine is rendering; everything
    //! else falls through to playNote and goes out the way it always did. Deliberately not virtual:
    //! it delegates to the virtual pair, so anything watching those still sees every note.
    void playNoteAt(InstrumentW instrument, MidiNoteDataCR data, std::chrono::steady_clock::time_point when);
    void stopNoteAt(InstrumentW instrument, MidiNoteDataCR data, std::chrono::steady_clock::time_point when);
    virtual Q_INVOKABLE void stopAllNotes(InstrumentW instrument);
    virtual Q_INVOKABLE void stopAllNotes();
    using MidiCcDataCR = const MidiCcData &;
    Q_INVOKABLE void sendCcData(InstrumentW instrument, MidiCcDataCR data);
    //! Controller moves go the same way as the notes.
    //!
    //! Not merely for their own accuracy: a value written on the same line as a note has to reach
    //! the device on the same frame as that note. Left immediate while the notes ran ahead, it would
    //! arrive a whole lookahead before the note it belongs to, which is worse than either being late.
    void sendCcDataAt(InstrumentW instrument, MidiCcDataCR data, std::chrono::steady_clock::time_point when);
    Q_INVOKABLE void sendClock(InstrumentW instrument);
    Q_INVOKABLE void sendStart(InstrumentW instrument);
    Q_INVOKABLE void sendStop(InstrumentW instrument);
    using PitchBendDataCR = const PitchBendData &;
    Q_INVOKABLE void sendPitchBendData(InstrumentW instrument, PitchBendDataCR data);
    //! Bend moves go the same way as the notes. See sendCcDataAt.
    void sendPitchBendDataAt(InstrumentW instrument, PitchBendDataCR data, std::chrono::steady_clock::time_point when);

public slots:
    void handleInstrumentRequest(const InstrumentRequest & instrumentRequest);
    void setControllerPort(QString portName);
    void setMidiSyncEnabled(bool enabled);

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
    void startReceived();
    void stopReceived();
    void continueReceived();
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

protected:
    MidiService(DeviceServiceS deviceService, QObject * parent, bool initializeRealWorkers);

private:
    //! Applies an instrument request to an internal device.
    //!
    //! Internal devices have no MIDI port to open, so the request cannot go through MidiWorkerOut.
    //! It is unrolled into the same messages the wire would carry instead, so that track settings
    //! reach a virtual instrument exactly like they reach external gear.
    void handleInternalDeviceInstrumentRequest(const InstrumentRequest & instrumentRequest);

    void initializeWorkers();
    void initializeInputWorker();
    void initializeOutputWorker();

    std::unique_ptr<MidiWorkerOut> m_outputWorker;
    QStringList m_outputPorts;

    std::unique_ptr<MidiWorkerIn> m_inputWorker;
    QThread m_inputWorkerThread;
    QStringList m_inputPorts;

    DeviceServiceS m_deviceService;
};

} // namespace noteahead

#endif // MIDI_SERVICE_HPP
