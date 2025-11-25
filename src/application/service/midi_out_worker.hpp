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

#ifndef MIDI_OUT_WORKER_HPP
#define MIDI_OUT_WORKER_HPP

#include "midi_worker.hpp"

#include <QTimer>

#include <memory>
#include <vector>

#include "../instrument_request.hpp"

namespace noteahead {

class Instrument;
class InstrumentRequest;
class MidiOut;
class MidiPort;

class MidiOutWorker : public MidiWorker
{
    Q_OBJECT

public:
    explicit MidiOutWorker(QObject * parent = nullptr);

    Q_INVOKABLE void handleInstrumentRequest(const InstrumentRequest & instrumentRequest);

    Q_INVOKABLE void playAndStopMiddleC(QString portName, quint8 channel, quint8 velocity);
    Q_INVOKABLE void playNote(QString portName, quint8 channel, quint8 midiNote, quint8 velocity);

    Q_INVOKABLE void stopNote(QString portName, quint8 channel, quint8 midiNote);
    Q_INVOKABLE void stopAllNotes(QString portName, quint8 channel);

    Q_INVOKABLE void sendClock(QString portName);
    Q_INVOKABLE void sendStart(QString portName);
    Q_INVOKABLE void sendStop(QString portName);

    Q_INVOKABLE void sendCcData(QString portName, quint8 channel, quint8 controller, quint8 value);
    Q_INVOKABLE void sendPitchBendData(QString portName, quint8 channel, quint8 msb, quint8 lsb);

    Q_INVOKABLE void requestPatchChange(QString portName, quint8 channel, quint8 patch);

private:
    using MidiPortS = std::shared_ptr<MidiPort>;
    void applyBank(const Instrument & instrument, MidiPortS port);
    void applyPatch(const Instrument & instrument, MidiPortS port);
    void initializeStopTimer();

    void sendMidiCcSettings(const MidiPort & port, const Instrument & instrument);

    std::shared_ptr<MidiOut> m_midiOut;
    std::unique_ptr<QTimer> m_midiStopTimer;

    struct StopTask
    {
        QString portName;

        quint8 channel = 0;

        quint8 note = 0;
    };

    std::vector<StopTask> m_stopTasks;
};

} // namespace noteahead

#endif // MIDI_OUT_WORKER_HPP
