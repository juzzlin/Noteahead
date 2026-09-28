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

#include <QVariant>

#include "../../application/service/device_service.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"

#include <cmath>

namespace noteahead {

DrumSynthV2Controller::DrumSynthV2Controller(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DeviceController { parent }
  , m_deviceService { std::move(deviceService) }
{
}

DeviceController::DeviceS DrumSynthV2Controller::device() const
{
    return m_device;
}

bool DrumSynthV2Controller::setDevice(DeviceS device)
{
    if (const auto drumSynth = std::dynamic_pointer_cast<DrumSynthV2Device>(device)) {
        if (m_device) {
            disconnect(m_device.get(), nullptr, this, nullptr);
        }
        m_device = drumSynth;
        connectDeviceSignals();
        requestSettings();
        return true;
    }
    return false;
}

void DrumSynthV2Controller::setDevice(const QString & deviceName)
{
    setDevice(m_deviceService->device(deviceName.toStdString()));
}

int DrumSynthV2Controller::lpfSlope() const
{
    return m_device ? m_device->lpfSlope() : 0;
}

void DrumSynthV2Controller::setLpfSlope(int value)
{
    if (m_device) {
        m_device->setLpfSlope(value);
        emit lpfSlopeChanged();
    }
}

int DrumSynthV2Controller::hpfSlope() const
{
    return m_device ? m_device->hpfSlope() : 0;
}

void DrumSynthV2Controller::setHpfSlope(int value)
{
    if (m_device) {
        m_device->setHpfSlope(value);
        emit hpfSlopeChanged();
    }
}

int DrumSynthV2Controller::selectedVoice() const
{
    return m_selectedVoice;
}

void DrumSynthV2Controller::setSelectedVoice(int index)
{
    if (m_selectedVoice != index) {
        m_selectedVoice = index;
        emit selectedVoiceChanged();
        requestSettings();
    }
}

int DrumSynthV2Controller::voiceLevel() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyLevel().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthV2Controller::setVoiceLevel(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyLevel().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voicePan() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyPan().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setVoicePan(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyPan().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceLpfCutoff() const
{
    if (!m_device) {
        return 1000;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyCutoff().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 1000;
}

void DrumSynthV2Controller::setVoiceLpfCutoff(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyCutoff().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceHpfCutoff() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyHpfCutoff().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthV2Controller::setVoiceHpfCutoff(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceTune() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyTune().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
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
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setVoiceDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceAmpAttack() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpAttack().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthV2Controller::setVoiceAmpAttack(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpAttack().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceAmpHold() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpHold().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthV2Controller::setVoiceAmpHold(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpHold().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceAmpDecay() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpDecay().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthV2Controller::setVoiceAmpDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceAmpCurve() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpCurve().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthV2Controller::setVoiceAmpCurve(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpCurve().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::voiceAttack() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAttack().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
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
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
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
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
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
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
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
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthV2Controller::setKickPitchDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), Constants::NahdXml::xmlKeyPitchDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthV2Controller::snareSnappy() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynthV2::voiceId(static_cast<int>(DrumSynthV2::VoiceIndex::Snare)) + "_" + Constants::NahdXml::xmlKeySnappy().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
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
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
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
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
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
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
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
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 300;
}

void DrumSynthV2Controller::setVoiceResonance(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyResonance().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
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

QVariantList DrumSynthV2Controller::activeNotes() const
{
    QVariantList list;
    if (m_device) {
        for (int i = 0; i < DrumSynthV2::NumVoices; i++) {
            if (const uint8_t note = m_device->voiceNote(i); note > 0) {
                list.append(note);
            }
        }
        // Pedal HiHat is not a separate voice but it is an active note
        list.append(static_cast<uint8_t>(DrumSynthV2::MidiNote::PedalHiHat));
    }
    return list;
}

void DrumSynthV2Controller::playVoice(int index)
{
    if (m_device) {
        if (const uint8_t note = m_device->voiceNote(index); note > 0) {
            playNote(note, 1.0);
        }
    }
}

std::string DrumSynthV2Controller::currentVoicePrefix() const
{
    return DrumSynthV2::voiceId(m_selectedVoice) + "_";
}

void DrumSynthV2Controller::requestSettings()
{
    emit selectedVoiceChanged();
    emit voiceLevelChanged();
    emit voicePanChanged();
    emit voiceLpfCutoffChanged();
    emit voiceHpfCutoffChanged();
    emit lpfSlopeChanged();
    emit hpfSlopeChanged();
    emit voiceTuneChanged();
    emit voiceDecayChanged();
    emit voiceAttackChanged();
    emit voiceAmpAttackChanged();
    emit voiceAmpHoldChanged();
    emit voiceAmpDecayChanged();
    emit voiceAmpCurveChanged();
    emit kickAttackChanged();
    emit kickClickTuneChanged();
    emit kickPitchDepthChanged();
    emit kickPitchDecayChanged();
    emit snareSnappyChanged();
    emit snareToneChanged();
    emit tomPitchDepthChanged();
    emit tomPitchDecayChanged();
    emit voiceResonanceChanged();
    emit activeNotesChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
