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

#ifndef MIDI_IN_WORKER_H
#define MIDI_IN_WORKER_H

#include "midi_worker.hpp"

#include <QObject>
#include <QTimer>

#include <memory>

namespace noteahead {

class MidiIn;
class MidiDevice;

class MidiInWorker : public MidiWorker
{
    Q_OBJECT

public:
    explicit MidiInWorker(QObject * parent = nullptr);

public slots:
    void setControllerPort(QString portName);

private:
    void initializeScanTimer();

    using Message = std::vector<unsigned char>;
    using MessageCR = const Message &;
    void handleIncomingMessage(double deltaTime, MessageCR message);

    QString m_controllerPort;

    std::shared_ptr<MidiIn> m_midiIn;
    std::unique_ptr<QTimer> m_midiScanTimer;

    QStringList m_availablePorts;
};

} // namespace noteahead

#endif // MIDI_IN_WORKER_H
