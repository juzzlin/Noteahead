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

#include "string_voice_v2_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "string_voice_v2_device.hpp"

#include <cmath>

namespace noteahead {

StringVoiceV2DialogModel::StringVoiceV2DialogModel(std::shared_ptr<StringVoiceV2Device> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

StringVoiceV2DialogModel::~StringVoiceV2DialogModel() = default;

DeviceDialogModel::DeviceS StringVoiceV2DialogModel::device() const
{
    return m_device;
}

bool StringVoiceV2DialogModel::setDevice(DeviceS device)
{
    if (const auto stringVoice = std::dynamic_pointer_cast<StringVoiceV2Device>(device)) {
        setDevice(stringVoice);
        return true;
    }
    return false;
}

int StringVoiceV2DialogModel::stringsBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setStringsBalance(int value)
{
    if (m_device) {
        m_device->setStringsBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::voiceBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVoiceBalance(int value)
{
    if (m_device) {
        m_device->setVoiceBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool StringVoiceV2DialogModel::stringsUpper() const
{
    return m_device ? m_device->stringsUpper() : false;
}

void StringVoiceV2DialogModel::setStringsUpper(bool value)
{
    if (m_device) {
        m_device->setStringsUpper(value);
    }
}

bool StringVoiceV2DialogModel::stringsLower() const
{
    return m_device ? m_device->stringsLower() : false;
}

void StringVoiceV2DialogModel::setStringsLower(bool value)
{
    if (m_device) {
        m_device->setStringsLower(value);
    }
}

int StringVoiceV2DialogModel::stringsTone() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsTone() * Constants::uiInternalScaling())) : 0;
}

int StringVoiceV2DialogModel::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void StringVoiceV2DialogModel::setStringsTone(int value)
{
    if (m_device) {
        m_device->setStringsTone(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::stringsAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setStringsAttack(int value)
{
    if (m_device) {
        m_device->setStringsAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::stringsRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setStringsRelease(int value)
{
    if (m_device) {
        m_device->setStringsRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::voiceMale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVoiceMale8(int value)
{
    if (m_device) {
        m_device->setVoiceMale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::voiceMale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVoiceMale4(int value)
{
    if (m_device) {
        m_device->setVoiceMale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::voiceUpperMale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceUpperMale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVoiceUpperMale8(int value)
{
    if (m_device) {
        m_device->setVoiceUpperMale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::voiceFemale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceFemale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVoiceFemale4(int value)
{
    if (m_device) {
        m_device->setVoiceFemale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::voiceAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVoiceAttack(int value)
{
    if (m_device) {
        m_device->setVoiceAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::voiceRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVoiceRelease(int value)
{
    if (m_device) {
        m_device->setVoiceRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::vibratoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoRate() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVibratoRate(int value)
{
    if (m_device) {
        m_device->setVibratoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::vibratoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDepth() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVibratoDepth(int value)
{
    if (m_device) {
        m_device->setVibratoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::vibratoDelay() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDelay() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setVibratoDelay(int value)
{
    if (m_device) {
        m_device->setVibratoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool StringVoiceV2DialogModel::ensembleEnabled() const
{
    return m_device ? m_device->ensembleEnabled() : false;
}

void StringVoiceV2DialogModel::setEnsembleEnabled(bool value)
{
    if (m_device) {
        m_device->setEnsembleEnabled(value);
    }
}

int StringVoiceV2DialogModel::ensembleMode() const
{
    return m_device ? m_device->ensembleMode() : 0;
}

void StringVoiceV2DialogModel::setEnsembleMode(int value)
{
    if (m_device) {
        m_device->setEnsembleMode(value);
    }
}

bool StringVoiceV2DialogModel::vocoderEnabled() const
{
    return m_device ? m_device->vocoderEnabled() : false;
}

void StringVoiceV2DialogModel::setVocoderEnabled(bool value)
{
    if (m_device) {
        m_device->setVocoderEnabled(value);
    }
}

int StringVoiceV2DialogModel::vocoderSidechain() const
{
    return m_device ? m_device->vocoderSidechain() : -1;
}

void StringVoiceV2DialogModel::setVocoderSidechain(int value)
{
    if (m_device) {
        m_device->setVocoderSidechain(value);
    }
}

int StringVoiceV2DialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV2DialogModel::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV2DialogModel::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void StringVoiceV2DialogModel::requestSettings()
{
    emit stringsBalanceChanged();
    emit voiceBalanceChanged();
    emit stringsUpperChanged();
    emit stringsLowerChanged();
    emit stringsToneChanged();
    emit velocitySensitivityChanged();
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

void StringVoiceV2DialogModel::setDevice(std::shared_ptr<StringVoiceV2Device> device)
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
