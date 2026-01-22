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

#ifndef TRACK_SETTINGS_MODEL_HPP
#define TRACK_SETTINGS_MODEL_HPP

#include "../../domain/instrument.hpp"
#include "midi_cc_selection_model.hpp"

#include <memory>

#include <QObject>

namespace noteahead {

class EditorService;
class Instrument;

class TrackSettingsModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(quint64 trackIndex READ trackIndex WRITE setTrackIndex NOTIFY trackIndexChanged)

    Q_PROPERTY(QStringList availableMidiPorts READ availableMidiPorts NOTIFY availableMidiPortsChanged)
    Q_PROPERTY(QString portName READ portName WRITE setPortName NOTIFY portNameChanged)

    Q_PROPERTY(quint8 channel READ channel WRITE setChannel NOTIFY channelChanged)

    Q_PROPERTY(bool patchEnabled READ patchEnabled WRITE setPatchEnabled NOTIFY patchEnabledChanged)
    Q_PROPERTY(quint8 patch READ patch WRITE setPatch NOTIFY patchChanged)

    Q_PROPERTY(bool bankEnabled READ bankEnabled WRITE setBankEnabled NOTIFY bankEnabledChanged)
    Q_PROPERTY(quint8 bankLsb READ bankLsb WRITE setBankLsb NOTIFY bankLsbChanged)
    Q_PROPERTY(quint8 bankMsb READ bankMsb WRITE setBankMsb NOTIFY bankMsbChanged)
    Q_PROPERTY(bool bankByteOrderSwapped READ bankByteOrderSwapped WRITE setBankByteOrderSwapped NOTIFY bankByteOrderSwappedChanged)

    Q_PROPERTY(bool sendMidiClock READ sendMidiClock WRITE setSendMidiClock NOTIFY sendMidiClockChanged)
    Q_PROPERTY(bool sendTransport READ sendTransport WRITE setSendTransport NOTIFY sendTransportChanged)

    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged)
    Q_PROPERTY(int transpose READ transpose WRITE setTranspose NOTIFY transposeChanged)
    Q_PROPERTY(int velocityJitter READ velocityJitter WRITE setVelocityJitter NOTIFY velocityJitterChanged)
    Q_PROPERTY(int velocityKeyTrack READ velocityKeyTrack WRITE setVelocityKeyTrack NOTIFY velocityKeyTrackChanged)
    Q_PROPERTY(int velocityKeyTrackOffset READ velocityKeyTrackOffset WRITE setVelocityKeyTrackOffset NOTIFY velocityKeyTrackOffsetChanged)
    Q_PROPERTY(int autoNoteOffOffset READ autoNoteOffOffset WRITE setAutoNoteOffOffset NOTIFY autoNoteOffOffsetChanged)
    Q_PROPERTY(bool autoNoteOffOffsetEnabled READ autoNoteOffOffsetEnabled WRITE setAutoNoteOffOffsetEnabled NOTIFY autoNoteOffOffsetEnabledChanged)
    
    Q_PROPERTY(MidiCcSelectionModel* midiCcModel READ midiCcModel CONSTANT)

