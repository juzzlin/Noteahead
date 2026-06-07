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

#include "drum_synth_controller.hpp"

#include <QVariant>

#include "application/service/device_service.hpp"
#include "common/constants.hpp"
#include "common/utils.hpp"
#include "domain/devices/drum_synth_device.hpp"

#include <cmath>

namespace noteahead {

DrumSynthController::DrumSynthController(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DeviceController { parent }
  , m_deviceService { std::move(deviceService) }
{
}

DeviceController::DeviceS DrumSynthController::device() const
{
    return m_device;
}

bool DrumSynthController::setDevice(DeviceS device)
{
    if (const auto drumSynth = std::dynamic_pointer_cast<DrumSynthDevice>(device)) {
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

void DrumSynthController::setDevice(const QString & deviceName)
{
    setDevice(m_deviceService->device(deviceName.toStdString()));
}

int DrumSynthController::selectedVoice() const
{
    return m_selectedVoice;
}

void DrumSynthController::setSelectedVoice(int index)
{
    if (m_selectedVoice != index) {
        m_selectedVoice = index;
        emit selectedVoiceChanged();
        requestSettings();
    }
}

int DrumSynthController::voiceLevel() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyLevel().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthController::setVoiceLevel(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyLevel().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::voicePan() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyPan().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setVoicePan(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyPan().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::voiceLpfCutoff() const
{
    if (!m_device) {
        return 1000;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyCutoff().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 1000;
}

void DrumSynthController::setVoiceLpfCutoff(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyCutoff().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::voiceHpfCutoff() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyHpfCutoff().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthController::setVoiceHpfCutoff(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::voiceTune() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyTune().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setVoiceTune(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyTune().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::voiceDecay() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyDecay().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setVoiceDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::voiceAttack() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAttack().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthController::setVoiceAttack(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAttack().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::kickAttack() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynth::voiceId(static_cast<int>(DrumSynth::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyAttack().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setKickAttack(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynth::VoiceIndex::Kick), Constants::NahdXml::xmlKeyAttack().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::kickClickTune() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynth::voiceId(static_cast<int>(DrumSynth::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyClickTune().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setKickClickTune(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynth::VoiceIndex::Kick), Constants::NahdXml::xmlKeyClickTune().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::kickPitchDepth() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynth::voiceId(static_cast<int>(DrumSynth::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyPitchDepth().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setKickPitchDepth(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynth::VoiceIndex::Kick), Constants::NahdXml::xmlKeyPitchDepth().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::kickPitchDecay() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynth::voiceId(static_cast<int>(DrumSynth::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyPitchDecay().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setKickPitchDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynth::VoiceIndex::Kick), Constants::NahdXml::xmlKeyPitchDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::snareSnappy() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynth::voiceId(static_cast<int>(DrumSynth::VoiceIndex::Snare)) + "_" + Constants::NahdXml::xmlKeySnappy().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setSnareSnappy(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynth::VoiceIndex::Snare), Constants::NahdXml::xmlKeySnappy().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::snareTone() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(DrumSynth::voiceId(static_cast<int>(DrumSynth::VoiceIndex::Snare)) + "_" + Constants::NahdXml::xmlKeyTone().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setSnareTone(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(static_cast<int>(DrumSynth::VoiceIndex::Snare), Constants::NahdXml::xmlKeyTone().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::tomPitchDepth() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyPitchDepth().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setTomPitchDepth(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyPitchDepth().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::tomPitchDecay() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyPitchDecay().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setTomPitchDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyPitchDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::voiceResonance() const
{
    if (!m_device) {
        return 300;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyResonance().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().value() * Constants::uiInternalScaling())) : 300;
}

void DrumSynthController::setVoiceResonance(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyResonance().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool DrumSynthController::isKick() const
{
    return m_selectedVoice == static_cast<int>(DrumSynth::VoiceIndex::Kick);
}

bool DrumSynthController::isSnare() const
{
    return m_selectedVoice == static_cast<int>(DrumSynth::VoiceIndex::Snare);
}

bool DrumSynthController::isTom() const
{
    return m_selectedVoice >= static_cast<int>(DrumSynth::VoiceIndex::LowTom) && m_selectedVoice <= static_cast<int>(DrumSynth::VoiceIndex::HighTom);
}

bool DrumSynthController::isCymbal() const
{
    return m_selectedVoice >= static_cast<int>(DrumSynth::VoiceIndex::Crash) && m_selectedVoice <= static_cast<int>(DrumSynth::VoiceIndex::ReverseCrash);
}

bool DrumSynthController::hasResonance() const
{
    return (m_selectedVoice == static_cast<int>(DrumSynth::VoiceIndex::ClosedHiHat) || m_selectedVoice == static_cast<int>(DrumSynth::VoiceIndex::OpenHiHat)) || isCymbal();
}

bool DrumSynthController::hasAttack() const
{
    return isKick() || (m_selectedVoice == static_cast<int>(DrumSynth::VoiceIndex::Crash) || m_selectedVoice == static_cast<int>(DrumSynth::VoiceIndex::ReverseCrash));
}

QVariantList DrumSynthController::activeNotes() const
{
    QVariantList list;
    if (m_device) {
        for (int i = 0; i < DrumSynth::NumVoices; i++) {
            if (const uint8_t note = m_device->voiceNote(i); note > 0) {
                list.append(note);
            }
        }
        // Pedal HiHat is not a separate voice but it is an active note
        list.append(static_cast<uint8_t>(DrumSynth::MidiNote::PedalHiHat));
    }
    return list;
}

void DrumSynthController::playVoice(int index)
{
    if (m_device) {
        if (const uint8_t note = m_device->voiceNote(index); note > 0) {
            playNote(note, 1.0);
        }
    }
}

std::string DrumSynthController::currentVoicePrefix() const
{
    return DrumSynth::voiceId(m_selectedVoice) + "_";
}

void DrumSynthController::requestSettings()
{
    emit selectedVoiceChanged();
    emit voiceLevelChanged();
    emit voicePanChanged();
    emit voiceLpfCutoffChanged();
    emit voiceHpfCutoffChanged();
    emit voiceTuneChanged();
    emit voiceDecayChanged();
    emit voiceAttackChanged();
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
