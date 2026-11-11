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

#include "string_voice_v3_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "string_voice_v3_device.hpp"

#include <cmath>

namespace noteahead {

StringVoiceV3DialogModel::StringVoiceV3DialogModel(std::shared_ptr<StringVoiceV3Device> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

StringVoiceV3DialogModel::~StringVoiceV3DialogModel() = default;

DeviceDialogModel::DeviceS StringVoiceV3DialogModel::device() const
{
    return m_device;
}

bool StringVoiceV3DialogModel::setDevice(DeviceS device)
{
    if (const auto stringVoice = std::dynamic_pointer_cast<StringVoiceV3Device>(device)) {
        setDevice(stringVoice);
        return true;
    }
    return false;
}

int StringVoiceV3DialogModel::stringsBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setStringsBalance(int value)
{
    if (m_device) {
        m_device->setStringsBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::voiceBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVoiceBalance(int value)
{
    if (m_device) {
        m_device->setVoiceBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool StringVoiceV3DialogModel::stringsUpper() const
{
    return m_device ? m_device->stringsUpper() : false;
}

void StringVoiceV3DialogModel::setStringsUpper(bool value)
{
    if (m_device) {
        m_device->setStringsUpper(value);
    }
}

bool StringVoiceV3DialogModel::stringsLower() const
{
    return m_device ? m_device->stringsLower() : false;
}

void StringVoiceV3DialogModel::setStringsLower(bool value)
{
    if (m_device) {
        m_device->setStringsLower(value);
    }
}

int StringVoiceV3DialogModel::stringsTone() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsTone() * Constants::uiInternalScaling())) : 0;
}

int StringVoiceV3DialogModel::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void StringVoiceV3DialogModel::setStringsTone(int value)
{
    if (m_device) {
        m_device->setStringsTone(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::stringsAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setStringsAttack(int value)
{
    if (m_device) {
        m_device->setStringsAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::stringsRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setStringsRelease(int value)
{
    if (m_device) {
        m_device->setStringsRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::voiceMale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVoiceMale8(int value)
{
    if (m_device) {
        m_device->setVoiceMale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::voiceMale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVoiceMale4(int value)
{
    if (m_device) {
        m_device->setVoiceMale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::voiceUpperMale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceUpperMale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVoiceUpperMale8(int value)
{
    if (m_device) {
        m_device->setVoiceUpperMale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::voiceFemale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceFemale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVoiceFemale4(int value)
{
    if (m_device) {
        m_device->setVoiceFemale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::voiceAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVoiceAttack(int value)
{
    if (m_device) {
        m_device->setVoiceAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::voiceRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVoiceRelease(int value)
{
    if (m_device) {
        m_device->setVoiceRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::vibratoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoRate() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVibratoRate(int value)
{
    if (m_device) {
        m_device->setVibratoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::vibratoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDepth() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVibratoDepth(int value)
{
    if (m_device) {
        m_device->setVibratoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::vibratoDelay() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDelay() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setVibratoDelay(int value)
{
    if (m_device) {
        m_device->setVibratoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool StringVoiceV3DialogModel::ensembleEnabled() const
{
    return m_device ? m_device->ensembleEnabled() : false;
}

void StringVoiceV3DialogModel::setEnsembleEnabled(bool value)
{
    if (m_device) {
        m_device->setEnsembleEnabled(value);
    }
}

int StringVoiceV3DialogModel::ensembleMode() const
{
    return m_device ? m_device->ensembleMode() : 0;
}

void StringVoiceV3DialogModel::setEnsembleMode(int value)
{
    if (m_device) {
        m_device->setEnsembleMode(value);
    }
}

bool StringVoiceV3DialogModel::vocoderEnabled() const
{
    return m_device ? m_device->vocoderEnabled() : false;
}

void StringVoiceV3DialogModel::setVocoderEnabled(bool value)
{
    if (m_device) {
        m_device->setVocoderEnabled(value);
    }
}

int StringVoiceV3DialogModel::vocoderSidechain() const
{
    return m_device ? m_device->vocoderSidechain() : -1;
}

void StringVoiceV3DialogModel::setVocoderSidechain(int value)
{
    if (m_device) {
        m_device->setVocoderSidechain(value);
    }
}

int StringVoiceV3DialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceV3DialogModel::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceV3DialogModel::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void StringVoiceV3DialogModel::requestSettings()
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

void StringVoiceV3DialogModel::setDevice(std::shared_ptr<StringVoiceV3Device> device)
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
