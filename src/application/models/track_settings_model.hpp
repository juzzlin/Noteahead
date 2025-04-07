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

class TrackSettingsModel : public MidiCcSelectionModel
{
    Q_OBJECT

    Q_PROPERTY(quint64 trackIndex READ trackIndex WRITE setTrackIndex NOTIFY trackIndexChanged)

    Q_PROPERTY(QStringList availableMidiPorts READ availableMidiPorts NOTIFY availableMidiPortsChanged)
    Q_PROPERTY(QString portName READ portName WRITE setPortName NOTIFY portNameChanged)

    Q_PROPERTY(quint8 channel READ channel WRITE setChannel NOTIFY channelChanged)

    Q_PROPERTY(quint8 cutoff READ cutoff WRITE setCutoff NOTIFY cutoffChanged)
    Q_PROPERTY(bool cutoffEnabled READ cutoffEnabled WRITE setCutoffEnabled NOTIFY cutoffEnabledChanged)

    Q_PROPERTY(bool patchEnabled READ patchEnabled WRITE setPatchEnabled NOTIFY patchEnabledChanged)
    Q_PROPERTY(quint8 patch READ patch WRITE setPatch NOTIFY patchChanged)

    Q_PROPERTY(bool bankEnabled READ bankEnabled WRITE setBankEnabled NOTIFY bankEnabledChanged)
    Q_PROPERTY(quint8 bankLsb READ bankLsb WRITE setBankLsb NOTIFY bankLsbChanged)
    Q_PROPERTY(quint8 bankMsb READ bankMsb WRITE setBankMsb NOTIFY bankMsbChanged)
    Q_PROPERTY(bool bankByteOrderSwapped READ bankByteOrderSwapped WRITE setBankByteOrderSwapped NOTIFY bankByteOrderSwappedChanged)

    Q_PROPERTY(quint8 pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(bool panEnabled READ panEnabled WRITE setPanEnabled NOTIFY panEnabledChanged)

    Q_PROPERTY(quint8 volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool volumeEnabled READ volumeEnabled WRITE setVolumeEnabled NOTIFY volumeEnabledChanged)

    Q_PROPERTY(bool sendMidiClock READ sendMidiClock WRITE setSendMidiClock NOTIFY sendMidiClockChanged)
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged)

public:
    explicit TrackSettingsModel(QObject * parent = nullptr);

    ~TrackSettingsModel() override;

    Q_INVOKABLE void applyAll();

    Q_INVOKABLE void requestInstrumentData();
    Q_INVOKABLE void requestTestSound(quint8 velocity);

    Q_INVOKABLE void save();

    QStringList availableMidiPorts() const;
    void setAvailableMidiPorts(QStringList portNames);

    quint64 trackIndex() const;
    void setTrackIndex(quint64 trackIndex);

    void setInstrumentData(const Instrument &);

    void reset();

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

    quint8 cutoff() const;
    void setCutoff(quint8 cutoff);

    bool cutoffEnabled() const;
    void setCutoffEnabled(bool enabled);

    quint8 pan() const;
    void setPan(quint8 volume);

    bool panEnabled() const;
    void setPanEnabled(bool enabled);

    quint8 patch() const;
    void setPatch(quint8 patch);

    quint8 volume() const;
    void setVolume(quint8 volume);
    bool volumeEnabled() const;
    void setVolumeEnabled(bool enabled);

    bool sendMidiClock() const;
    void setSendMidiClock(bool enabled);

    int delay() const;
    void setDelay(int delay);

signals:
    void applyAllRequested();
    void applyPatchRequested();

    void availableMidiPortsChanged();

    void bankByteOrderSwappedChanged();
    void bankEnabledChanged();
    void bankLsbChanged();
    void bankMsbChanged();

    void channelChanged();

    void cutoffChanged();
    void cutoffEnabledChanged();

    void instrumentDataReceived();
    void instrumentDataRequested();

    void panChanged();
    void panEnabledChanged();

    void patchChanged();
    void patchEnabledChanged();

    void portNameChanged();

    void saveRequested();

    void testSoundRequested(quint8 velocity);

    void trackIndexChanged();

    void volumeChanged();
    void volumeEnabledChanged();

    void sendMidiClockChanged();

    void delayChanged();

private:
    void pushApplyDisabled();
    void popApplyDisabled();

    QString m_instrumentPortName;

    QString m_portName;
    QStringList m_availableMidiPorts;

    bool m_applyDisabled = false;
    bool m_bankByteOrderSwapped { false };
    bool m_bankEnabled { false };
    bool m_cutoffEnabled { false };
    bool m_panEnabled { false };
    bool m_patchEnabled { false };
    bool m_volumeEnabled { false };
    bool m_sendMidiClock { false };

    quint64 m_trackIndex { 0 };

    std::vector<bool> m_applyDisabledStack;

    quint8 m_bankLsb { 0 };
    quint8 m_bankMsb { 0 };

    quint8 m_channel { 0 };

    const quint8 m_defaultCutoff { 127 };
    quint8 m_cutoff { m_defaultCutoff };

    quint8 m_patch { 0 };

    const quint8 m_defaultPan { 64 };
    quint8 m_pan { m_defaultPan };

    const quint8 m_defaultVolume { 127 };
    quint8 m_volume { m_defaultVolume };

    int m_delay { 0 };
};

} // namespace noteahead

#endif // TRACK_SETTINGS_MODEL_HPP
