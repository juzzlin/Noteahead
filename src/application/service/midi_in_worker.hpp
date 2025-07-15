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

#ifndef MIDI_IN_WORKER_HPP
#define MIDI_IN_WORKER_HPP

#include "midi_worker.hpp"

#include <QObject>

#include <memory>
#include <unordered_map>

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

signals:
    void noteOnReceived(quint8 channel, quint8 note, quint8 velocity);
    void noteOffReceived(quint8 channel, quint8 note);

    void polyAftertouchReceived(quint8 channel, quint8 note, quint8 pressure);
    void aftertouchReceived(quint8 channel, quint8 pressure); // Channel pressure

    void controlChangeReceived(quint8 channel, quint8 controller, quint8 value);
    void programChangeReceived(quint8 channel, quint8 program);

    void pitchBendReceived(quint8 channel, quint16 value); // 0–16383, center = 8192

    void rpnReceived(quint8 channel, quint8 msb, quint8 lsb, quint16 value);
    void nrpnReceived(quint8 channel, quint8 msb, quint8 lsb, quint16 value);

    void sysExReceived(const QByteArray & data);

protected:
    void handlePortsChanged() override;

private:
    void initializeScanTimer();

    using Message = std::vector<unsigned char>;
    using MessageCR = const Message &;
    void handleIncomingMessage(double deltaTime, MessageCR message);

    void handleNoteOff(quint8 channel, MessageCR message);
    void handleNoteOn(quint8 channel, MessageCR message);

    void handlePolyAftertouch(quint8 channel, MessageCR message);
    void handleControlChange(quint8 channel, MessageCR message);

    void handleProgramChange(quint8 channel, MessageCR message);

    void handleChannelAftertouch(quint8 channel, MessageCR message);

    void handlePitchBend(quint8 channel, MessageCR message);

    void handleSysEx(MessageCR message);

    void handleRpnOrNrpn(quint8 channel, quint8 controller, quint8 value);
    void updateRpnState(quint8 channel, quint8 controller, quint8 value);
    void updateNrpnState(quint8 channel, quint8 controller, quint8 value);
    void resetRpnNrpnState(quint8 channel);

    void logMidiMessage(double deltaTime, MessageCR message);

    QString m_controllerPort;

    std::shared_ptr<MidiIn> m_midiIn;

    using RpnStateMap = std::unordered_map<quint8, std::optional<std::pair<quint8, quint8>>>;
    RpnStateMap m_rpnState;
    RpnStateMap m_nrpnState;
};

} // namespace noteahead

#endif // MIDI_IN_WORKER_HPP
