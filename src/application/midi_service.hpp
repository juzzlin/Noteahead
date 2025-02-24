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

#include "instrument_request.hpp"

namespace noteahead {

class Instrument;
class InstrumentRequest;
class MidiBackend;
class MidiWorker;

class MidiService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList availableMidiPorts READ availableMidiPorts NOTIFY availableMidiPortsChanged)

public:
    explicit MidiService(QObject * parent = nullptr);

    ~MidiService() override;

    Q_INVOKABLE QStringList availableMidiPorts() const;

    void setIsPlaying(bool isPlaying);

    void playAndStopMiddleC(QString portName, uint8_t channel, uint8_t velocity);

    using InstrumentW = std::weak_ptr<Instrument>;

    void playNote(InstrumentW instrument, uint8_t midiNote, uint8_t velocity);
    void stopNote(InstrumentW instrument, uint8_t midiNote);
    void stopAllNotes(InstrumentW instrument);

    void sendClock(InstrumentW instrument);

public slots:
    void handleInstrumentRequest(const InstrumentRequest & instrumentRequest);

signals:
    void availableMidiPortsChanged(const QStringList & availableMidiPorts);
    void midiPortsAppeared(const QStringList & midiPorts);
    void midiPortsDisappeared(const QStringList & midiPorts);

    void statusTextRequested(QString message);

    void instrumentRequestHandlingRequested(const InstrumentRequest & instrumentRequest);

private:
    void initializeWorker();

    std::mutex m_workerMutex; // Calls to this service may become directly from PlayerWorker and also live notes from other sources

    std::unique_ptr<MidiWorker> m_midiWorker;

    QThread m_midiWorkerThread;

    QStringList m_availableMidiPorts;
};

} // namespace noteahead

#endif // MIDI_SERVICE_HPP
