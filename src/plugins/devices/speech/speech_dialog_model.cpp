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

#include "speech_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "speech_device.hpp"

#include <cmath>

namespace noteahead {

SpeechDialogModel::SpeechDialogModel(std::shared_ptr<SpeechDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

SpeechDialogModel::~SpeechDialogModel() = default;

DeviceDialogModel::DeviceS SpeechDialogModel::device() const
{
    return m_device;
}

bool SpeechDialogModel::setDevice(DeviceS device)
{
    if (const auto speech = std::dynamic_pointer_cast<SpeechDevice>(device)) {
        setDevice(speech);
        return true;
    }
    return false;
}

void SpeechDialogModel::setDevice(std::shared_ptr<SpeechDevice> device)
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

int SpeechDialogModel::rate() const
{
    return m_device ? static_cast<int>(std::round(m_device->rate() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setRate(int value)
{
    if (m_device) {
        m_device->setRate(static_cast<float>(value) / Constants::uiInternalScaling());
        emit rateChanged();
    }
}

int SpeechDialogModel::portamento() const
{
    return m_device ? static_cast<int>(std::round(m_device->portamento() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setPortamento(int value)
{
    if (m_device) {
        m_device->setPortamento(static_cast<float>(value) / Constants::uiInternalScaling());
        emit portamentoChanged();
    }
}

int SpeechDialogModel::glide() const
{
    return m_device ? static_cast<int>(std::round(m_device->glide() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setGlide(int value)
{
    if (m_device) {
        m_device->setGlide(static_cast<float>(value) / Constants::uiInternalScaling());
        emit glideChanged();
    }
}

int SpeechDialogModel::formantShift() const
{
    return m_device ? static_cast<int>(std::round(m_device->formantShift() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setFormantShift(int value)
{
    if (m_device) {
        m_device->setFormantShift(static_cast<float>(value) / Constants::uiInternalScaling());
        emit formantShiftChanged();
    }
}

int SpeechDialogModel::breathiness() const
{
    return m_device ? static_cast<int>(std::round(m_device->breathiness() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setBreathiness(int value)
{
    if (m_device) {
        m_device->setBreathiness(static_cast<float>(value) / Constants::uiInternalScaling());
        emit breathinessChanged();
    }
}

int SpeechDialogModel::consonantLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->consonantLevel() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setConsonantLevel(int value)
{
    if (m_device) {
        m_device->setConsonantLevel(static_cast<float>(value) / Constants::uiInternalScaling());
        emit consonantLevelChanged();
    }
}

int SpeechDialogModel::sibilance() const
{
    return m_device ? static_cast<int>(std::round(m_device->sibilance() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setSibilance(int value)
{
    if (m_device) {
        m_device->setSibilance(static_cast<float>(value) / Constants::uiInternalScaling());
        emit sibilanceChanged();
        emit voiceTypeChanged();
        emit velocitySensitivityChanged();
    }
}

int SpeechDialogModel::openQuotient() const
{
    return m_device ? static_cast<int>(std::round(m_device->openQuotient() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setOpenQuotient(int value)
{
    if (m_device) {
        m_device->setOpenQuotient(static_cast<float>(value) / Constants::uiInternalScaling());
        emit openQuotientChanged();
    }
}

int SpeechDialogModel::voicePerturbation() const
{
    return m_device ? static_cast<int>(std::round(m_device->voicePerturbation() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setVoicePerturbation(int value)
{
    if (m_device) {
        m_device->setVoicePerturbation(static_cast<float>(value) / Constants::uiInternalScaling());
        emit voicePerturbationChanged();
    }
}

int SpeechDialogModel::voiceEngine() const
{
    return m_device ? m_device->voiceEngine() : 1;
}

void SpeechDialogModel::setVoiceEngine(int value)
{
    if (m_device) {
        m_device->setVoiceEngine(value);
        emit voiceEngineChanged();
    }
}

int SpeechDialogModel::voiceType() const
{
    return m_device ? m_device->voiceType() : 0;
}

void SpeechDialogModel::setVoiceType(int value)
{
    if (m_device) {
        m_device->setVoiceType(value);
        emit voiceTypeChanged();
        emit velocitySensitivityChanged();
    }
}

int SpeechDialogModel::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
        emit velocitySensitivityChanged();
    }
}

int SpeechDialogModel::intonation() const
{
    return m_device ? static_cast<int>(std::round(m_device->intonation() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setIntonation(int value)
{
    if (m_device) {
        m_device->setIntonation(static_cast<float>(value) / Constants::uiInternalScaling());
        emit intonationChanged();
    }
}

int SpeechDialogModel::vibratoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoRate() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setVibratoRate(int value)
{
    if (m_device) {
        m_device->setVibratoRate(static_cast<float>(value) / Constants::uiInternalScaling());
        emit vibratoRateChanged();
    }
}

int SpeechDialogModel::vibratoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDepth() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setVibratoDepth(int value)
{
    if (m_device) {
        m_device->setVibratoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
        emit vibratoDepthChanged();
    }
}

int SpeechDialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
        emit lpfCutoffChanged();
    }
}

int SpeechDialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SpeechDialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
        emit hpfCutoffChanged();
    }
}

int SpeechDialogModel::triggerMode() const
{
    return m_device ? m_device->triggerMode() : 0;
}

void SpeechDialogModel::setTriggerMode(int value)
{
    if (m_device) {
        m_device->setTriggerMode(value);
        emit triggerModeChanged();
        // The phoneme readout breaks per line in Line mode and not otherwise, and the count beneath
        // it says a different thing there, so both follow the mode as well as the phrase.
        emit phraseChanged();
    }
}

int SpeechDialogModel::syncMode() const
{
    return m_device ? m_device->syncMode() : 0;
}

void SpeechDialogModel::setSyncMode(int value)
{
    if (m_device) {
        m_device->setSyncMode(value);
        emit syncModeChanged();
    }
}

int SpeechDialogModel::syncLength() const
{
    return m_device ? m_device->syncLength() : 0;
}

void SpeechDialogModel::setSyncLength(int value)
{
    if (m_device) {
        m_device->setSyncLength(value);
        emit syncLengthChanged();
    }
}

int SpeechDialogModel::syncDivision() const
{
    return m_device ? m_device->syncDivision() : 0;
}

void SpeechDialogModel::setSyncDivision(int value)
{
    if (m_device) {
        m_device->setSyncDivision(value);
        emit syncDivisionChanged();
    }
}

QString SpeechDialogModel::phrase() const
{
    return m_device ? QString::fromStdString(m_device->phrase()) : QString {};
}

void SpeechDialogModel::setPhrase(const QString & phrase)
{
    if (m_device) {
        m_device->setPhrase(phrase.toStdString());
        emit phraseChanged();
    }
}

QString SpeechDialogModel::phrasePhonemes() const
{
    return m_device ? QString::fromStdString(m_device->phrasePhonemes()) : QString {};
}

int SpeechDialogModel::syllableCount() const
{
    return m_device ? static_cast<int>(m_device->syllableCount()) : 0;
}

int SpeechDialogModel::lineCount() const
{
    return m_device ? static_cast<int>(m_device->lineCount()) : 0;
}

void SpeechDialogModel::requestSettings()
{
    emit rateChanged();
    emit portamentoChanged();
    emit glideChanged();
    emit formantShiftChanged();
    emit breathinessChanged();
    emit consonantLevelChanged();
    emit sibilanceChanged();
    emit voiceTypeChanged();
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
    emit phraseChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
