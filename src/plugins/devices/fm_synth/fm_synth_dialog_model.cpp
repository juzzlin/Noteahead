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

#include "fm_synth_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/dsp/lfo.hpp"
#include "../../../view/controllers/fm_operator_controller.hpp"
#include "../../effects/delay/delay.hpp"
#include "fm_synth_device.hpp"
#include "fm_synth_presets.hpp"

#include <cmath>
#include <random>

namespace noteahead {

namespace {
//! The Time knob reads in milliseconds
constexpr float MaxDelayMs { FmSynthDevice::MaxDelaySeconds * 1000.0f };
} // namespace

FmSynthDialogModel::FmSynthDialogModel(std::shared_ptr<FmSynthDevice> synth, QObject * parent)
  : DeviceDialogModel { parent }
  , m_synth { std::move(synth) }
{
    for (size_t i = 0; i < FmSynthDevice::OperatorCount; i++) {
        auto * op = new FmOperatorController { i, this };
        op->setSynth(m_synth);
        m_operators.push_back(op);
    }
    connectDeviceSignals();
}

FmSynthDialogModel::~FmSynthDialogModel() = default;

void FmSynthDialogModel::retranslate()
{
    emit translationsChanged();
}

DeviceDialogModel::DeviceS FmSynthDialogModel::device() const
{
    return m_synth;
}

bool FmSynthDialogModel::setDevice(DeviceS device)
{
    if (const auto fmSynth = std::dynamic_pointer_cast<FmSynthDevice>(device); fmSynth) {
        if (m_synth) {
            disconnect(m_synth.get(), nullptr, this, nullptr);
        }
        m_synth = std::move(fmSynth);
        for (auto * op : m_operators) {
            op->setSynth(m_synth);
        }
        connectDeviceSignals();
        requestSettings();
        return true;
    }
    return false;
}

QVariantList FmSynthDialogModel::operators() const
{
    QVariantList list;
    for (auto * op : m_operators) {
        list << QVariant::fromValue(static_cast<QObject *>(op));
    }
    return list;
}

QStringList FmSynthDialogModel::factoryPresetNames() const
{
    QStringList list;
    for (const auto & preset : FmSynthPresets::presets()) {
        list << QString::fromStdString(preset.name);
    }
    return list;
}

void FmSynthDialogModel::loadFactoryPreset(int index)
{
    if (m_synth) {
        m_synth->loadPreset(index);
        requestSettings();
    }
}

void FmSynthDialogModel::randomizePatch()
{
    if (m_synth) {
        // Seeded from the system generator: the point is a different patch every time it is
        // pressed. The device takes a seed rather than rolling its own, so a test can ask for a
        // particular patch and get it back.
        std::random_device device;
        m_synth->loadRandomPatch(device());
        requestSettings();
    }
}

QStringList FmSynthDialogModel::algorithmNames() const
{
    QStringList list;
    for (auto && name : FmSynthDevice::algorithmNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmSynthDialogModel::operatorWaveformNames() const
{
    QStringList list;
    for (auto && name : FmOperator::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmSynthDialogModel::ratioNames() const
{
    QStringList list;
    for (int setting = 0; setting <= FmSynthDevice::MaxRatioSetting; setting++) {
        const double value = FmSynthDevice::ratioForSetting(setting);
        list << (value < 1.0 ? QString::number(value, 'f', 1) : QString::number(static_cast<int>(value)));
    }
    return list;
}

QStringList FmSynthDialogModel::voiceModes() const
{
    // Order is the persisted VoiceMode ordinal, so Mono trails the stacked modes rather than sitting
    // next to Poly.
    return { tr("Poly"), tr("Unison"), tr("Dual"), tr("Supersaw"), tr("Drift"), tr("Mono") };
}

QStringList FmSynthDialogModel::modTargetNames() const
{
    return { tr("Cutoff"), tr("Pitch"), tr("Mod Index"), tr("Feedback") };
}

QStringList FmSynthDialogModel::lfoWaveformNames() const
{
    QStringList list;
    for (auto && name : Lfo::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmSynthDialogModel::lfoModeNames() const
{
    return { tr("Normal"), tr("BPM"), tr("1-Shot") };
}

QStringList FmSynthDialogModel::lfoTargetNames() const
{
    return { tr("Pitch"), tr("Cutoff"), tr("Mod Index"), tr("Volume"), tr("Resonance"), tr("Pan") };
}

void FmSynthDialogModel::setDeviceService(std::shared_ptr<DeviceService> deviceService)
{
    m_deviceService = std::move(deviceService);
}

void FmSynthDialogModel::requestSettings()
{
    emit algorithmChanged();
    emit feedbackChanged();
    emit lpfCutoffChanged();
    emit lpfResonanceChanged();
    emit lpfSlopeChanged();
    emit hpfSlopeChanged();
    emit hpfCutoffChanged();
    emit ampAttackChanged();
    emit ampDecayChanged();
    emit ampSustainChanged();
    emit ampReleaseChanged();
    emit ampCurveChanged();
    emit ampVelocitySensitivityChanged();
    emit modAttackChanged();
    emit modDecayChanged();
    emit modSustainChanged();
    emit modIntChanged();
    emit modTargetChanged();
    emit modCurveChanged();
    emit lfoWaveformChanged();
    emit lfoModeChanged();
    emit lfoRateChanged();
    emit lfoIntChanged();
    emit lfoTargetChanged();
    emit lfoDelayChanged();
    emit lfoFadeChanged();
    emit lfo2WaveformChanged();
    emit lfo2ModeChanged();
    emit lfo2RateChanged();
    emit lfo2IntChanged();
    emit lfo2TargetChanged();
    emit lfo2DelayChanged();
    emit lfo2FadeChanged();
    emit lfoRateDividerChanged();
    emit lfo2RateDividerChanged();
    emit lfoFreeRunChanged();
    emit lfo2FreeRunChanged();
    emit voiceModeChanged();
    emit voiceDepthChanged();
    emit phaseRandomnessChanged();
    emit panSpreadChanged();
    emit portamentoChanged();
    emit pitchBendRangeChanged();
    emit delayTypeChanged();
    emit delayTimeChanged();
    emit delayFeedbackChanged();
    emit delayDepthChanged();
    emit delayMixChanged();
    emit delaySyncDivisionChanged();
    emit delayFeedbackLpfChanged();
    emit delayFeedbackHpfChanged();
    emit delaySyncChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();

    for (auto * op : m_operators) {
        op->refresh();
    }
}

int FmSynthDialogModel::algorithm() const
{
    return m_synth ? m_synth->algorithm() : 0;
}

void FmSynthDialogModel::setAlgorithm(int value)
{
    if (m_synth) {
        m_synth->setAlgorithm(value);
    }
}

int FmSynthDialogModel::feedback() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->feedback() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setFeedback(int value)
{
    if (m_synth) {
        m_synth->setFeedback(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lpfSlope() const
{
    return m_synth ? m_synth->lpfSlope() : 1;
}

void FmSynthDialogModel::setLpfSlope(int value)
{
    if (m_synth) {
        m_synth->setLpfSlope(value);
        emit lpfSlopeChanged();
    }
}

int FmSynthDialogModel::hpfSlope() const
{
    return m_synth ? m_synth->hpfSlope() : 1;
}

void FmSynthDialogModel::setHpfSlope(int value)
{
    if (m_synth) {
        m_synth->setHpfSlope(value);
        emit hpfSlopeChanged();
    }
}

int FmSynthDialogModel::lpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLpfResonance(int value)
{
    if (m_synth) {
        m_synth->setLpfResonance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::hpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setHpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::ampAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampAttack() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setAmpAttack(int value)
{
    if (m_synth) {
        m_synth->setAmpAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::ampDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampDecay() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setAmpDecay(int value)
{
    if (m_synth) {
        m_synth->setAmpDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::ampSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampSustain() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setAmpSustain(int value)
{
    if (m_synth) {
        m_synth->setAmpSustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::ampRelease() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampRelease() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setAmpRelease(int value)
{
    if (m_synth) {
        m_synth->setAmpRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::ampCurve() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampCurve() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setAmpCurve(int value)
{
    if (m_synth) {
        m_synth->setAmpCurve(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::ampVelocitySensitivity() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampVelocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setAmpVelocitySensitivity(int value)
{
    if (m_synth) {
        m_synth->setAmpVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::modAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modAttack() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setModAttack(int value)
{
    if (m_synth) {
        m_synth->setModAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::modDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modDecay() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setModDecay(int value)
{
    if (m_synth) {
        m_synth->setModDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::modSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modSustain() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setModSustain(int value)
{
    if (m_synth) {
        m_synth->setModSustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::modInt() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modInt() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setModInt(int value)
{
    if (m_synth) {
        m_synth->setModInt(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::modTarget() const
{
    return m_synth ? static_cast<int>(m_synth->modTarget()) : 0;
}

void FmSynthDialogModel::setModTarget(int value)
{
    if (m_synth) {
        m_synth->setModTarget(static_cast<FmSynthDevice::ModTarget>(value));
    }
}

int FmSynthDialogModel::modCurve() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modCurve() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setModCurve(int value)
{
    if (m_synth) {
        m_synth->setModCurve(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lfoWaveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfoWaveform()) : 0;
}

void FmSynthDialogModel::setLfoWaveform(int value)
{
    if (m_synth) {
        m_synth->setLfoWaveform(static_cast<Lfo::Waveform>(value));
    }
}

int FmSynthDialogModel::lfoMode() const
{
    return m_synth ? static_cast<int>(m_synth->lfoMode()) : 0;
}

void FmSynthDialogModel::setLfoMode(int value)
{
    if (m_synth) {
        m_synth->setLfoMode(static_cast<Lfo::Mode>(value));
    }
}

int FmSynthDialogModel::lfoRate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLfoRate(int value)
{
    if (m_synth) {
        m_synth->setLfoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lfoInt() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoInt() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLfoInt(int value)
{
    if (m_synth) {
        m_synth->setLfoInt(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lfoTarget() const
{
    return m_synth ? static_cast<int>(m_synth->lfoTarget()) : 0;
}

int FmSynthDialogModel::lfoDelay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoDelay() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLfoDelay(int value)
{
    if (m_synth) {
        m_synth->setLfoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lfoFade() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoFade() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLfoFade(int value)
{
    if (m_synth) {
        m_synth->setLfoFade(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void FmSynthDialogModel::setLfoTarget(int value)
{
    if (m_synth) {
        m_synth->setLfoTarget(static_cast<FmSynthDevice::LfoTarget>(value));
    }
}

int FmSynthDialogModel::lfo2Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Waveform()) : 0;
}

void FmSynthDialogModel::setLfo2Waveform(int value)
{
    if (m_synth) {
        m_synth->setLfo2Waveform(static_cast<Lfo::Waveform>(value));
    }
}

int FmSynthDialogModel::lfo2Mode() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Mode()) : 0;
}

void FmSynthDialogModel::setLfo2Mode(int value)
{
    if (m_synth) {
        m_synth->setLfo2Mode(static_cast<Lfo::Mode>(value));
    }
}

int FmSynthDialogModel::lfo2Rate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Rate() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLfo2Rate(int value)
{
    if (m_synth) {
        m_synth->setLfo2Rate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lfo2Int() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Int() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLfo2Int(int value)
{
    if (m_synth) {
        m_synth->setLfo2Int(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lfo2Target() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Target()) : 0;
}

int FmSynthDialogModel::lfo2Delay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Delay() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLfo2Delay(int value)
{
    if (m_synth) {
        m_synth->setLfo2Delay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lfo2Fade() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Fade() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setLfo2Fade(int value)
{
    if (m_synth) {
        m_synth->setLfo2Fade(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::lfoRateDivider() const
{
    return m_synth ? m_synth->lfoRateDivider() : 1;
}

void FmSynthDialogModel::setLfoRateDivider(int divider)
{
    if (m_synth) {
        m_synth->setLfoRateDivider(divider);
    }
}

int FmSynthDialogModel::lfo2RateDivider() const
{
    return m_synth ? m_synth->lfo2RateDivider() : 1;
}

void FmSynthDialogModel::setLfo2RateDivider(int divider)
{
    if (m_synth) {
        m_synth->setLfo2RateDivider(divider);
    }
}

bool FmSynthDialogModel::lfoFreeRun() const
{
    return m_synth ? m_synth->lfoFreeRun() : false;
}

void FmSynthDialogModel::setLfoFreeRun(bool freeRun)
{
    if (m_synth) {
        m_synth->setLfoFreeRun(freeRun);
    }
}

bool FmSynthDialogModel::lfo2FreeRun() const
{
    return m_synth ? m_synth->lfo2FreeRun() : false;
}

void FmSynthDialogModel::setLfo2FreeRun(bool freeRun)
{
    if (m_synth) {
        m_synth->setLfo2FreeRun(freeRun);
    }
}

int FmSynthDialogModel::maxLfoRateDivider() const
{
    return Lfo::MaxRateDivider;
}

void FmSynthDialogModel::setLfo2Target(int value)
{
    if (m_synth) {
        m_synth->setLfo2Target(static_cast<FmSynthDevice::LfoTarget>(value));
    }
}

int FmSynthDialogModel::voiceMode() const
{
    return m_synth ? static_cast<int>(m_synth->voiceMode()) : 0;
}

void FmSynthDialogModel::setVoiceMode(int value)
{
    if (m_synth) {
        m_synth->setVoiceMode(static_cast<FmSynthDevice::VoiceMode>(value));
    }
}

int FmSynthDialogModel::voiceDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->voiceDepth() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setVoiceDepth(int value)
{
    if (m_synth) {
        m_synth->setVoiceDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::phaseRandomness() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->phaseRandomness() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setPhaseRandomness(int randomness)
{
    if (m_synth) {
        m_synth->setPhaseRandomness(static_cast<float>(randomness) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::panSpread() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->panSpread() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setPanSpread(int value)
{
    if (m_synth) {
        m_synth->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::portamento() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->portamento() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setPortamento(int value)
{
    if (m_synth) {
        m_synth->setPortamento(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::pitchBendRange() const
{
    return m_synth ? m_synth->pitchBendRange() : 0;
}

void FmSynthDialogModel::setPitchBendRange(int value)
{
    if (m_synth) {
        m_synth->setPitchBendRange(value);
    }
}

int FmSynthDialogModel::delayType() const
{
    return m_synth ? static_cast<int>(m_synth->delayType()) : 0;
}

void FmSynthDialogModel::setDelayType(int value)
{
    if (m_synth) {
        m_synth->setDelayType(static_cast<Delay::Type>(value));
    }
}

int FmSynthDialogModel::delayTime() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayTime() * MaxDelayMs)) : 0;
}

void FmSynthDialogModel::setDelayTime(int value)
{
    if (m_synth) {
        m_synth->setDelayTime(static_cast<float>(value) / MaxDelayMs);
    }
}

int FmSynthDialogModel::delayFeedback() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedback() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setDelayFeedback(int value)
{
    if (m_synth) {
        m_synth->setDelayFeedback(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::delayDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayDepth() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setDelayDepth(int value)
{
    if (m_synth) {
        m_synth->setDelayDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::delayMix() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayMix() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setDelayMix(int value)
{
    if (m_synth) {
        m_synth->setDelayMix(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::delaySyncDivision() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delaySyncDivision() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setDelaySyncDivision(int value)
{
    if (m_synth) {
        m_synth->setDelaySyncDivision(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::delayFeedbackLpf() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackLpf() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setDelayFeedbackLpf(int value)
{
    if (m_synth) {
        m_synth->setDelayFeedbackLpf(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthDialogModel::delayFeedbackHpf() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackHpf() * Constants::uiInternalScaling())) : 0;
}

void FmSynthDialogModel::setDelayFeedbackHpf(int value)
{
    if (m_synth) {
        m_synth->setDelayFeedbackHpf(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool FmSynthDialogModel::delaySync() const
{
    return m_synth ? m_synth->delaySync() : false;
}

void FmSynthDialogModel::setDelaySync(bool value)
{
    if (m_synth) {
        m_synth->setDelaySync(value);
    }
}

} // namespace noteahead
