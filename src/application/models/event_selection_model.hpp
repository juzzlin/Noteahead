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

#ifndef EVENT_SELECTION_MODEL_HPP
#define EVENT_SELECTION_MODEL_HPP

#include <QObject>

#include <memory>

namespace noteahead {

class EditorService;
class InstrumentSettings;

class EventSelectionModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(uint8_t cutoff READ cutoff WRITE setCutoff NOTIFY cutoffChanged)

    Q_PROPERTY(bool cutoffEnabled READ cutoffEnabled WRITE setCutoffEnabled NOTIFY cutoffEnabledChanged)

    Q_PROPERTY(bool patchEnabled READ patchEnabled WRITE setPatchEnabled NOTIFY patchEnabledChanged)

    Q_PROPERTY(uint8_t patch READ patch WRITE setPatch NOTIFY patchChanged)

    Q_PROPERTY(bool bankEnabled READ bankEnabled WRITE setBankEnabled NOTIFY bankEnabledChanged)

    Q_PROPERTY(uint8_t bankLsb READ bankLsb WRITE setBankLsb NOTIFY bankLsbChanged)

    Q_PROPERTY(uint8_t bankMsb READ bankMsb WRITE setBankMsb NOTIFY bankMsbChanged)

    Q_PROPERTY(bool bankByteOrderSwapped READ bankByteOrderSwapped WRITE setBankByteOrderSwapped NOTIFY bankByteOrderSwappedChanged)

    Q_PROPERTY(uint8_t pan READ pan WRITE setPan NOTIFY panChanged)

    Q_PROPERTY(bool panEnabled READ panEnabled WRITE setPanEnabled NOTIFY panEnabledChanged)

    Q_PROPERTY(uint8_t volume READ volume WRITE setVolume NOTIFY volumeChanged)

    Q_PROPERTY(bool volumeEnabled READ volumeEnabled WRITE setVolumeEnabled NOTIFY volumeEnabledChanged)

public:
    explicit EventSelectionModel(QObject * parent = nullptr);

    ~EventSelectionModel() override;

    Q_INVOKABLE void requestData();

    Q_INVOKABLE void save();

    void reset();

    using InstrumentSettingsU = std::unique_ptr<InstrumentSettings>;
    InstrumentSettingsU toInstrumentSettings() const;
    void fromInstrumentSettings(const InstrumentSettings & instrumentSettings);

    QString portName() const;
    void setPortName(const QString & name);

    bool bankByteOrderSwapped() const;
    void setBankByteOrderSwapped(bool swapped);

    bool bankEnabled() const;
    void setBankEnabled(bool enabled);

    bool patchEnabled() const;
    void setPatchEnabled(bool enabled);

    uint8_t bankLsb() const;
    void setBankLsb(uint8_t lsb);

    uint8_t bankMsb() const;
    void setBankMsb(uint8_t msb);

    uint8_t channel() const;
    void setChannel(uint8_t channel);

    uint8_t cutoff() const;
    void setCutoff(uint8_t cutoff);

    bool cutoffEnabled() const;
    void setCutoffEnabled(bool enabled);

    uint8_t pan() const;
    void setPan(uint8_t volume);

    bool panEnabled() const;
    void setPanEnabled(bool enabled);

    uint8_t patch() const;
    void setPatch(uint8_t patch);

    uint8_t volume() const;
    void setVolume(uint8_t volume);

    bool volumeEnabled() const;
    void setVolumeEnabled(bool enabled);

signals:
    void bankByteOrderSwappedChanged();

    void bankEnabledChanged();

    void bankLsbChanged();

    void bankMsbChanged();

    void cutoffChanged();

    void cutoffEnabledChanged();

    void dataReceived();

    void dataRequested();

    void panChanged();

    void panEnabledChanged();

    void patchChanged();

    void patchEnabledChanged();

    void saveRequested();

    void volumeChanged();

    void volumeEnabledChanged();

private:
    bool m_bankByteOrderSwapped { false };

    bool m_bankEnabled { false };

    bool m_cutoffEnabled { false };

    bool m_panEnabled { false };

    bool m_patchEnabled { false };

    bool m_volumeEnabled { false };

    uint8_t m_bankLsb { 0 };

    uint8_t m_bankMsb { 0 };

    uint8_t m_channel { 0 };

    const uint8_t m_defaultCutoff { 127 };

    uint8_t m_cutoff { m_defaultCutoff };

    uint8_t m_patch { 0 };

    const uint8_t m_defaultPan { 64 };

    uint8_t m_pan { m_defaultPan };

    const uint8_t m_defaultVolume { 127 };

    uint8_t m_volume { m_defaultVolume };
};

} // namespace noteahead

#endif // EVENT_SELECTION_MODEL_HPP
