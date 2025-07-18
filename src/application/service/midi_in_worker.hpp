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

#include "../../domain/midi_address.hpp"

#include <QObject>

#include <map>
#include <memory>
#include <unordered_map>

namespace noteahead {

class MidiIn;
class MidiDevice;
class MidiNoteData;

class MidiInWorker : public MidiWorker
{
    Q_OBJECT

public:
    explicit MidiInWorker(QObject * parent = nullptr);

    using MidiAddressCR = const MidiAddress &;
    using MidiNoteDataCR = const MidiNoteData &;

public slots:
    void setControllerPort(QString portName);

signals:
    void startReceived();
    void stopReceived();
    void continueReceived();

    void noteOnReceived(MidiAddressCR address, MidiNoteDataCR data);
    void noteOffReceived(MidiAddressCR address, MidiNoteDataCR data);
    void pitchBendReceived(MidiAddressCR address, quint16 value);
    void controlChangeReceived(MidiAddressCR address, quint8 controller, quint8 value);

    void polyAftertouchReceived(MidiAddressCR address, quint8 note, quint8 pressure);
    void aftertouchReceived(MidiAddressCR address, quint8 pressure); // Channel pressure

    void programChangeReceived(MidiAddressCR address, quint8 program);

    void rpnReceived(MidiAddressCR address, quint8 msb, quint8 lsb, quint16 value);
    void nrpnReceived(MidiAddressCR address, quint8 msb, quint8 lsb, quint16 value);

    void sysExReceived(const QByteArray & data);

    //! For logging purposes
    void dataReceived(const QString & data);

protected:
    void handlePortsChanged() override;

private:
    void initializeScanTimer();

    MidiAddress currentAddress(uint8_t channel) const;

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

    //! Custom transport mappings for PITA devices.
    std::map<quint8, std::function<void()>> m_transportMappings = {
        { 54, [this]() { emit startReceived(); } }, // Arturia Keystep Play/Pause
        { 55, [this]() { emit stopReceived(); } }, // if Keystep sends Stop on a CC too
        // Extend with more mappings as needed
    };
};

} // namespace noteahead

#endif // MIDI_IN_WORKER_HPP
