// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef AUDIO_SETTINGS_MODEL_HPP
#define AUDIO_SETTINGS_MODEL_HPP

#include <memory>

#include <QObject>
#include <QVariantList>

namespace noteahead {

class AudioService;
class SettingsService;

class AudioSettingsModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList inputDevices READ inputDevices NOTIFY inputDevicesChanged)
    Q_PROPERTY(int selectedInputDeviceId READ selectedInputDeviceId WRITE setSelectedInputDeviceId NOTIFY selectedInputDeviceIdChanged)

public:
    using AudioServiceS = std::shared_ptr<AudioService>;
    using SettingsServiceS = std::shared_ptr<SettingsService>;

    explicit AudioSettingsModel(AudioServiceS audioService, SettingsServiceS settingsService, QObject * parent = nullptr);
    ~AudioSettingsModel() override;

    QVariantList inputDevices() const;
    
    int selectedInputDeviceId() const;
    void setSelectedInputDeviceId(int deviceId);
    
    Q_INVOKABLE void refreshInputDevices();

signals:
    void inputDevicesChanged();
    void selectedInputDeviceIdChanged(int deviceId);

private:
    AudioServiceS m_audioService;
    SettingsServiceS m_settingsService;

    QVariantList m_inputDevices;
    int m_selectedInputDeviceId = 0;
};

} // namespace noteahead

#endif // AUDIO_SETTINGS_MODEL_HPP
