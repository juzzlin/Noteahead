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

#include "string_voice_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/string_voice_device.hpp"

#include <cmath>

namespace noteahead {

StringVoiceController::StringVoiceController(std::shared_ptr<StringVoiceDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

StringVoiceController::~StringVoiceController() = default;

DeviceController::DeviceS StringVoiceController::device() const
{
    return m_device;
}

bool StringVoiceController::setDevice(DeviceS device)
{
    if (const auto stringVoice = std::dynamic_pointer_cast<StringVoiceDevice>(device)) {
        setDevice(stringVoice);
        return true;
    }
    return false;
}

int StringVoiceController::stringsBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setStringsBalance(int value)
{
    if (m_device) {
        m_device->setStringsBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::voiceBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVoiceBalance(int value)
{
    if (m_device) {
        m_device->setVoiceBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::stringsLevel8() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsLevel8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setStringsLevel8(int value)
{
    if (m_device) {
        m_device->setStringsLevel8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::stringsLevel4() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsLevel4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setStringsLevel4(int value)
{
    if (m_device) {
        m_device->setStringsLevel4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::stringsAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setStringsAttack(int value)
{
    if (m_device) {
        m_device->setStringsAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::stringsRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setStringsRelease(int value)
{
    if (m_device) {
        m_device->setStringsRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::voiceMale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVoiceMale8(int value)
{
    if (m_device) {
        m_device->setVoiceMale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::voiceMale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVoiceMale4(int value)
{
    if (m_device) {
        m_device->setVoiceMale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::voiceFemale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceFemale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVoiceFemale8(int value)
{
    if (m_device) {
        m_device->setVoiceFemale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::voiceFemale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceFemale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVoiceFemale4(int value)
{
    if (m_device) {
        m_device->setVoiceFemale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::voiceAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVoiceAttack(int value)
{
    if (m_device) {
        m_device->setVoiceAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::voiceRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVoiceRelease(int value)
{
    if (m_device) {
        m_device->setVoiceRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::vibratoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoRate() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVibratoRate(int value)
{
    if (m_device) {
        m_device->setVibratoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::vibratoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDepth() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVibratoDepth(int value)
{
    if (m_device) {
        m_device->setVibratoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::vibratoDelay() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDelay() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setVibratoDelay(int value)
{
    if (m_device) {
        m_device->setVibratoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool StringVoiceController::ensembleEnabled() const
{
    return m_device ? m_device->ensembleEnabled() : false;
}

void StringVoiceController::setEnsembleEnabled(bool value)
{
    if (m_device) {
        m_device->setEnsembleEnabled(value);
    }
}

int StringVoiceController::ensembleMode() const
{
    return m_device ? m_device->ensembleMode() : 0;
}

void StringVoiceController::setEnsembleMode(int value)
{
    if (m_device) {
        m_device->setEnsembleMode(value);
    }
}

bool StringVoiceController::vocoderEnabled() const
{
    return m_device ? m_device->vocoderEnabled() : false;
}

void StringVoiceController::setVocoderEnabled(bool value)
{
    if (m_device) {
        m_device->setVocoderEnabled(value);
    }
}

int StringVoiceController::vocoderSidechain() const
{
    return m_device ? m_device->vocoderSidechain() : -1;
}

void StringVoiceController::setVocoderSidechain(int value)
{
    if (m_device) {
        m_device->setVocoderSidechain(value);
    }
}

int StringVoiceController::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceController::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceController::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void StringVoiceController::requestSettings()
{
    emit stringsBalanceChanged();
    emit voiceBalanceChanged();
    emit stringsLevel8Changed();
    emit stringsLevel4Changed();
    emit stringsAttackChanged();
    emit stringsReleaseChanged();
    emit voiceMale8Changed();
    emit voiceMale4Changed();
    emit voiceFemale8Changed();
    emit voiceFemale4Changed();
    emit voiceAttackChanged();
    emit voiceReleaseChanged();
    emit vibratoRateChanged();
    emit vibratoDepthChanged();
    emit vibratoDelayChanged();
    emit ensembleEnabledChanged();
    emit ensembleModeChanged();
    emit vocoderEnabledChanged();
    emit vocoderSidechainChanged();
    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
    emit panSpreadChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

void StringVoiceController::setDevice(std::shared_ptr<StringVoiceDevice> device)
{
    if (m_device != device) {
        if (m_device) {
            disconnect(m_device.get(), nullptr, this, nullptr);
        }
        m_device = std::move(device);
        connectDeviceSignals();
        emit deviceChanged();
        requestSettings();
    }
}

} // namespace noteahead
