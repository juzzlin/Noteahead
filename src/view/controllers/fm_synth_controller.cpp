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

#include "fm_synth_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/fm_synth_device.hpp"
#include "../../domain/devices/fm_synth_presets.hpp"
#include "../../domain/dsp/lfo.hpp"
#include "../../domain/effects/delay.hpp"
#include "fm_operator_controller.hpp"

#include <cmath>
#include <random>

namespace noteahead {

FmSynthController::FmSynthController(std::shared_ptr<FmSynthDevice> synth, QObject * parent)
  : DeviceController { parent }
  , m_synth { std::move(synth) }
{
    for (size_t i = 0; i < FmSynthDevice::OperatorCount; i++) {
        auto * op = new FmOperatorController { i, this };
        op->setSynth(m_synth);
        m_operators.push_back(op);
    }
    connectDeviceSignals();
}

FmSynthController::~FmSynthController() = default;

void FmSynthController::retranslate()
{
    emit translationsChanged();
}

DeviceController::DeviceS FmSynthController::device() const
{
    return m_synth;
}

bool FmSynthController::setDevice(DeviceS device)
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

QVariantList FmSynthController::operators() const
{
    QVariantList list;
    for (auto * op : m_operators) {
        list << QVariant::fromValue(static_cast<QObject *>(op));
    }
    return list;
}

QStringList FmSynthController::factoryPresetNames() const
{
    QStringList list;
    for (const auto & preset : FmSynthPresets::presets()) {
        list << QString::fromStdString(preset.name);
    }
    return list;
}

void FmSynthController::loadFactoryPreset(int index)
{
    if (m_synth) {
        m_synth->loadPreset(index);
        requestSettings();
    }
}

void FmSynthController::randomizePatch()
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

QStringList FmSynthController::algorithmNames() const
{
    QStringList list;
    for (auto && name : FmSynthDevice::algorithmNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmSynthController::operatorWaveformNames() const
{
    QStringList list;
    for (auto && name : FmOperator::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmSynthController::ratioNames() const
{
    QStringList list;
    for (int setting = 0; setting <= FmSynthDevice::MaxRatioSetting; setting++) {
        const double value = FmSynthDevice::ratioForSetting(setting);
        list << (value < 1.0 ? QString::number(value, 'f', 1) : QString::number(static_cast<int>(value)));
    }
    return list;
}

QStringList FmSynthController::voiceModes() const
{
    // Order is the persisted VoiceMode ordinal, so Mono trails the stacked modes rather than sitting
    // next to Poly.
    return { tr("Poly"), tr("Unison"), tr("Dual"), tr("Supersaw"), tr("Drift"), tr("Mono") };
}

QStringList FmSynthController::modTargetNames() const
{
    return { tr("Cutoff"), tr("Pitch"), tr("Mod Index"), tr("Feedback") };
}

QStringList FmSynthController::lfoWaveformNames() const
{
    QStringList list;
    for (auto && name : Lfo::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList FmSynthController::lfoModeNames() const
{
    return { tr("Normal"), tr("BPM"), tr("1-Shot") };
}

QStringList FmSynthController::lfoTargetNames() const
{
    return { tr("Pitch"), tr("Cutoff"), tr("Mod Index"), tr("Volume"), tr("Resonance"), tr("Pan") };
}

void FmSynthController::setDeviceService(std::shared_ptr<DeviceService> deviceService)
{
    m_deviceService = std::move(deviceService);
}

void FmSynthController::requestSettings()
{
    emit algorithmChanged();
    emit feedbackChanged();
    emit lpfCutoffChanged();
    emit lpfResonanceChanged();
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
    emit lfo2WaveformChanged();
    emit lfo2ModeChanged();
    emit lfo2RateChanged();
    emit lfo2IntChanged();
    emit lfo2TargetChanged();
    emit voiceModeChanged();
    emit voiceDepthChanged();
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

void FmSynthController::connectDeviceSignals()
{
    if (!m_synth) {
        return;
    }

    connect(m_synth.get(), &FmSynthDevice::dataChanged, this, &FmSynthController::requestSettings);
}

int FmSynthController::algorithm() const
{
    return m_synth ? m_synth->algorithm() : 0;
}

void FmSynthController::setAlgorithm(int value)
{
    if (m_synth) {
        m_synth->setAlgorithm(value);
    }
}

int FmSynthController::feedback() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->feedback() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setFeedback(int value)
{
    if (m_synth) {
        m_synth->setFeedback(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::lpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setLpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::lpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setLpfResonance(int value)
{
    if (m_synth) {
        m_synth->setLpfResonance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::hpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setHpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::ampAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampAttack() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setAmpAttack(int value)
{
    if (m_synth) {
        m_synth->setAmpAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::ampDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampDecay() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setAmpDecay(int value)
{
    if (m_synth) {
        m_synth->setAmpDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::ampSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampSustain() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setAmpSustain(int value)
{
    if (m_synth) {
        m_synth->setAmpSustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::ampRelease() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampRelease() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setAmpRelease(int value)
{
    if (m_synth) {
        m_synth->setAmpRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::ampCurve() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampCurve() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setAmpCurve(int value)
{
    if (m_synth) {
        m_synth->setAmpCurve(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::ampVelocitySensitivity() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampVelocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setAmpVelocitySensitivity(int value)
{
    if (m_synth) {
        m_synth->setAmpVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::modAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modAttack() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setModAttack(int value)
{
    if (m_synth) {
        m_synth->setModAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::modDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modDecay() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setModDecay(int value)
{
    if (m_synth) {
        m_synth->setModDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::modSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modSustain() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setModSustain(int value)
{
    if (m_synth) {
        m_synth->setModSustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::modInt() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modInt() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setModInt(int value)
{
    if (m_synth) {
        m_synth->setModInt(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::modTarget() const
{
    return m_synth ? static_cast<int>(m_synth->modTarget()) : 0;
}

void FmSynthController::setModTarget(int value)
{
    if (m_synth) {
        m_synth->setModTarget(static_cast<FmSynthDevice::ModTarget>(value));
    }
}

int FmSynthController::modCurve() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modCurve() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setModCurve(int value)
{
    if (m_synth) {
        m_synth->setModCurve(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::lfoWaveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfoWaveform()) : 0;
}

void FmSynthController::setLfoWaveform(int value)
{
    if (m_synth) {
        m_synth->setLfoWaveform(static_cast<Lfo::Waveform>(value));
    }
}

int FmSynthController::lfoMode() const
{
    return m_synth ? static_cast<int>(m_synth->lfoMode()) : 0;
}

void FmSynthController::setLfoMode(int value)
{
    if (m_synth) {
        m_synth->setLfoMode(static_cast<Lfo::Mode>(value));
    }
}

int FmSynthController::lfoRate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setLfoRate(int value)
{
    if (m_synth) {
        m_synth->setLfoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::lfoInt() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoInt() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setLfoInt(int value)
{
    if (m_synth) {
        m_synth->setLfoInt(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::lfoTarget() const
{
    return m_synth ? static_cast<int>(m_synth->lfoTarget()) : 0;
}

void FmSynthController::setLfoTarget(int value)
{
    if (m_synth) {
        m_synth->setLfoTarget(static_cast<FmSynthDevice::LfoTarget>(value));
    }
}

int FmSynthController::lfo2Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Waveform()) : 0;
}

void FmSynthController::setLfo2Waveform(int value)
{
    if (m_synth) {
        m_synth->setLfo2Waveform(static_cast<Lfo::Waveform>(value));
    }
}

int FmSynthController::lfo2Mode() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Mode()) : 0;
}

void FmSynthController::setLfo2Mode(int value)
{
    if (m_synth) {
        m_synth->setLfo2Mode(static_cast<Lfo::Mode>(value));
    }
}

int FmSynthController::lfo2Rate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Rate() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setLfo2Rate(int value)
{
    if (m_synth) {
        m_synth->setLfo2Rate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::lfo2Int() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Int() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setLfo2Int(int value)
{
    if (m_synth) {
        m_synth->setLfo2Int(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::lfo2Target() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Target()) : 0;
}

void FmSynthController::setLfo2Target(int value)
{
    if (m_synth) {
        m_synth->setLfo2Target(static_cast<FmSynthDevice::LfoTarget>(value));
    }
}

int FmSynthController::voiceMode() const
{
    return m_synth ? static_cast<int>(m_synth->voiceMode()) : 0;
}

void FmSynthController::setVoiceMode(int value)
{
    if (m_synth) {
        m_synth->setVoiceMode(static_cast<FmSynthDevice::VoiceMode>(value));
    }
}

int FmSynthController::voiceDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->voiceDepth() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setVoiceDepth(int value)
{
    if (m_synth) {
        m_synth->setVoiceDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::panSpread() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->panSpread() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setPanSpread(int value)
{
    if (m_synth) {
        m_synth->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::portamento() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->portamento() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setPortamento(int value)
{
    if (m_synth) {
        m_synth->setPortamento(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::pitchBendRange() const
{
    return m_synth ? m_synth->pitchBendRange() : 0;
}

void FmSynthController::setPitchBendRange(int value)
{
    if (m_synth) {
        m_synth->setPitchBendRange(value);
    }
}

int FmSynthController::delayType() const
{
    return m_synth ? static_cast<int>(m_synth->delayType()) : 0;
}

void FmSynthController::setDelayType(int value)
{
    if (m_synth) {
        m_synth->setDelayType(static_cast<Delay::Type>(value));
    }
}

int FmSynthController::delayTime() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayTime() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setDelayTime(int value)
{
    if (m_synth) {
        m_synth->setDelayTime(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::delayFeedback() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedback() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setDelayFeedback(int value)
{
    if (m_synth) {
        m_synth->setDelayFeedback(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::delayDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayDepth() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setDelayDepth(int value)
{
    if (m_synth) {
        m_synth->setDelayDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::delayMix() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayMix() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setDelayMix(int value)
{
    if (m_synth) {
        m_synth->setDelayMix(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::delaySyncDivision() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delaySyncDivision() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setDelaySyncDivision(int value)
{
    if (m_synth) {
        m_synth->setDelaySyncDivision(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::delayFeedbackLpf() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackLpf() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setDelayFeedbackLpf(int value)
{
    if (m_synth) {
        m_synth->setDelayFeedbackLpf(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int FmSynthController::delayFeedbackHpf() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackHpf() * Constants::uiInternalScaling())) : 0;
}

void FmSynthController::setDelayFeedbackHpf(int value)
{
    if (m_synth) {
        m_synth->setDelayFeedbackHpf(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool FmSynthController::delaySync() const
{
    return m_synth ? m_synth->delaySync() : false;
}

void FmSynthController::setDelaySync(bool value)
{
    if (m_synth) {
        m_synth->setDelaySync(value);
    }
}

} // namespace noteahead
