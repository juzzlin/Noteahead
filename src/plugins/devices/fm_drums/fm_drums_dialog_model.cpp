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

#include "fm_drums_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/devices/drum_pad_device.hpp"
#include "../../../view/controllers/fm_drum_operator_controller.hpp"
#include "../fm_synth/fm_synth_device.hpp"
#include "fm_drums_device.hpp"
#include "fm_drums_presets.hpp"

#include <cmath>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

FmDrumsDialogModel::FmDrumsDialogModel(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DrumPadDialogModel { std::move(deviceService), parent }
{
    for (size_t op = 0; op < FmDrumEngine::OperatorCount; op++) {
        m_operators.push_back(new FmDrumOperatorController { op, *this });
    }
}

bool FmDrumsDialogModel::acceptsDevice(const DeviceS & device) const
{
    return std::dynamic_pointer_cast<FmDrumsDevice>(device) != nullptr;
}

int FmDrumsDialogModel::knob(const QString & key) const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + key.toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void FmDrumsDialogModel::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, key.toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmDrumsDialogModel::choice(const QString & key) const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + key.toStdString());
    return parameter ? static_cast<int>(std::lround(parameter->get().shownValue())) : 0;
}

void FmDrumsDialogModel::setChoice(const QString & key, int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, key.toStdString(), static_cast<float>(value));
    }
}

QVariantList FmDrumsDialogModel::operators() const
{
    QVariantList list;
    for (auto * op : m_operators) {
        list << QVariant::fromValue(static_cast<QObject *>(op));
    }
    return list;
}

