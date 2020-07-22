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

#include "device_service.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/devices/drum_synth_device.hpp"

#include <cmath>

namespace noteahead {

DrumSynthController::DrumSynthController(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
{
}

void DrumSynthController::setDevice(const QString & deviceName)
{
    if (auto dev = std::dynamic_pointer_cast<DrumSynthDevice>(m_deviceService->device(deviceName.toStdString()))) {
        if (m_device) {
            disconnect(m_device.get(), &Device::dataChanged, this, &DrumSynthController::updateProperties);
        }
        m_device = dev;
        connect(m_device.get(), &Device::dataChanged, this, &DrumSynthController::updateProperties, Qt::UniqueConnection);
        connect(m_device.get(), &Device::dataChanged, this, &DrumSynthController::sampleRateChanged, Qt::UniqueConnection);
        updateProperties();
    }
}

uint32_t DrumSynthController::sampleRate() const
{
    return m_device ? m_device->sampleRate() : static_cast<uint32_t>(Constants::defaultSampleRate());
}

float DrumSynthController::cutoffToHz(float cutoff) const
{
    return Utils::Dsp::cutoffToHz(cutoff / Constants::uiInternalScaling(), static_cast<float>(sampleRate()));
}

int DrumSynthController::selectedPad() const { return m_selectedPad; }
void DrumSynthController::setSelectedPad(int index) { if (m_selectedPad != index) { m_selectedPad = index; emit selectedPadChanged(); updateProperties(); } }

int DrumSynthController::padLevel() const
{
    if (!m_device) return 0;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyLevel().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthController::setPadLevel(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyLevel().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::padPan() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyPan().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setPadPan(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyPan().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::padLpfCutoff() const
{
    if (!m_device) return 1000;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyCutoff().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 1000;
}

void DrumSynthController::setPadLpfCutoff(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyCutoff().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::padHpfCutoff() const
{
    if (!m_device) return 0;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyHpfCutoff().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthController::setPadHpfCutoff(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::padTune() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyTune().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setPadTune(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyTune().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::padDecay() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyDecay().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setPadDecay(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::padAttack() const
{
    if (!m_device) return 0;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyAttack().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 0;
}

void DrumSynthController::setPadAttack(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyAttack().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::kickAttack() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter("Pad" + std::to_string(DrumSynthDevice::Kick) + "_" + Constants::NahdXml::xmlKeyAttack().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setKickAttack(int value)
{
    if (m_device) {
        m_device->updatePadParameter(DrumSynthDevice::Kick, Constants::NahdXml::xmlKeyAttack().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::kickClickTune() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter("Pad" + std::to_string(DrumSynthDevice::Kick) + "_" + Constants::NahdXml::xmlKeyClickTune().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setKickClickTune(int value)
{
    if (m_device) {
        m_device->updatePadParameter(DrumSynthDevice::Kick, Constants::NahdXml::xmlKeyClickTune().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::kickPitchDepth() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter("Pad" + std::to_string(DrumSynthDevice::Kick) + "_" + Constants::NahdXml::xmlKeyPitchDepth().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setKickPitchDepth(int value)
{
    if (m_device) {
        m_device->updatePadParameter(DrumSynthDevice::Kick, Constants::NahdXml::xmlKeyPitchDepth().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::kickPitchDecay() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter("Pad" + std::to_string(DrumSynthDevice::Kick) + "_" + Constants::NahdXml::xmlKeyPitchDecay().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setKickPitchDecay(int value)
{
    if (m_device) {
        m_device->updatePadParameter(DrumSynthDevice::Kick, Constants::NahdXml::xmlKeyPitchDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::snareSnappy() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter("Pad" + std::to_string(DrumSynthDevice::Snare) + "_" + Constants::NahdXml::xmlKeySnappy().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setSnareSnappy(int value)
{
    if (m_device) {
        m_device->updatePadParameter(DrumSynthDevice::Snare, Constants::NahdXml::xmlKeySnappy().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::snareTone() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter("Pad" + std::to_string(DrumSynthDevice::Snare) + "_" + Constants::NahdXml::xmlKeyTone().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setSnareTone(int value)
{
    if (m_device) {
        m_device->updatePadParameter(DrumSynthDevice::Snare, Constants::NahdXml::xmlKeyTone().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::tomPitchDepth() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyPitchDepth().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setTomPitchDepth(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyPitchDepth().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::tomPitchDecay() const
{
    if (!m_device) return 500;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyPitchDecay().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 500;
}

void DrumSynthController::setTomPitchDecay(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyPitchDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::padResonance() const
{
    if (!m_device) return 300;
    auto p = m_device->parameter(currentPadPrefix() + Constants::NahdXml::xmlKeyResonance().toStdString());
    return p ? static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling())) : 300;
}

void DrumSynthController::setPadResonance(int value)
{
    if (m_device) {
        m_device->updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyResonance().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumSynthController::volume() const { return m_device ? static_cast<int>(std::round(m_device->volume() * Constants::uiInternalScaling())) : 1000; }
void DrumSynthController::setVolume(int value) { if (m_device) m_device->setVolume(static_cast<float>(value) / Constants::uiInternalScaling()); }

int DrumSynthController::gain() const { return m_device ? static_cast<int>(std::round(m_device->gain() * Constants::uiInternalScaling())) : 500; }
void DrumSynthController::setGain(int value) { if (m_device) m_device->setGain(static_cast<float>(value) / Constants::uiInternalScaling()); }

int DrumSynthController::pan() const { return m_device ? static_cast<int>(std::round(m_device->pan() * Constants::uiInternalScaling())) : 500; }
void DrumSynthController::setPan(int value) { if (m_device) m_device->setPan(static_cast<float>(value) / Constants::uiInternalScaling()); }

void DrumSynthController::playNote(int note, double velocity)
{
    if (m_device) {
        m_device->processMidiNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity * 127.0));
    }
}

void DrumSynthController::stopNote(int note)
{
    if (m_device) {
        m_device->processMidiNoteOff(static_cast<uint8_t>(note));
    }
}

void DrumSynthController::playPad(int index)
{
    if (m_device) {
        const uint8_t note = m_device->padNote(index);
        if (note > 0) {
            playNote(note, 1.0);
        }
    }
}

std::string DrumSynthController::currentPadPrefix() const
{
    return "Pad" + std::to_string(m_selectedPad) + "_";
}

void DrumSynthController::updateProperties()
{
    emit selectedPadChanged();
    emit padLevelChanged();
    emit padPanChanged();
    emit padLpfCutoffChanged();
    emit padHpfCutoffChanged();
    emit padTuneChanged();
    emit padDecayChanged();
    emit padAttackChanged();
    emit kickAttackChanged();
    emit kickClickTuneChanged();
    emit kickPitchDepthChanged();
    emit kickPitchDecayChanged();
    emit snareSnappyChanged();
    emit snareToneChanged();
    emit tomPitchDepthChanged();
    emit tomPitchDecayChanged();
    emit padResonanceChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
