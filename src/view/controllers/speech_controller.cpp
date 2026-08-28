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

#include "speech_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/speech_device.hpp"

#include <cmath>

namespace noteahead {

SpeechController::SpeechController(std::shared_ptr<SpeechDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

SpeechController::~SpeechController() = default;

DeviceController::DeviceS SpeechController::device() const
{
    return m_device;
}

bool SpeechController::setDevice(DeviceS device)
{
    if (const auto speech = std::dynamic_pointer_cast<SpeechDevice>(device)) {
        setDevice(speech);
        return true;
    }
    return false;
}

void SpeechController::setDevice(std::shared_ptr<SpeechDevice> device)
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

int SpeechController::rate() const
{
    return m_device ? static_cast<int>(std::round(m_device->rate() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setRate(int value)
{
    if (m_device) {
        m_device->setRate(static_cast<float>(value) / Constants::uiInternalScaling());
        emit rateChanged();
    }
}

int SpeechController::glide() const
{
    return m_device ? static_cast<int>(std::round(m_device->glide() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setGlide(int value)
{
    if (m_device) {
        m_device->setGlide(static_cast<float>(value) / Constants::uiInternalScaling());
        emit glideChanged();
    }
}

int SpeechController::formantShift() const
{
    return m_device ? static_cast<int>(std::round(m_device->formantShift() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setFormantShift(int value)
{
    if (m_device) {
        m_device->setFormantShift(static_cast<float>(value) / Constants::uiInternalScaling());
        emit formantShiftChanged();
    }
}

int SpeechController::breathiness() const
{
    return m_device ? static_cast<int>(std::round(m_device->breathiness() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setBreathiness(int value)
{
    if (m_device) {
        m_device->setBreathiness(static_cast<float>(value) / Constants::uiInternalScaling());
        emit breathinessChanged();
    }
}

int SpeechController::consonantLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->consonantLevel() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setConsonantLevel(int value)
{
    if (m_device) {
        m_device->setConsonantLevel(static_cast<float>(value) / Constants::uiInternalScaling());
        emit consonantLevelChanged();
    }
}

int SpeechController::sibilance() const
{
    return m_device ? static_cast<int>(std::round(m_device->sibilance() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setSibilance(int value)
{
    if (m_device) {
        m_device->setSibilance(static_cast<float>(value) / Constants::uiInternalScaling());
        emit sibilanceChanged();
        emit voiceTypeChanged();
        emit velocitySensitivityChanged();
    }
}

int SpeechController::voiceType() const
{
    return m_device ? m_device->voiceType() : 0;
}

void SpeechController::setVoiceType(int value)
{
    if (m_device) {
        m_device->setVoiceType(value);
        emit voiceTypeChanged();
        emit velocitySensitivityChanged();
    }
}

int SpeechController::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
        emit velocitySensitivityChanged();
    }
}

int SpeechController::intonation() const
{
    return m_device ? static_cast<int>(std::round(m_device->intonation() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setIntonation(int value)
{
    if (m_device) {
        m_device->setIntonation(static_cast<float>(value) / Constants::uiInternalScaling());
        emit intonationChanged();
    }
}

int SpeechController::vibratoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoRate() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setVibratoRate(int value)
{
    if (m_device) {
        m_device->setVibratoRate(static_cast<float>(value) / Constants::uiInternalScaling());
        emit vibratoRateChanged();
    }
}

int SpeechController::vibratoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vibratoDepth() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setVibratoDepth(int value)
{
    if (m_device) {
        m_device->setVibratoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
        emit vibratoDepthChanged();
    }
}

int SpeechController::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
        emit lpfCutoffChanged();
    }
}

int SpeechController::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SpeechController::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
        emit hpfCutoffChanged();
    }
}

int SpeechController::triggerMode() const
{
    return m_device ? m_device->triggerMode() : 0;
}

void SpeechController::setTriggerMode(int value)
{
    if (m_device) {
        m_device->setTriggerMode(value);
        emit triggerModeChanged();
    }
}

int SpeechController::syncMode() const
{
    return m_device ? m_device->syncMode() : 0;
}

void SpeechController::setSyncMode(int value)
{
    if (m_device) {
        m_device->setSyncMode(value);
        emit syncModeChanged();
    }
}

int SpeechController::syncLength() const
{
    return m_device ? m_device->syncLength() : 0;
}

void SpeechController::setSyncLength(int value)
{
    if (m_device) {
        m_device->setSyncLength(value);
        emit syncLengthChanged();
    }
}

int SpeechController::syncDivision() const
{
    return m_device ? m_device->syncDivision() : 0;
}

void SpeechController::setSyncDivision(int value)
{
    if (m_device) {
        m_device->setSyncDivision(value);
        emit syncDivisionChanged();
    }
}

QString SpeechController::phrase() const
{
    return m_device ? QString::fromStdString(m_device->phrase()) : QString {};
}

void SpeechController::setPhrase(const QString & phrase)
{
    if (m_device) {
        m_device->setPhrase(phrase.toStdString());
        emit phraseChanged();
    }
}

QString SpeechController::phrasePhonemes() const
{
    return m_device ? QString::fromStdString(m_device->phrasePhonemes()) : QString {};
}

int SpeechController::syllableCount() const
{
    return m_device ? static_cast<int>(m_device->syllableCount()) : 0;
}

void SpeechController::requestSettings()
{
    emit rateChanged();
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
