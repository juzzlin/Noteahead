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

#ifndef MIDI_SETTINGS_MODEL_HPP
#define MIDI_SETTINGS_MODEL_HPP

#include <memory>

#include <QObject>

namespace noteahead {

class EditorService;
class Instrument;
class SettingsService;

class MidiSettingsModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList midiInPorts READ midiInPorts NOTIFY midiInPortsChanged)
    Q_PROPERTY(QString controllerPort READ controllerPort WRITE setControllerPort NOTIFY controllerPortChanged)
    Q_PROPERTY(QString debugData READ debugData WRITE setDebugData NOTIFY debugDataChanged)

public:
    using SettingsServiceS = std::shared_ptr<SettingsService>;
    explicit MidiSettingsModel(SettingsServiceS settingsService, QObject * parent = nullptr);
    ~MidiSettingsModel() override;

    QStringList midiInPorts() const;
    void setMidiInPorts(QStringList portNames);

    QString controllerPort() const;
    void setControllerPort(const QString & name);

    QString debugData() const;
    void setDebugData(const QString & data);

signals:
    void midiInPortsChanged();
    void controllerPortChanged(const QString & portName);
    void debugDataChanged();

private:
    SettingsServiceS m_settingsService;

    QStringList m_midiInPorts;
    QString m_controllerPort;

    QString m_debugData;

    bool m_settingPorts = false;
};

} // namespace noteahead

#endif // MIDI_SETTINGS_MODEL_HPP