public:
    explicit TrackSettingsModel(QObject * parent = nullptr);
    ~TrackSettingsModel() override;

    Q_INVOKABLE void applyAll();
    Q_INVOKABLE void applyMidiCc();

    Q_INVOKABLE void requestInstrumentData();
    void setInstrumentData(const Instrument &);

    Q_INVOKABLE void requestNoteOff(quint8 note);
    Q_INVOKABLE void requestNoteOn(quint8 note, quint8 velocity);
    Q_INVOKABLE void requestTestSound(quint8 velocity);

    Q_INVOKABLE void save();
    void reset();

    QStringList availableMidiPorts() const;
    void setAvailableMidiPorts(QStringList portNames);

    quint64 trackIndex() const;
    void setTrackIndex(quint64 trackIndex);

    using InstrumentU = std::unique_ptr<Instrument>;
    InstrumentU toInstrument() const;

    QString portName() const;
    void setPortName(const QString & name);

    bool bankByteOrderSwapped() const;
    void setBankByteOrderSwapped(bool swapped);

    bool bankEnabled() const;
    void setBankEnabled(bool enabled);

    bool patchEnabled() const;
    void setPatchEnabled(bool enabled);

    quint8 bankLsb() const;
    void setBankLsb(quint8 lsb);

    quint8 bankMsb() const;
    void setBankMsb(quint8 msb);

    quint8 channel() const;
    void setChannel(quint8 channel);

    quint8 patch() const;
    void setPatch(quint8 patch);

    bool sendMidiClock() const;
    void setSendMidiClock(bool enabled);
    bool sendTransport() const;
    void setSendTransport(bool enabled);

    int delay() const;
    void setDelay(int delay);
    int transpose() const;
    void setTranspose(int transpose);
    int velocityJitter() const;
    void setVelocityJitter(int velocityJitter);
    int velocityKeyTrack() const;
    void setVelocityKeyTrack(int velocityKeyTrack);
    int velocityKeyTrackOffset() const;
    void setVelocityKeyTrackOffset(int velocityKeyTrackOffset);
    int autoNoteOffOffset() const;
    void setAutoNoteOffOffset(int autoNoteOffOffset);
    bool autoNoteOffOffsetEnabled() const;
    void setAutoNoteOffOffsetEnabled(bool enabled);
    
    MidiCcSelectionModel* midiCcModel() const;

signals:
    void applyAllRequested();
    void applyMidiCcRequested();

    void applyPatchRequested();

    void availableMidiPortsChanged();

    void bankByteOrderSwappedChanged();
    void bankEnabledChanged();
    void bankLsbChanged();
    void bankMsbChanged();

    void channelChanged();

    void instrumentDataReceived();
    void instrumentDataRequested();

    void patchChanged();
    void patchEnabledChanged();

    void portNameChanged();

    void saveRequested();

    void noteOffRequested(quint8 note);
    void noteOnRequested(quint8 note, quint8 velocity);
    void testSoundRequested(quint8 velocity);

    void trackIndexChanged();

    void sendMidiClockChanged();
    void sendTransportChanged();

    void delayChanged();
    void transposeChanged();
    void velocityJitterChanged();
    void velocityKeyTrackChanged();
    void velocityKeyTrackOffsetChanged();
    void autoNoteOffOffsetChanged();
    void autoNoteOffOffsetEnabledChanged();

private:
    void pushApplyDisabled();
    void popApplyDisabled();

    struct InstrumentSettings
    {
        QString portName;
        quint8 channel { 0 };
        bool patchEnabled { false };
        quint8 patch { 0 };
        bool bankEnabled { false };
        quint8 bankLsb { 0 };
        quint8 bankMsb { 0 };
        bool bankByteOrderSwapped { false };
        int transpose { 0 };
    };

    InstrumentSettings m_instrumentSettings;

    struct TimingSettings
    {
        bool sendMidiClock { false };
        bool sendTransport { false };
        int delay { 0 };
        bool autoNoteOffOffsetEnabled { false };
        int autoNoteOffOffset { 0 };
    };

    TimingSettings m_timingSettings;

    struct MidiEffectSettings
    {
        int velocityJitter { 0 };
        int velocityKeyTrack { 0 };
        int velocityKeyTrackOffset { 0 };
    };

    MidiEffectSettings m_midiEffectSettings;

    QString m_instrumentPortName;
    QStringList m_availableMidiPorts;

    bool m_applyDisabled { false };
    std::vector<bool> m_applyDisabledStack;

    quint64 m_trackIndex { 0 };
    
    MidiCcSelectionModel* m_midiCcModel;
};

} // namespace noteahead

#endif // TRACK_SETTINGS_MODEL_HPP