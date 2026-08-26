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

#include "string_voice_v2_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/string_voice_v2_device.hpp"

#include <cmath>

namespace noteahead {

StringVoiceV2Controller::StringVoiceV2Controller(std::shared_ptr<StringVoiceV2Device> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

StringVoiceV2Controller::~StringVoiceV2Controller() = default;

DeviceController::DeviceS StringVoiceV2Controller::device() const
{
    return m_device;
}

bool StringVoiceV2Controller::setDevice(DeviceS device)
{
    if (const auto stringVoice = std::dynamic_pointer_cast<StringVoiceV2Device>(device)) {
        setDevice(stringVoice);
        return true;
    }
    return false;
}

int StringVoiceV2Controller::stringsBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setStringsBalance(int value)
{
    if (m_device) {
        m_device->setStringsBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::voiceBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVoiceBalance(int value)
{
    if (m_device) {
        m_device->setVoiceBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool StringVoiceV2Controller::stringsUpper() const
{
    return m_device ? m_device->stringsUpper() : false;
}

void StringVoiceV2Controller::setStringsUpper(bool value)
{
    if (m_device) {
        m_device->setStringsUpper(value);
    }
}

bool StringVoiceV2Controller::stringsLower() const
{
    return m_device ? m_device->stringsLower() : false;
}

void StringVoiceV2Controller::setStringsLower(bool value)
{
    if (m_device) {
        m_device->setStringsLower(value);
    }
}

int StringVoiceV2Controller::stringsTone() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsTone() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setStringsTone(int value)
{
    if (m_device) {
        m_device->setStringsTone(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::stringsAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setStringsAttack(int value)
{
    if (m_device) {
        m_device->setStringsAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::stringsRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setStringsRelease(int value)
{
    if (m_device) {
        m_device->setStringsRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::voiceMale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVoiceMale8(int value)
{
    if (m_device) {
        m_device->setVoiceMale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::voiceMale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVoiceMale4(int value)
{
    if (m_device) {
        m_device->setVoiceMale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::voiceUpperMale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceUpperMale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVoiceUpperMale8(int value)
{
    if (m_device) {
        m_device->setVoiceUpperMale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::voiceFemale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceFemale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVoiceFemale4(int value)
{
    if (m_device) {
        m_device->setVoiceFemale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::voiceAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVoiceAttack(int value)
{
    if (m_device) {
        m_device->setVoiceAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::voiceRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVoiceRelease(int value)
{
    if (m_device) {
        m_device->setVoiceRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::vibratoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoRate() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVibratoRate(int value)
{
    if (m_device) {
        m_device->setVibratoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::vibratoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDepth() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVibratoDepth(int value)
{
    if (m_device) {
        m_device->setVibratoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::vibratoDelay() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDelay() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setVibratoDelay(int value)
{
    if (m_device) {
        m_device->setVibratoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool StringVoiceV2Controller::ensembleEnabled() const
{
    return m_device ? m_device->ensembleEnabled() : false;
}

void StringVoiceV2Controller::setEnsembleEnabled(bool value)
{
    if (m_device) {
        m_device->setEnsembleEnabled(value);
    }
}

int StringVoiceV2Controller::ensembleMode() const
{
    return m_device ? m_device->ensembleMode() : 0;
}

void StringVoiceV2Controller::setEnsembleMode(int value)
{
    if (m_device) {
        m_device->setEnsembleMode(value);
    }
}

bool StringVoiceV2Controller::vocoderEnabled() const
{
    return m_device ? m_device->vocoderEnabled() : false;
}

void StringVoiceV2Controller::setVocoderEnabled(bool value)
{
    if (m_device) {
        m_device->setVocoderEnabled(value);
    }
}

int StringVoiceV2Controller::vocoderSidechain() const
{
    return m_device ? m_device->vocoderSidechain() : -1;
}

void StringVoiceV2Controller::setVocoderSidechain(int value)
{
    if (m_device) {
        m_device->setVocoderSidechain(value);
    }
}

int StringVoiceV2Controller::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2Controller::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2Controller::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void StringVoiceV2Controller::requestSettings()
{
    emit stringsBalanceChanged();
    emit voiceBalanceChanged();
    emit stringsUpperChanged();
    emit stringsLowerChanged();
    emit stringsToneChanged();
    emit stringsAttackChanged();
    emit stringsReleaseChanged();
    emit voiceMale8Changed();
    emit voiceMale4Changed();
    emit voiceUpperMale8Changed();
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

void StringVoiceV2Controller::setDevice(std::shared_ptr<StringVoiceV2Device> device)
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
