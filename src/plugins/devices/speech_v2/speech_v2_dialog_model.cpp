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

#include "speech_v2_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "speech_v2_device.hpp"

#include <cmath>

namespace noteahead {

SpeechV2DialogModel::SpeechV2DialogModel(std::shared_ptr<SpeechV2Device> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

SpeechV2DialogModel::~SpeechV2DialogModel() = default;

DeviceDialogModel::DeviceS SpeechV2DialogModel::device() const
{
    return m_device;
}

bool SpeechV2DialogModel::setDevice(DeviceS device)
{
    if (const auto speech = std::dynamic_pointer_cast<SpeechV2Device>(device)) {
        setDevice(speech);
        return true;
    }
    return false;
}

void SpeechV2DialogModel::setDevice(std::shared_ptr<SpeechV2Device> device)
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

int SpeechV2DialogModel::rate() const
{
    return m_device ? static_cast<int>(std::round(m_device->rate() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setRate(int value)
{
    if (m_device) {
        m_device->setRate(static_cast<float>(value) / Constants::uiInternalScaling());
        emit rateChanged();
    }
}

int SpeechV2DialogModel::portamento() const
{
    return m_device ? static_cast<int>(std::round(m_device->portamento() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setPortamento(int value)
{
    if (m_device) {
        m_device->setPortamento(static_cast<float>(value) / Constants::uiInternalScaling());
        emit portamentoChanged();
    }
}

int SpeechV2DialogModel::glide() const
{
    return m_device ? static_cast<int>(std::round(m_device->glide() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setGlide(int value)
{
    if (m_device) {
        m_device->setGlide(static_cast<float>(value) / Constants::uiInternalScaling());
        emit glideChanged();
    }
}

int SpeechV2DialogModel::formantShift() const
{
    return m_device ? static_cast<int>(std::round(m_device->formantShift() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setFormantShift(int value)
{
    if (m_device) {
        m_device->setFormantShift(static_cast<float>(value) / Constants::uiInternalScaling());
        emit formantShiftChanged();
    }
}

int SpeechV2DialogModel::breathiness() const
{
    return m_device ? static_cast<int>(std::round(m_device->breathiness() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setBreathiness(int value)
{
    if (m_device) {
        m_device->setBreathiness(static_cast<float>(value) / Constants::uiInternalScaling());
        emit breathinessChanged();
    }
}

int SpeechV2DialogModel::consonantLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->consonantLevel() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setConsonantLevel(int value)
{
    if (m_device) {
        m_device->setConsonantLevel(static_cast<float>(value) / Constants::uiInternalScaling());
        emit consonantLevelChanged();
    }
}

int SpeechV2DialogModel::sibilance() const
{
    return m_device ? static_cast<int>(std::round(m_device->sibilance() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setSibilance(int value)
{
    if (m_device) {
        m_device->setSibilance(static_cast<float>(value) / Constants::uiInternalScaling());
        emit sibilanceChanged();
    }
}

int SpeechV2DialogModel::openQuotient() const
{
    return m_device ? static_cast<int>(std::round(m_device->openQuotient() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setOpenQuotient(int value)
{
    if (m_device) {
        m_device->setOpenQuotient(static_cast<float>(value) / Constants::uiInternalScaling());
        emit openQuotientChanged();
    }
}

int SpeechV2DialogModel::voicePerturbation() const
{
    return m_device ? static_cast<int>(std::round(m_device->voicePerturbation() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setVoicePerturbation(int value)
{
    if (m_device) {
        m_device->setVoicePerturbation(static_cast<float>(value) / Constants::uiInternalScaling());
        emit voicePerturbationChanged();
    }
}

int SpeechV2DialogModel::voiceType() const
{
    return m_device ? m_device->voiceType() : 0;
}

void SpeechV2DialogModel::setVoiceType(int value)
{
    if (m_device) {
        m_device->setVoiceType(value);
        emit voiceTypeChanged();
    }
}

int SpeechV2DialogModel::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
        emit velocitySensitivityChanged();
    }
}

int SpeechV2DialogModel::intonation() const
{
    return m_device ? static_cast<int>(std::round(m_device->intonation() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setIntonation(int value)
{
    if (m_device) {
        m_device->setIntonation(static_cast<float>(value) / Constants::uiInternalScaling());
        emit intonationChanged();
    }
}

int SpeechV2DialogModel::vibratoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoRate() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setVibratoRate(int value)
{
    if (m_device) {
        m_device->setVibratoRate(static_cast<float>(value) / Constants::uiInternalScaling());
        emit vibratoRateChanged();
    }
}

int SpeechV2DialogModel::vibratoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDepth() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setVibratoDepth(int value)
{
    if (m_device) {
        m_device->setVibratoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
        emit vibratoDepthChanged();
    }
}

int SpeechV2DialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
        emit lpfCutoffChanged();
    }
}

int SpeechV2DialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SpeechV2DialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
        emit hpfCutoffChanged();
    }
}

int SpeechV2DialogModel::triggerMode() const
{
    return m_device ? m_device->triggerMode() : 0;
}

void SpeechV2DialogModel::setTriggerMode(int value)
{
    if (m_device) {
        m_device->setTriggerMode(value);
        emit triggerModeChanged();
        emit phraseChanged();
    }
}

int SpeechV2DialogModel::syncMode() const
{
    return m_device ? m_device->syncMode() : 0;
}

void SpeechV2DialogModel::setSyncMode(int value)
{
    if (m_device) {
        m_device->setSyncMode(value);
        emit syncModeChanged();
    }
}

int SpeechV2DialogModel::syncLength() const
{
    return m_device ? m_device->syncLength() : 0;
}

void SpeechV2DialogModel::setSyncLength(int value)
{
    if (m_device) {
        m_device->setSyncLength(value);
        emit syncLengthChanged();
    }
}

int SpeechV2DialogModel::syncDivision() const
{
    return m_device ? m_device->syncDivision() : 0;
}

void SpeechV2DialogModel::setSyncDivision(int value)
{
    if (m_device) {
        m_device->setSyncDivision(value);
        emit syncDivisionChanged();
    }
}

int SpeechV2DialogModel::pitchBendRange() const
{
    return m_device ? m_device->pitchBendRange() : 0;
}

void SpeechV2DialogModel::setPitchBendRange(int value)
{
    if (m_device) {
        m_device->setPitchBendRange(value);
        emit pitchBendRangeChanged();
    }
}

QString SpeechV2DialogModel::phrase() const
{
    return m_device ? QString::fromStdString(m_device->phrase()) : QString {};
}

void SpeechV2DialogModel::setPhrase(const QString & phrase)
{
    if (m_device) {
        m_device->setPhrase(phrase.toStdString());
        emit phraseChanged();
    }
}

QString SpeechV2DialogModel::phrasePhonemes() const
{
    return m_device ? QString::fromStdString(m_device->phrasePhonemes()) : QString {};
}

int SpeechV2DialogModel::syllableCount() const
{
    return m_device ? static_cast<int>(m_device->syllableCount()) : 0;
}

int SpeechV2DialogModel::lineCount() const
{
    return m_device ? static_cast<int>(m_device->lineCount()) : 0;
}

void SpeechV2DialogModel::requestSettings()
{
    emit rateChanged();
    emit portamentoChanged();
    emit glideChanged();
    emit formantShiftChanged();
    emit breathinessChanged();
    emit consonantLevelChanged();
    emit sibilanceChanged();
    emit voiceTypeChanged();
    emit openQuotientChanged();
    emit voicePerturbationChanged();
    emit velocitySensitivityChanged();
    emit intonationChanged();
    emit vibratoRateChanged();
    emit vibratoDepthChanged();
    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
    emit triggerModeChanged();
    emit syncModeChanged();
    emit syncLengthChanged();
    emit syncDivisionChanged();
    emit pitchBendRangeChanged();
    emit phraseChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
