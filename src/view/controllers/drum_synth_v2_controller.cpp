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

#include "drum_synth_v2_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/drum_synth_v2_constants.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"

#include <cmath>

namespace noteahead {

DrumSynthV2Controller::DrumSynthV2Controller(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DrumPadController { std::move(deviceService), parent }
{
}

bool DrumSynthV2Controller::acceptsDevice(const DeviceS & device) const
{
    return std::dynamic_pointer_cast<DrumSynthV2Device>(device) != nullptr;
}

QStringList DrumSynthV2Controller::factoryPresetNames() const
{
    return { "Init" };
}

void DrumSynthV2Controller::loadFactoryPreset(int index)
{
    if (index == 0) {
        reset();
    }
}

int DrumSynthV2Controller::voiceTune() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyTune().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setVoiceTune(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyTune().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceDecay() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyDecay().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setVoiceDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceAttack() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAttack().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthV2Controller::setVoiceAttack(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAttack().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::kickAttack() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynthV2::voiceId(static_cast<int>(DrumSynthV2::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyAttack().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setKickAttack(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), Constants::NahdXml::xmlKeyAttack().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::kickClickTune() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynthV2::voiceId(static_cast<int>(DrumSynthV2::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyClickTune().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setKickClickTune(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), Constants::NahdXml::xmlKeyClickTune().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::kickPitchDepth() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynthV2::voiceId(static_cast<int>(DrumSynthV2::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyPitchDepth().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setKickPitchDepth(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), Constants::NahdXml::xmlKeyPitchDepth().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::kickPitchDecay() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynthV2::voiceId(static_cast<int>(DrumSynthV2::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyPitchDecay().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setKickPitchDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), Constants::NahdXml::xmlKeyPitchDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::rimClick() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynthV2::voiceId(static_cast<int>(DrumSynthV2::VoiceIndex::Rim)) + "_" + Constants::NahdXml::xmlKeyClick().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setRimClick(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Rim), Constants::NahdXml::xmlKeyClick().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool DrumSynthV2Controller::isRim() const
{
    return m_selectedVoice == static_cast<int>(DrumSynthV2::VoiceIndex::Rim);
}

int DrumSynthV2Controller::snareSnappy() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynthV2::voiceId(static_cast<int>(DrumSynthV2::VoiceIndex::Snare)) + "_" + Constants::NahdXml::xmlKeySnappy().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setSnareSnappy(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Snare), Constants::NahdXml::xmlKeySnappy().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::snareTone() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynthV2::voiceId(static_cast<int>(DrumSynthV2::VoiceIndex::Snare)) + "_" + Constants::NahdXml::xmlKeyTone().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setSnareTone(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Snare), Constants::NahdXml::xmlKeyTone().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::tomPitchDepth() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyPitchDepth().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setTomPitchDepth(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyPitchDepth().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::tomPitchDecay() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyPitchDecay().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setTomPitchDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyPitchDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceResonance() const
{
    if (!m_device) {
        return 300;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyResonance().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 300;
}

void DrumSynthV2Controller::setVoiceResonance(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyResonance().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceWobble() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyWobble().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setVoiceWobble(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyWobble().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool DrumSynthV2Controller::isKick() const
{
    return m_selectedVoice == static_cast<int>(DrumSynthV2::VoiceIndex::Kick);
}

bool DrumSynthV2Controller::isSnare() const
{
    return m_selectedVoice == static_cast<int>(DrumSynthV2::VoiceIndex::Snare);
}

bool DrumSynthV2Controller::isTom() const
{
    return m_selectedVoice >= static_cast<int>(DrumSynthV2::VoiceIndex::LowTom) && m_selectedVoice <= static_cast<int>(DrumSynthV2::VoiceIndex::HighTom);
}

bool DrumSynthV2Controller::isCymbal() const
{
    return m_selectedVoice >= static_cast<int>(DrumSynthV2::VoiceIndex::Crash) && m_selectedVoice <= static_cast<int>(DrumSynthV2::VoiceIndex::ReverseCrash);
}

bool DrumSynthV2Controller::hasResonance() const
{
    return (m_selectedVoice == static_cast<int>(DrumSynthV2::VoiceIndex::ClosedHiHat) || m_selectedVoice == static_cast<int>(DrumSynthV2::VoiceIndex::OpenHiHat)) || isCymbal();
}

bool DrumSynthV2Controller::hasAttack() const
{
    return isKick() || (m_selectedVoice == static_cast<int>(DrumSynthV2::VoiceIndex::Crash) || m_selectedVoice == static_cast<int>(DrumSynthV2::VoiceIndex::ReverseCrash));
}

bool DrumSynthV2Controller::hasWobble() const
{
    return m_selectedVoice == static_cast<int>(DrumSynthV2::VoiceIndex::Crash) || m_selectedVoice == static_cast<int>(DrumSynthV2::VoiceIndex::ReverseCrash);
}

void DrumSynthV2Controller::requestSettings()
{
    DrumPadController::requestSettings();

    emit voiceTuneChanged();
    emit voiceDecayChanged();
    emit voiceAttackChanged();
    emit kickAttackChanged();
    emit kickClickTuneChanged();
    emit kickPitchDepthChanged();
    emit kickPitchDecayChanged();
    emit rimClickChanged();
    emit snareSnappyChanged();
    emit snareToneChanged();
    emit tomPitchDepthChanged();
    emit tomPitchDecayChanged();
    emit voiceResonanceChanged();
    emit voiceWobbleChanged();
}

} // namespace noteahead
