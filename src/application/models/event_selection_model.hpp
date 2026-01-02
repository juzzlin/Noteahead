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

    Q_PROPERTY(bool patchEnabled READ patchEnabled WRITE setPatchEnabled NOTIFY patchEnabledChanged)
    Q_PROPERTY(quint8 patch READ patch WRITE setPatch NOTIFY patchChanged)

    Q_PROPERTY(bool bankEnabled READ bankEnabled WRITE setBankEnabled NOTIFY bankEnabledChanged)
    Q_PROPERTY(quint8 bankLsb READ bankLsb WRITE setBankLsb NOTIFY bankLsbChanged)
    Q_PROPERTY(quint8 bankMsb READ bankMsb WRITE setBankMsb NOTIFY bankMsbChanged)
    Q_PROPERTY(bool bankByteOrderSwapped READ bankByteOrderSwapped WRITE setBankByteOrderSwapped NOTIFY bankByteOrderSwappedChanged)

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

    quint8 bankLsb() const;
    void setBankLsb(quint8 lsb);

    quint8 bankMsb() const;
    void setBankMsb(quint8 msb);

    quint8 channel() const;
    void setChannel(quint8 channel);

    quint8 patch() const;
    void setPatch(quint8 patch);

signals:
    void bankByteOrderSwappedChanged();
    void bankEnabledChanged();
    void bankLsbChanged();
    void bankMsbChanged();

    void dataReceived();
    void dataRequested();

    void patchChanged();
    void patchEnabledChanged();

    void saveRequested();

private:
    bool m_bankByteOrderSwapped { false };
    bool m_bankEnabled { false };

    bool m_patchEnabled { false };

    quint8 m_bankLsb { 0 };
    quint8 m_bankMsb { 0 };

    quint8 m_channel { 0 };

    quint8 m_patch { 0 };
};

} // namespace noteahead

#endif // EVENT_SELECTION_MODEL_HPP