QStringList FmDrumsDialogModel::algorithmNames() const
{
    QStringList list;
    for (auto && name : FmSynthDevice::algorithmNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmDrumsDialogModel::operatorWaveformNames() const
{
    QStringList list;
    for (auto && name : FmOperator::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmDrumsDialogModel::noiseFilterModeNames() const
{
    // In FmDrumEngine::NoiseFilterMode order; filter jargon, so not translated
    return { "LP", "BP", "HP" };
}

QStringList FmDrumsDialogModel::padPresetNames() const
{
    QStringList list;
    for (auto && preset : FmDrumsPresets::pads()) {
        list << QString::fromStdString(preset.name);
    }
    return list;
}

void FmDrumsDialogModel::loadPadPreset(int index)
{
    if (const auto drums = std::dynamic_pointer_cast<FmDrumsDevice>(m_device); drums && index >= 0) {
        drums->loadPadPreset(m_selectedVoice, static_cast<size_t>(index));
    }
}

QStringList FmDrumsDialogModel::factoryPresetNames() const
{
    QStringList list;
    for (auto && kit : FmDrumsPresets::kits()) {
        list << QString::fromStdString(kit.name);
    }
    return list;
}

void FmDrumsDialogModel::loadFactoryPreset(int index)
{
    if (const auto drums = std::dynamic_pointer_cast<FmDrumsDevice>(m_device); drums && index >= 0) {
        drums->loadKit(static_cast<size_t>(index));
    }
}

int FmDrumsDialogModel::algorithm() const
{
    return choice(NahdXml::xmlKeyAlgorithm());
}

void FmDrumsDialogModel::setAlgorithm(int algorithm)
{
    setChoice(NahdXml::xmlKeyAlgorithm(), algorithm);
}

int FmDrumsDialogModel::pitch() const
{
    return knob(NahdXml::xmlKeyFrequency());
}

void FmDrumsDialogModel::setPitch(int value)
{
    setKnob(NahdXml::xmlKeyFrequency(), value);
}

int FmDrumsDialogModel::feedback() const
{
    return knob(NahdXml::xmlKeyFeedback());
}

void FmDrumsDialogModel::setFeedback(int value)
{
    setKnob(NahdXml::xmlKeyFeedback(), value);
}

int FmDrumsDialogModel::pitchDepth() const
{
    return knob(NahdXml::xmlKeyPitchDepth());
}

void FmDrumsDialogModel::setPitchDepth(int value)
{
    setKnob(NahdXml::xmlKeyPitchDepth(), value);
}

int FmDrumsDialogModel::pitchDecay() const
{
    return knob(NahdXml::xmlKeyPitchDecay());
}

void FmDrumsDialogModel::setPitchDecay(int value)
{
    setKnob(NahdXml::xmlKeyPitchDecay(), value);
}

int FmDrumsDialogModel::noiseLevel() const
{
    return knob(NahdXml::xmlKeyNoiseLevel());
}

void FmDrumsDialogModel::setNoiseLevel(int value)
{
    setKnob(NahdXml::xmlKeyNoiseLevel(), value);
}

int FmDrumsDialogModel::noiseModulation() const
{
    return knob(NahdXml::xmlKeyNoiseModulation());
}

void FmDrumsDialogModel::setNoiseModulation(int value)
{
    setKnob(NahdXml::xmlKeyNoiseModulation(), value);
}

int FmDrumsDialogModel::noiseTarget() const
{
    return choice(NahdXml::xmlKeyNoiseTarget());
}

void FmDrumsDialogModel::setNoiseTarget(int value)
{
    setChoice(NahdXml::xmlKeyNoiseTarget(), value);
}

int FmDrumsDialogModel::noiseAttack() const
{
    return knob(NahdXml::xmlKeyNoiseAttack());
}

void FmDrumsDialogModel::setNoiseAttack(int value)
{
    setKnob(NahdXml::xmlKeyNoiseAttack(), value);
}

int FmDrumsDialogModel::noiseDecay() const
{
    return knob(NahdXml::xmlKeyNoiseDecay());
}

void FmDrumsDialogModel::setNoiseDecay(int value)
{
    setKnob(NahdXml::xmlKeyNoiseDecay(), value);
}

int FmDrumsDialogModel::noiseCurve() const
{
    return knob(NahdXml::xmlKeyNoiseCurve());
}

void FmDrumsDialogModel::setNoiseCurve(int value)
{
    setKnob(NahdXml::xmlKeyNoiseCurve(), value);
}

int FmDrumsDialogModel::noiseFilterMode() const
{
    return choice(NahdXml::xmlKeyNoiseFilterMode());
}

void FmDrumsDialogModel::setNoiseFilterMode(int value)
{
    setChoice(NahdXml::xmlKeyNoiseFilterMode(), value);
}

int FmDrumsDialogModel::noiseCutoff() const
{
    return knob(NahdXml::xmlKeyNoiseCutoff());
}

void FmDrumsDialogModel::setNoiseCutoff(int value)
{
    setKnob(NahdXml::xmlKeyNoiseCutoff(), value);
}

int FmDrumsDialogModel::noiseResonance() const
{
    return knob(NahdXml::xmlKeyNoiseResonance());
}

void FmDrumsDialogModel::setNoiseResonance(int value)
{
    setKnob(NahdXml::xmlKeyNoiseResonance(), value);
}

int FmDrumsDialogModel::velocitySensitivity() const
{
    return knob(NahdXml::xmlKeyVelocitySensitivity());
}

void FmDrumsDialogModel::setVelocitySensitivity(int value)
{
    setKnob(NahdXml::xmlKeyVelocitySensitivity(), value);
}

int FmDrumsDialogModel::modVelocitySensitivity() const
{
    return knob(NahdXml::xmlKeyModVelocitySensitivity());
}

void FmDrumsDialogModel::setModVelocitySensitivity(int value)
{
    setKnob(NahdXml::xmlKeyModVelocitySensitivity(), value);
}

void FmDrumsDialogModel::requestSettings()
{
    DrumPadDialogModel::requestSettings();

    emit padChanged();
    for (auto * op : m_operators) {
        emit op->changed();
    }
}

} // namespace noteahead
