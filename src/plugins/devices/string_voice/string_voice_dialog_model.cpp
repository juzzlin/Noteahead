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

#include "string_voice_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "string_voice_device.hpp"

#include <cmath>

namespace noteahead {

StringVoiceDialogModel::StringVoiceDialogModel(std::shared_ptr<StringVoiceDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

StringVoiceDialogModel::~StringVoiceDialogModel() = default;

DeviceDialogModel::DeviceS StringVoiceDialogModel::device() const
{
    return m_device;
}

bool StringVoiceDialogModel::setDevice(DeviceS device)
{
    if (const auto stringVoice = std::dynamic_pointer_cast<StringVoiceDevice>(device)) {
        setDevice(stringVoice);
        return true;
    }
    return false;
}

int StringVoiceDialogModel::stringsBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setStringsBalance(int value)
{
    if (m_device) {
        m_device->setStringsBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::voiceBalance() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceBalance() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVoiceBalance(int value)
{
    if (m_device) {
        m_device->setVoiceBalance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::stringsLevel8() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsLevel8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setStringsLevel8(int value)
{
    if (m_device) {
        m_device->setStringsLevel8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::stringsLevel4() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsLevel4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setStringsLevel4(int value)
{
    if (m_device) {
        m_device->setStringsLevel4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::stringsAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setStringsAttack(int value)
{
    if (m_device) {
        m_device->setStringsAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::stringsRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringsRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setStringsRelease(int value)
{
    if (m_device) {
        m_device->setStringsRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::voiceMale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVoiceMale8(int value)
{
    if (m_device) {
        m_device->setVoiceMale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::voiceMale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceMale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVoiceMale4(int value)
{
    if (m_device) {
        m_device->setVoiceMale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::voiceUpperMale8() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceUpperMale8() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVoiceUpperMale8(int value)
{
    if (m_device) {
        m_device->setVoiceUpperMale8(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::voiceFemale4() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceFemale4() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVoiceFemale4(int value)
{
    if (m_device) {
        m_device->setVoiceFemale4(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::voiceAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceAttack() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVoiceAttack(int value)
{
    if (m_device) {
        m_device->setVoiceAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::voiceRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceRelease() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVoiceRelease(int value)
{
    if (m_device) {
        m_device->setVoiceRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::vibratoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoRate() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVibratoRate(int value)
{
    if (m_device) {
        m_device->setVibratoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::vibratoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDepth() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVibratoDepth(int value)
{
    if (m_device) {
        m_device->setVibratoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::vibratoDelay() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDelay() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setVibratoDelay(int value)
{
    if (m_device) {
        m_device->setVibratoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool StringVoiceDialogModel::ensembleEnabled() const
{
    return m_device ? m_device->ensembleEnabled() : false;
}

void StringVoiceDialogModel::setEnsembleEnabled(bool value)
{
    if (m_device) {
        m_device->setEnsembleEnabled(value);
    }
}

int StringVoiceDialogModel::ensembleMode() const
{
    return m_device ? m_device->ensembleMode() : 0;
}

void StringVoiceDialogModel::setEnsembleMode(int value)
{
    if (m_device) {
        m_device->setEnsembleMode(value);
    }
}

bool StringVoiceDialogModel::vocoderEnabled() const
{
    return m_device ? m_device->vocoderEnabled() : false;
}

void StringVoiceDialogModel::setVocoderEnabled(bool value)
{
    if (m_device) {
        m_device->setVocoderEnabled(value);
    }
}

int StringVoiceDialogModel::vocoderSidechain() const
{
    return m_device ? m_device->vocoderSidechain() : -1;
}

void StringVoiceDialogModel::setVocoderSidechain(int value)
{
    if (m_device) {
        m_device->setVocoderSidechain(value);
    }
}

int StringVoiceDialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringVoiceDialogModel::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void StringVoiceDialogModel::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void StringVoiceDialogModel::requestSettings()
{
    emit stringsBalanceChanged();
    emit voiceBalanceChanged();
    emit stringsLevel8Changed();
    emit stringsLevel4Changed();
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

void StringVoiceDialogModel::setDevice(std::shared_ptr<StringVoiceDevice> device)
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
