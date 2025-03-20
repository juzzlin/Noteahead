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

#ifndef MIDI_WORKER_HPP
#define MIDI_WORKER_HPP

#include <QObject>
#include <QTimer>

#include <memory>
#include <vector>

#include "instrument_request.hpp"

namespace noteahead {

class Instrument;
class InstrumentRequest;
class MidiBackend;
class MidiDevice;

class MidiWorker : public QObject
{
    Q_OBJECT

public:
    explicit MidiWorker(QObject * parent = nullptr);

    Q_INVOKABLE void handleInstrumentRequest(const InstrumentRequest & instrumentRequest);

    Q_INVOKABLE void playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity);
    Q_INVOKABLE void playNote(QString portName, quint8 channel, quint8 midiNote, quint8 velocity);

    Q_INVOKABLE void stopNote(QString portName, quint8 channel, quint8 midiNote);
    Q_INVOKABLE void stopAllNotes(QString portName, quint8 channel);

    Q_INVOKABLE void sendClock(QString portName);
    Q_INVOKABLE void sendCcData(QString portName, quint8 channel, quint8 controller, quint8 value);

    Q_INVOKABLE void requestPatchChange(QString portName, quint8 channel, quint8 patch);

    Q_INVOKABLE void setIsPlaying(bool isPlaying);

signals:
    void availableMidiPortsChanged(QStringList availableMidiPorts);
    void midiPortsAppeared(QStringList midiPorts);
    void midiPortsDisappeared(QStringList midiPorts);

    void statusTextRequested(QString message);

private:
    void initializeScanTimer();
    void initializeStopTimer();

    void sendMidiCcSettings(const MidiDevice & midiDevice, const Instrument & instrument);

    std::shared_ptr<MidiBackend> m_midiBackend;

    std::unique_ptr<QTimer> m_midiScanTimer;

    std::unique_ptr<QTimer> m_midiStopTimer;

    struct StopTask
    {
        QString portName;

        quint8 channel = 0;

        quint8 note = 0;
    };

    std::vector<StopTask> m_stopTasks;

    QStringList m_availableMidiPorts;

    std::atomic_bool m_isPlaying = false;
};

} // namespace noteahead

#endif // MIDI_WORKER_HPP
