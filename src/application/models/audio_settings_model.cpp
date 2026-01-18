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

#include "audio_settings_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../service/audio_service.hpp"
#include "../service/settings_service.hpp"

namespace noteahead {

static const auto TAG = "AudioSettingsModel";

AudioSettingsModel::AudioSettingsModel(AudioServiceS audioService, SettingsServiceS settingsService, QObject * parent)
  : QObject { parent }
  , m_audioService { std::move(audioService) }
  , m_settingsService { std::move(settingsService) }
{
    m_selectedInputDeviceId = m_settingsService->audioInputDeviceId();
    // Ensure the audio service knows about the saved device ID
    m_audioService->setInputDevice(m_selectedInputDeviceId);
    refreshInputDevices();
}

AudioSettingsModel::~AudioSettingsModel() = default;

QVariantList AudioSettingsModel::inputDevices() const
{
    return m_inputDevices;
}

int AudioSettingsModel::selectedInputDeviceId() const
{
    return m_selectedInputDeviceId;
}

void AudioSettingsModel::setSelectedInputDeviceId(int deviceId)
{
    if (m_selectedInputDeviceId != deviceId) {
        juzzlin::L(TAG).info() << "Selecting audio input device ID: " << deviceId;
        m_selectedInputDeviceId = deviceId;
        m_settingsService->setAudioInputDeviceId(deviceId);
        m_audioService->setInputDevice(deviceId);
        emit selectedInputDeviceIdChanged(deviceId);
    }
}

void AudioSettingsModel::refreshInputDevices()
{
    juzzlin::L(TAG).info() << "Refreshing input devices...";
    m_inputDevices = m_audioService->getInputDevices();
    emit inputDevicesChanged();
}

} // namespace noteahead
