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

    Q_PROPERTY(QStringList availableMidiPorts READ availableMidiPorts NOTIFY availableMidiPortsChanged)
    Q_PROPERTY(QString controllerPort READ controllerPort WRITE setControllerPort NOTIFY controllerPortChanged)

public:
    using SettingsServiceS = std::shared_ptr<SettingsService>;
    explicit MidiSettingsModel(SettingsServiceS settingsService, QObject * parent = nullptr);
    ~MidiSettingsModel() override;

    QStringList availableMidiPorts() const;
    void setAvailableMidiPorts(QStringList portNames);

    QString controllerPort() const;
    void setControllerPort(const QString & name);

signals:
    void availableMidiPortsChanged();
    void controllerPortChanged();

private:
    SettingsServiceS m_settingsService;

    QString m_controllerPort;
    QStringList m_availableMidiPorts;
};

} // namespace noteahead

#endif // MIDI_SETTINGS_MODEL_HPP
