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

#include "fm_drums_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/drum_pad_device.hpp"
#include "../../domain/devices/fm_drums_device.hpp"
#include "../../domain/devices/fm_drums_presets.hpp"
#include "../../domain/devices/fm_synth_device.hpp"
#include "fm_drum_operator_controller.hpp"

#include <cmath>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

FmDrumsController::FmDrumsController(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DrumPadController { std::move(deviceService), parent }
{
    for (size_t op = 0; op < FmDrumEngine::OperatorCount; op++) {
        m_operators.push_back(new FmDrumOperatorController { op, *this });
    }
}

bool FmDrumsController::acceptsDevice(const DeviceS & device) const
{
    return std::dynamic_pointer_cast<FmDrumsDevice>(device) != nullptr;
}

int FmDrumsController::knob(const QString & key) const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + key.toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void FmDrumsController::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, key.toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmDrumsController::choice(const QString & key) const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + key.toStdString());
    return parameter ? static_cast<int>(std::lround(parameter->get().shownValue())) : 0;
}

void FmDrumsController::setChoice(const QString & key, int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, key.toStdString(), static_cast<float>(value));
    }
}

QVariantList FmDrumsController::operators() const
{
    QVariantList list;
    for (auto * op : m_operators) {
        list << QVariant::fromValue(static_cast<QObject *>(op));
    }
    return list;
}

QStringList FmDrumsController::algorithmNames() const
{
    QStringList list;
    for (auto && name : FmSynthDevice::algorithmNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmDrumsController::operatorWaveformNames() const
{
    QStringList list;
    for (auto && name : FmOperator::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmDrumsController::noiseFilterModeNames() const
{
    // In FmDrumEngine::NoiseFilterMode order; filter jargon, so not translated
    return { "LP", "BP", "HP" };
}

QStringList FmDrumsController::padPresetNames() const
{
    QStringList list;
    for (auto && preset : FmDrumsPresets::pads()) {
        list << QString::fromStdString(preset.name);
    }
    return list;
}

void FmDrumsController::loadPadPreset(int index)
{
    if (const auto drums = std::dynamic_pointer_cast<FmDrumsDevice>(m_device); drums && index >= 0) {
        drums->loadPadPreset(m_selectedVoice, static_cast<size_t>(index));
    }
}

QStringList FmDrumsController::factoryPresetNames() const
{
    QStringList list;
    for (auto && kit : FmDrumsPresets::kits()) {
        list << QString::fromStdString(kit.name);
    }
    return list;
}

void FmDrumsController::loadFactoryPreset(int index)
{
    if (const auto drums = std::dynamic_pointer_cast<FmDrumsDevice>(m_device); drums && index >= 0) {
        drums->loadKit(static_cast<size_t>(index));
    }
}

int FmDrumsController::algorithm() const
{
    return choice(NahdXml::xmlKeyAlgorithm());
}

void FmDrumsController::setAlgorithm(int algorithm)
{
    setChoice(NahdXml::xmlKeyAlgorithm(), algorithm);
}

int FmDrumsController::pitch() const
{
    return knob(NahdXml::xmlKeyFrequency());
}

void FmDrumsController::setPitch(int value)
{
    setKnob(NahdXml::xmlKeyFrequency(), value);
}

int FmDrumsController::feedback() const
{
    return knob(NahdXml::xmlKeyFeedback());
}

void FmDrumsController::setFeedback(int value)
{
    setKnob(NahdXml::xmlKeyFeedback(), value);
}

int FmDrumsController::pitchDepth() const
{
    return knob(NahdXml::xmlKeyPitchDepth());
}

void FmDrumsController::setPitchDepth(int value)
{
    setKnob(NahdXml::xmlKeyPitchDepth(), value);
}

int FmDrumsController::pitchDecay() const
{
    return knob(NahdXml::xmlKeyPitchDecay());
}

void FmDrumsController::setPitchDecay(int value)
{
    setKnob(NahdXml::xmlKeyPitchDecay(), value);
}

int FmDrumsController::noiseLevel() const
{
    return knob(NahdXml::xmlKeyNoiseLevel());
}

void FmDrumsController::setNoiseLevel(int value)
{
    setKnob(NahdXml::xmlKeyNoiseLevel(), value);
}

int FmDrumsController::noiseModulation() const
{
    return knob(NahdXml::xmlKeyNoiseModulation());
}

void FmDrumsController::setNoiseModulation(int value)
{
    setKnob(NahdXml::xmlKeyNoiseModulation(), value);
}

int FmDrumsController::noiseTarget() const
{
    return choice(NahdXml::xmlKeyNoiseTarget());
}

void FmDrumsController::setNoiseTarget(int value)
{
    setChoice(NahdXml::xmlKeyNoiseTarget(), value);
}

int FmDrumsController::noiseAttack() const
{
    return knob(NahdXml::xmlKeyNoiseAttack());
}

void FmDrumsController::setNoiseAttack(int value)
{
    setKnob(NahdXml::xmlKeyNoiseAttack(), value);
}

int FmDrumsController::noiseDecay() const
{
    return knob(NahdXml::xmlKeyNoiseDecay());
}

void FmDrumsController::setNoiseDecay(int value)
{
    setKnob(NahdXml::xmlKeyNoiseDecay(), value);
}

int FmDrumsController::noiseCurve() const
{
    return knob(NahdXml::xmlKeyNoiseCurve());
}

void FmDrumsController::setNoiseCurve(int value)
{
    setKnob(NahdXml::xmlKeyNoiseCurve(), value);
}

int FmDrumsController::noiseFilterMode() const
{
    return choice(NahdXml::xmlKeyNoiseFilterMode());
}

void FmDrumsController::setNoiseFilterMode(int value)
{
    setChoice(NahdXml::xmlKeyNoiseFilterMode(), value);
}

int FmDrumsController::noiseCutoff() const
{
    return knob(NahdXml::xmlKeyNoiseCutoff());
}

void FmDrumsController::setNoiseCutoff(int value)
{
    setKnob(NahdXml::xmlKeyNoiseCutoff(), value);
}

int FmDrumsController::noiseResonance() const
{
    return knob(NahdXml::xmlKeyNoiseResonance());
}

void FmDrumsController::setNoiseResonance(int value)
{
    setKnob(NahdXml::xmlKeyNoiseResonance(), value);
}

int FmDrumsController::velocitySensitivity() const
{
    return knob(NahdXml::xmlKeyVelocitySensitivity());
}

void FmDrumsController::setVelocitySensitivity(int value)
{
    setKnob(NahdXml::xmlKeyVelocitySensitivity(), value);
}

int FmDrumsController::modVelocitySensitivity() const
{
    return knob(NahdXml::xmlKeyModVelocitySensitivity());
}

void FmDrumsController::setModVelocitySensitivity(int value)
{
    setKnob(NahdXml::xmlKeyModVelocitySensitivity(), value);
}

void FmDrumsController::requestSettings()
{
    DrumPadController::requestSettings();

    emit padChanged();
    for (auto * op : m_operators) {
        emit op->changed();
    }
}

} // namespace noteahead
