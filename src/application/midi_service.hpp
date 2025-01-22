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

#ifndef MIDI_SERVICE_HPP
#define MIDI_SERVICE_HPP

#include <QObject>
#include <QTimer>

#include <memory>

#include "instrument_request.hpp"

namespace cacophony {

class InstrumentRequest;
class MidiBackend;

class MidiService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList availableMidiPorts READ availableMidiPorts NOTIFY availableMidiPortsChanged)

public:
    explicit MidiService(QObject * parent = nullptr);

    Q_INVOKABLE QStringList availableMidiPorts() const;

    Q_INVOKABLE void requestPatchChange(QString portName, uint8_t channel, uint8_t patch);

    void setIsPlaying(bool isPlaying);

public slots:
    void handleInstrumentRequest(const InstrumentRequest & instrumentRequest);

signals:
    void availableMidiPortsChanged();

    void statusTextRequested(QString message);

private:
    void processFailedInstrumentRequests();

    std::shared_ptr<MidiBackend> m_midiBackend;

    std::unique_ptr<QTimer> m_midiScanTimer;

    QStringList m_availableMidiPorts;

    bool m_isPlaying = false;

    std::map<QString, InstrumentRequest> m_failedInstrumentRequests;
};

} // namespace cacophony

#endif // MIDI_SERVICE_HPP
