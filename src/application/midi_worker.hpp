// This file is part of Cacophony.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef MIDI_WORKER_H
#define MIDI_WORKER_H

#include <QObject>
#include <QTimer>

#include <memory>

#include "instrument_request.hpp"

namespace cacophony {

class Instrument;
class InstrumentRequest;
class MidiBackend;

class MidiWorker : public QObject
{
    Q_OBJECT

public:
    explicit MidiWorker(QObject * parent = nullptr);

    Q_INVOKABLE void handleInstrumentRequest(const InstrumentRequest & instrumentRequest);

    Q_INVOKABLE void playAndStopMiddleC(QString portName, uint8_t channel, uint8_t velocity);

    Q_INVOKABLE void playNote(QString portName, uint8_t channel, uint8_t midiNote, uint8_t velocity);

    Q_INVOKABLE void stopNote(QString portName, uint8_t channel, uint8_t midiNote);

    Q_INVOKABLE void requestPatchChange(QString portName, uint8_t channel, uint8_t patch);

    Q_INVOKABLE void setIsPlaying(bool isPlaying);

signals:
    void availableMidiPortsChanged(QStringList availableMidiPorts);

    void statusTextRequested(QString message);

private:
    void processFailedInstrumentRequests();

    std::shared_ptr<MidiBackend> m_midiBackend;

    std::unique_ptr<QTimer> m_midiScanTimer;

    QStringList m_availableMidiPorts;

    std::atomic_bool m_isPlaying = false;

    std::map<QString, InstrumentRequest> m_failedInstrumentRequests;
};

} // namespace cacophony

#endif // MIDI_WORKER_H
