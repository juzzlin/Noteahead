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

#include "ju_synth_08_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "ju_synth_08_device.hpp"
#include "ju_synth_08_presets.hpp"

#include <cmath>

namespace noteahead {

JuSynth08DialogModel::JuSynth08DialogModel(std::shared_ptr<JuSynth08Device> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

JuSynth08DialogModel::~JuSynth08DialogModel() = default;

DeviceDialogModel::DeviceS JuSynth08DialogModel::device() const
{
    return m_device;
}

bool JuSynth08DialogModel::setDevice(DeviceS device)
{
    if (const auto synth = std::dynamic_pointer_cast<JuSynth08Device>(device)) {
        setDevice(synth);
        return true;
    }
    return false;
}

void JuSynth08DialogModel::setDevice(std::shared_ptr<JuSynth08Device> device)
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

QStringList JuSynth08DialogModel::factoryPresetNames() const
{
    QStringList names;
    for (auto && preset : JuSynth08Presets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void JuSynth08DialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

// LFO
int JuSynth08DialogModel::lfoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setLfoRate(int value)
{
    if (m_device) {
        m_device->setLfoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::lfoDelay() const
{
    return m_device ? static_cast<int>(std::round(m_device->lfoDelay() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setLfoDelay(int value)
{
    if (m_device) {
        m_device->setLfoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::lfoWaveform() const
{
    return m_device ? m_device->lfoWaveform() : 0;
}

void JuSynth08DialogModel::setLfoWaveform(int value)
{
    if (m_device) {
        m_device->setLfoWaveform(value);
    }
}

// VCO Mod & Pitch
int JuSynth08DialogModel::pitchLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setPitchLfoDepth(int value)
{
    if (m_device) {
        m_device->setPitchLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::pitchEnvDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchEnvDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setPitchEnvDepth(int value)
{
    if (m_device) {
        m_device->setPitchEnvDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::pitchEnvTarget() const
{
    return m_device ? m_device->pitchEnvTarget() : 2;
}

void JuSynth08DialogModel::setPitchEnvTarget(int value)
{
    if (m_device) {
        m_device->setPitchEnvTarget(value);
    }
}

int JuSynth08DialogModel::pulseWidth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pulseWidth() * Constants::uiInternalScaling())) : 500;
}

void JuSynth08DialogModel::setPulseWidth(int value)
{
    if (m_device) {
        m_device->setPulseWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::pwmSource() const
{
    return m_device ? m_device->pwmSource() : 0;
}

void JuSynth08DialogModel::setPwmSource(int value)
{
    if (m_device) {
        m_device->setPwmSource(value);
    }
}

int JuSynth08DialogModel::crossMod() const
{
    return m_device ? static_cast<int>(std::round(m_device->crossMod() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setCrossMod(int value)
{
    if (m_device) {
        m_device->setCrossMod(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// VCO 1
int JuSynth08DialogModel::vco1Waveform() const
{
    return m_device ? m_device->vco1Waveform() : 1;
}

void JuSynth08DialogModel::setVco1Waveform(int value)
{
    if (m_device) {
        m_device->setVco1Waveform(value);
    }
}

int JuSynth08DialogModel::vco1Range() const
{
    return m_device ? m_device->vco1Range() : 1;
}

void JuSynth08DialogModel::setVco1Range(int value)
{
    if (m_device) {
        m_device->setVco1Range(value);
    }
}

int JuSynth08DialogModel::vco1Level() const
{
    return m_device ? static_cast<int>(std::round(m_device->vco1Level() * Constants::uiInternalScaling())) : 800;
}

void JuSynth08DialogModel::setVco1Level(int value)
{
    if (m_device) {
        m_device->setVco1Level(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// VCO 2
int JuSynth08DialogModel::vco2Waveform() const
{
    return m_device ? m_device->vco2Waveform() : 0;
}

void JuSynth08DialogModel::setVco2Waveform(int value)
{
    if (m_device) {
        m_device->setVco2Waveform(value);
    }
}

int JuSynth08DialogModel::vco2Range() const
{
    return m_device ? m_device->vco2Range() : 1;
}

void JuSynth08DialogModel::setVco2Range(int value)
{
    if (m_device) {
        m_device->setVco2Range(value);
    }
}

int JuSynth08DialogModel::vco2FineTune() const
{
    return m_device ? static_cast<int>(std::round(m_device->vco2FineTune() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setVco2FineTune(int value)
{
    if (m_device) {
        m_device->setVco2FineTune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool JuSynth08DialogModel::sync() const
{
    return m_device && m_device->sync();
}

void JuSynth08DialogModel::setSync(bool value)
{
    if (m_device) {
        m_device->setSync(value);
    }
}

int JuSynth08DialogModel::vco2Level() const
{
    return m_device ? static_cast<int>(std::round(m_device->vco2Level() * Constants::uiInternalScaling())) : 800;
}

void JuSynth08DialogModel::setVco2Level(int value)
{
    if (m_device) {
        m_device->setVco2Level(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// Filter & Noise
int JuSynth08DialogModel::noiseLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->noiseLevel() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setNoiseLevel(int value)
{
    if (m_device) {
        m_device->setNoiseLevel(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::vcfSlope() const
{
    return m_device ? m_device->vcfSlope() : 1;
}

void JuSynth08DialogModel::setVcfSlope(int value)
{
    if (m_device) {
        m_device->setVcfSlope(value);
    }
}

int JuSynth08DialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 1000;
}

void JuSynth08DialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::lpfResonance() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setLpfResonance(int value)
{
    if (m_device) {
        m_device->setLpfResonance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::envMod() const
{
    return m_device ? static_cast<int>(std::round(m_device->envMod() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setEnvMod(int value)
{
    if (m_device) {
        m_device->setEnvMod(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool JuSynth08DialogModel::envPolarity() const
{
    return m_device && m_device->envPolarity();
}

void JuSynth08DialogModel::setEnvPolarity(bool value)
{
    if (m_device) {
        m_device->setEnvPolarity(value);
    }
}

int JuSynth08DialogModel::filterLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->filterLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setFilterLfoDepth(int value)
{
    if (m_device) {
        m_device->setFilterLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::keyTrack() const
{
    return m_device ? static_cast<int>(std::round(m_device->keyTrack() * Constants::uiInternalScaling())) : 1000;
}

void JuSynth08DialogModel::setKeyTrack(int value)
{
    if (m_device) {
        m_device->setKeyTrack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// VCA
int JuSynth08DialogModel::vcaMode() const
{
    return m_device ? m_device->vcaMode() : 0;
}

void JuSynth08DialogModel::setVcaMode(int value)
{
    if (m_device) {
        m_device->setVcaMode(value);
    }
}

int JuSynth08DialogModel::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::vcaLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vcaLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setVcaLfoDepth(int value)
{
    if (m_device) {
        m_device->setVcaLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// ENV 1
int JuSynth08DialogModel::env1Attack() const
{
    return m_device ? static_cast<int>(std::round(m_device->env1Attack() * Constants::uiInternalScaling())) : 10;
}

void JuSynth08DialogModel::setEnv1Attack(int value)
{
    if (m_device) {
        m_device->setEnv1Attack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::env1Decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->env1Decay() * Constants::uiInternalScaling())) : 300;
}

void JuSynth08DialogModel::setEnv1Decay(int value)
{
    if (m_device) {
        m_device->setEnv1Decay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::env1Sustain() const
{
    return m_device ? static_cast<int>(std::round(m_device->env1Sustain() * Constants::uiInternalScaling())) : 500;
}

void JuSynth08DialogModel::setEnv1Sustain(int value)
{
    if (m_device) {
        m_device->setEnv1Sustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::env1Release() const
{
    return m_device ? static_cast<int>(std::round(m_device->env1Release() * Constants::uiInternalScaling())) : 200;
}

void JuSynth08DialogModel::setEnv1Release(int value)
{
    if (m_device) {
        m_device->setEnv1Release(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool JuSynth08DialogModel::env1KeyFollow() const
{
    return m_device && m_device->env1KeyFollow();
}

void JuSynth08DialogModel::setEnv1KeyFollow(bool value)
{
    if (m_device) {
        m_device->setEnv1KeyFollow(value);
    }
}

// ENV 2
int JuSynth08DialogModel::env2Attack() const
{
    return m_device ? static_cast<int>(std::round(m_device->env2Attack() * Constants::uiInternalScaling())) : 10;
}

void JuSynth08DialogModel::setEnv2Attack(int value)
{
    if (m_device) {
        m_device->setEnv2Attack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::env2Decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->env2Decay() * Constants::uiInternalScaling())) : 300;
}

void JuSynth08DialogModel::setEnv2Decay(int value)
{
    if (m_device) {
        m_device->setEnv2Decay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::env2Sustain() const
{
    return m_device ? static_cast<int>(std::round(m_device->env2Sustain() * Constants::uiInternalScaling())) : 1000;
}

void JuSynth08DialogModel::setEnv2Sustain(int value)
{
    if (m_device) {
        m_device->setEnv2Sustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::env2Release() const
{
    return m_device ? static_cast<int>(std::round(m_device->env2Release() * Constants::uiInternalScaling())) : 200;
}

void JuSynth08DialogModel::setEnv2Release(int value)
{
    if (m_device) {
        m_device->setEnv2Release(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool JuSynth08DialogModel::env2KeyFollow() const
{
    return m_device && m_device->env2KeyFollow();
}

void JuSynth08DialogModel::setEnv2KeyFollow(bool value)
{
    if (m_device) {
        m_device->setEnv2KeyFollow(value);
    }
}

// Delay
int JuSynth08DialogModel::delayTime() const
{
    return m_device ? static_cast<int>(std::round(m_device->delayTime() * Constants::uiInternalScaling())) : 300;
}

void JuSynth08DialogModel::setDelayTime(int value)
{
    if (m_device) {
        m_device->setDelayTime(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::delayFeedback() const
{
    return m_device ? static_cast<int>(std::round(m_device->delayFeedback() * Constants::uiInternalScaling())) : 300;
}

void JuSynth08DialogModel::setDelayFeedback(int value)
{
    if (m_device) {
        m_device->setDelayFeedback(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::delayLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->delayLevel() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setDelayLevel(int value)
{
    if (m_device) {
        m_device->setDelayLevel(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// Global
int JuSynth08DialogModel::voiceMode() const
{
    return m_device ? m_device->voiceMode() : 0;
}

void JuSynth08DialogModel::setVoiceMode(int value)
{
    if (m_device) {
        m_device->setVoiceMode(value);
    }
}

int JuSynth08DialogModel::unisonDetune() const
{
    return m_device ? static_cast<int>(std::round(m_device->unisonDetune() * Constants::uiInternalScaling())) : 200;
}

void JuSynth08DialogModel::setUnisonDetune(int value)
{
    if (m_device) {
        m_device->setUnisonDetune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::portamento() const
{
    return m_device ? static_cast<int>(std::round(m_device->portamento() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setPortamento(int value)
{
    if (m_device) {
        m_device->setPortamento(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::pitchBendRange() const
{
    return m_device ? m_device->pitchBendRange() : 2;
}

void JuSynth08DialogModel::setPitchBendRange(int value)
{
    if (m_device) {
        m_device->setPitchBendRange(value);
        emit pitchBendRangeChanged();
    }
}

int JuSynth08DialogModel::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::oscillatorDrift() const
{
    return m_device ? static_cast<int>(std::round(m_device->oscillatorDrift() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setOscillatorDrift(int value)
{
    if (m_device) {
        m_device->setOscillatorDrift(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::oscillatorInstability() const
{
    return m_device ? static_cast<int>(std::round(m_device->oscillatorInstability() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setOscillatorInstability(int value)
{
    if (m_device) {
        m_device->setOscillatorInstability(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08DialogModel::voiceTolerance() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceTolerance() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08DialogModel::setVoiceTolerance(int value)
{
    if (m_device) {
        m_device->setVoiceTolerance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void JuSynth08DialogModel::requestSettings()
{
    emit lfoRateChanged();
    emit lfoDelayChanged();
    emit lfoWaveformChanged();

    emit pitchLfoDepthChanged();
    emit pitchEnvDepthChanged();
    emit pitchEnvTargetChanged();
    emit pulseWidthChanged();
    emit pwmSourceChanged();
    emit crossModChanged();

    emit vco1WaveformChanged();
    emit vco1RangeChanged();
    emit vco1LevelChanged();

    emit vco2WaveformChanged();
    emit vco2RangeChanged();
    emit vco2FineTuneChanged();
    emit syncChanged();
    emit vco2LevelChanged();

    emit noiseLevelChanged();
    emit hpfCutoffChanged();

    emit vcfSlopeChanged();
    emit lpfCutoffChanged();
    emit lpfResonanceChanged();
    emit envModChanged();
    emit envPolarityChanged();
    emit filterLfoDepthChanged();
    emit keyTrackChanged();

    emit vcaModeChanged();
    emit velocitySensitivityChanged();
    emit vcaLfoDepthChanged();

    emit env1AttackChanged();
    emit env1DecayChanged();
    emit env1SustainChanged();
    emit env1ReleaseChanged();
    emit env1KeyFollowChanged();

    emit env2AttackChanged();
    emit env2DecayChanged();
    emit env2SustainChanged();
    emit env2ReleaseChanged();
    emit env2KeyFollowChanged();

    emit delayTimeChanged();
    emit delayFeedbackChanged();
    emit delayLevelChanged();

    emit voiceModeChanged();
    emit unisonDetuneChanged();
    emit portamentoChanged();
    emit pitchBendRangeChanged();
    emit panSpreadChanged();
    emit oscillatorDriftChanged();
    emit oscillatorInstabilityChanged();
    emit voiceToleranceChanged();

    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
