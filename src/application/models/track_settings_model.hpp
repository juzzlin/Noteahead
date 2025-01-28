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

#include <memory>

#include <QObject>

namespace noteahead {

class EditorService;
class Instrument;

class TrackSettingsModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(uint32_t trackIndex READ trackIndex WRITE setTrackIndex NOTIFY trackIndexChanged)
    Q_PROPERTY(QStringList availableMidiPorts READ availableMidiPorts NOTIFY availableMidiPortsChanged)
    Q_PROPERTY(QString portName READ portName WRITE setPortName NOTIFY portNameChanged)
    Q_PROPERTY(uint8_t channel READ channel WRITE setChannel NOTIFY channelChanged)
    Q_PROPERTY(bool patchEnabled READ patchEnabled WRITE setPatchEnabled NOTIFY patchEnabledChanged)
    Q_PROPERTY(uint8_t patch READ patch WRITE setPatch NOTIFY patchChanged)
    Q_PROPERTY(bool bankEnabled READ bankEnabled WRITE setBankEnabled NOTIFY bankEnabledChanged)
    Q_PROPERTY(uint8_t bankLsb READ bankLsb WRITE setBankLsb NOTIFY bankLsbChanged)
    Q_PROPERTY(uint8_t bankMsb READ bankMsb WRITE setBankMsb NOTIFY bankMsbChanged)
    Q_PROPERTY(bool bankByteOrderSwapped READ bankByteOrderSwapped WRITE setBankByteOrderSwapped NOTIFY bankByteOrderSwappedChanged)

public:
    explicit TrackSettingsModel(QObject * parent = nullptr);

    ~TrackSettingsModel() override;

    Q_INVOKABLE void applyAll();

    Q_INVOKABLE void requestInstrumentData();

    Q_INVOKABLE void requestTestSound(uint8_t velocity);

    Q_INVOKABLE void save();

    QStringList availableMidiPorts() const;

    void setAvailableMidiPorts(QStringList portNames);

    uint32_t trackIndex() const;

    void setTrackIndex(uint32_t trackIndex);

    void setInstrumentData(const Instrument &);

    void reset();

    using InstrumentU = std::unique_ptr<Instrument>;

    InstrumentU toInstrument() const;

    QString portName() const;
    uint8_t channel() const;
    bool patchEnabled() const;
    uint8_t patch() const;
    bool bankEnabled() const;
    uint8_t bankLsb() const;
    uint8_t bankMsb() const;
    bool bankByteOrderSwapped() const;

    void setPortName(const QString & name);
    void setChannel(uint8_t channel);
    void setPatchEnabled(bool enabled);
    void setPatch(uint8_t patch);
    void setBankEnabled(bool enabled);
    void setBankLsb(uint8_t lsb);
    void setBankMsb(uint8_t msb);
    void setBankByteOrderSwapped(bool swapped);

signals:
    void availableMidiPortsChanged();

    void trackIndexChanged();

    void portNameChanged();
    void channelChanged();
    void patchEnabledChanged();
    void patchChanged();
    void bankEnabledChanged();
    void bankLsbChanged();
    void bankMsbChanged();
    void bankByteOrderSwappedChanged();

    void applyAllRequested();
    void applyPatchRequested();
    void instrumentDataRequested();
    void instrumentDataReceived();
    void saveRequested();
    void testSoundRequested(uint8_t velocity);

private:
    void pushApplyDisabled();

    void popApplyDisabled();

    bool m_applyDisabled = false;
    std::vector<bool> m_applyDisabledStack;

    QString m_instrumentPortName;

    QStringList m_availableMidiPorts;

    uint32_t m_trackIndex { 0 };
    QString m_portName;
    uint8_t m_channel { 0 };
    bool m_patchEnabled { false };
    uint8_t m_patch { 0 };
    bool m_bankEnabled { false };
    uint8_t m_bankLsb { 0 };
    uint8_t m_bankMsb { 0 };
    bool m_bankByteOrderSwapped { false };
};

} // namespace noteahead

#endif // TRACK_SETTINGS_MODEL_HPP
