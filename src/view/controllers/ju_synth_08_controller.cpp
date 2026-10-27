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

#include "ju_synth_08_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/ju_synth_08_device.hpp"
#include "../../domain/devices/ju_synth_08_presets.hpp"

#include <cmath>

namespace noteahead {

JuSynth08Controller::JuSynth08Controller(std::shared_ptr<JuSynth08Device> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

JuSynth08Controller::~JuSynth08Controller() = default;

DeviceController::DeviceS JuSynth08Controller::device() const
{
    return m_device;
}

bool JuSynth08Controller::setDevice(DeviceS device)
{
    if (const auto synth = std::dynamic_pointer_cast<JuSynth08Device>(device)) {
        setDevice(synth);
        return true;
    }
    return false;
}

void JuSynth08Controller::setDevice(std::shared_ptr<JuSynth08Device> device)
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

QStringList JuSynth08Controller::factoryPresetNames() const
{
    QStringList names;
    for (auto && preset : JuSynth08Presets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void JuSynth08Controller::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

// LFO
int JuSynth08Controller::lfoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setLfoRate(int value)
{
    if (m_device) {
        m_device->setLfoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::lfoDelay() const
{
    return m_device ? static_cast<int>(std::round(m_device->lfoDelay() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setLfoDelay(int value)
{
    if (m_device) {
        m_device->setLfoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::lfoWaveform() const
{
    return m_device ? m_device->lfoWaveform() : 0;
}

void JuSynth08Controller::setLfoWaveform(int value)
{
    if (m_device) {
        m_device->setLfoWaveform(value);
    }
}

// VCO Mod & Pitch
int JuSynth08Controller::pitchLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setPitchLfoDepth(int value)
{
    if (m_device) {
        m_device->setPitchLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::pitchEnvDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchEnvDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setPitchEnvDepth(int value)
{
    if (m_device) {
        m_device->setPitchEnvDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::pitchEnvTarget() const
{
    return m_device ? m_device->pitchEnvTarget() : 2;
}

void JuSynth08Controller::setPitchEnvTarget(int value)
{
    if (m_device) {
        m_device->setPitchEnvTarget(value);
    }
}

int JuSynth08Controller::pulseWidth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pulseWidth() * Constants::uiInternalScaling())) : 500;
}

void JuSynth08Controller::setPulseWidth(int value)
{
    if (m_device) {
        m_device->setPulseWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::pwmSource() const
{
    return m_device ? m_device->pwmSource() : 0;
}

void JuSynth08Controller::setPwmSource(int value)
{
    if (m_device) {
        m_device->setPwmSource(value);
    }
}

int JuSynth08Controller::crossMod() const
{
    return m_device ? static_cast<int>(std::round(m_device->crossMod() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setCrossMod(int value)
{
    if (m_device) {
        m_device->setCrossMod(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// VCO 1
int JuSynth08Controller::vco1Waveform() const
{
    return m_device ? m_device->vco1Waveform() : 1;
}

void JuSynth08Controller::setVco1Waveform(int value)
{
    if (m_device) {
        m_device->setVco1Waveform(value);
    }
}

int JuSynth08Controller::vco1Range() const
{
    return m_device ? m_device->vco1Range() : 1;
}

void JuSynth08Controller::setVco1Range(int value)
{
    if (m_device) {
        m_device->setVco1Range(value);
    }
}

int JuSynth08Controller::vco1Level() const
{
    return m_device ? static_cast<int>(std::round(m_device->vco1Level() * Constants::uiInternalScaling())) : 800;
}

void JuSynth08Controller::setVco1Level(int value)
{
    if (m_device) {
        m_device->setVco1Level(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// VCO 2
int JuSynth08Controller::vco2Waveform() const
{
    return m_device ? m_device->vco2Waveform() : 0;
}

void JuSynth08Controller::setVco2Waveform(int value)
{
    if (m_device) {
        m_device->setVco2Waveform(value);
    }
}

int JuSynth08Controller::vco2Range() const
{
    return m_device ? m_device->vco2Range() : 1;
}

void JuSynth08Controller::setVco2Range(int value)
{
    if (m_device) {
        m_device->setVco2Range(value);
    }
}

int JuSynth08Controller::vco2FineTune() const
{
    return m_device ? static_cast<int>(std::round(m_device->vco2FineTune() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setVco2FineTune(int value)
{
    if (m_device) {
        m_device->setVco2FineTune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool JuSynth08Controller::sync() const
{
    return m_device && m_device->sync();
}

void JuSynth08Controller::setSync(bool value)
{
    if (m_device) {
        m_device->setSync(value);
    }
}

int JuSynth08Controller::vco2Level() const
{
    return m_device ? static_cast<int>(std::round(m_device->vco2Level() * Constants::uiInternalScaling())) : 800;
}

void JuSynth08Controller::setVco2Level(int value)
{
    if (m_device) {
        m_device->setVco2Level(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// Filter & Noise
int JuSynth08Controller::noiseLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->noiseLevel() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setNoiseLevel(int value)
{
    if (m_device) {
        m_device->setNoiseLevel(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::vcfSlope() const
{
    return m_device ? m_device->vcfSlope() : 1;
}

void JuSynth08Controller::setVcfSlope(int value)
{
    if (m_device) {
        m_device->setVcfSlope(value);
    }
}

int JuSynth08Controller::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 1000;
}

void JuSynth08Controller::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::lpfResonance() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setLpfResonance(int value)
{
    if (m_device) {
        m_device->setLpfResonance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::envMod() const
{
    return m_device ? static_cast<int>(std::round(m_device->envMod() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setEnvMod(int value)
{
    if (m_device) {
        m_device->setEnvMod(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool JuSynth08Controller::envPolarity() const
{
    return m_device && m_device->envPolarity();
}

void JuSynth08Controller::setEnvPolarity(bool value)
{
    if (m_device) {
        m_device->setEnvPolarity(value);
    }
}

int JuSynth08Controller::filterLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->filterLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setFilterLfoDepth(int value)
{
    if (m_device) {
        m_device->setFilterLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::keyTrack() const
{
    return m_device ? static_cast<int>(std::round(m_device->keyTrack() * Constants::uiInternalScaling())) : 1000;
}

void JuSynth08Controller::setKeyTrack(int value)
{
    if (m_device) {
        m_device->setKeyTrack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// VCA
int JuSynth08Controller::vcaMode() const
{
    return m_device ? m_device->vcaMode() : 0;
}

void JuSynth08Controller::setVcaMode(int value)
{
    if (m_device) {
        m_device->setVcaMode(value);
    }
}

int JuSynth08Controller::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::vcaLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->vcaLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setVcaLfoDepth(int value)
{
    if (m_device) {
        m_device->setVcaLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// ENV 1
int JuSynth08Controller::env1Attack() const
{
    return m_device ? static_cast<int>(std::round(m_device->env1Attack() * Constants::uiInternalScaling())) : 10;
}

void JuSynth08Controller::setEnv1Attack(int value)
{
    if (m_device) {
        m_device->setEnv1Attack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::env1Decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->env1Decay() * Constants::uiInternalScaling())) : 300;
}

void JuSynth08Controller::setEnv1Decay(int value)
{
    if (m_device) {
        m_device->setEnv1Decay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::env1Sustain() const
{
    return m_device ? static_cast<int>(std::round(m_device->env1Sustain() * Constants::uiInternalScaling())) : 500;
}

void JuSynth08Controller::setEnv1Sustain(int value)
{
    if (m_device) {
        m_device->setEnv1Sustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::env1Release() const
{
    return m_device ? static_cast<int>(std::round(m_device->env1Release() * Constants::uiInternalScaling())) : 200;
}

void JuSynth08Controller::setEnv1Release(int value)
{
    if (m_device) {
        m_device->setEnv1Release(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool JuSynth08Controller::env1KeyFollow() const
{
    return m_device && m_device->env1KeyFollow();
}

void JuSynth08Controller::setEnv1KeyFollow(bool value)
{
    if (m_device) {
        m_device->setEnv1KeyFollow(value);
    }
}

// ENV 2
int JuSynth08Controller::env2Attack() const
{
    return m_device ? static_cast<int>(std::round(m_device->env2Attack() * Constants::uiInternalScaling())) : 10;
}

void JuSynth08Controller::setEnv2Attack(int value)
{
    if (m_device) {
        m_device->setEnv2Attack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::env2Decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->env2Decay() * Constants::uiInternalScaling())) : 300;
}

void JuSynth08Controller::setEnv2Decay(int value)
{
    if (m_device) {
        m_device->setEnv2Decay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::env2Sustain() const
{
    return m_device ? static_cast<int>(std::round(m_device->env2Sustain() * Constants::uiInternalScaling())) : 1000;
}

void JuSynth08Controller::setEnv2Sustain(int value)
{
    if (m_device) {
        m_device->setEnv2Sustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::env2Release() const
{
    return m_device ? static_cast<int>(std::round(m_device->env2Release() * Constants::uiInternalScaling())) : 200;
}

void JuSynth08Controller::setEnv2Release(int value)
{
    if (m_device) {
        m_device->setEnv2Release(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool JuSynth08Controller::env2KeyFollow() const
{
    return m_device && m_device->env2KeyFollow();
}

void JuSynth08Controller::setEnv2KeyFollow(bool value)
{
    if (m_device) {
        m_device->setEnv2KeyFollow(value);
    }
}

// Delay
int JuSynth08Controller::delayTime() const
{
    return m_device ? static_cast<int>(std::round(m_device->delayTime() * Constants::uiInternalScaling())) : 300;
}

void JuSynth08Controller::setDelayTime(int value)
{
    if (m_device) {
        m_device->setDelayTime(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::delayFeedback() const
{
    return m_device ? static_cast<int>(std::round(m_device->delayFeedback() * Constants::uiInternalScaling())) : 300;
}

void JuSynth08Controller::setDelayFeedback(int value)
{
    if (m_device) {
        m_device->setDelayFeedback(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::delayLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->delayLevel() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setDelayLevel(int value)
{
    if (m_device) {
        m_device->setDelayLevel(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// Global
int JuSynth08Controller::voiceMode() const
{
    return m_device ? m_device->voiceMode() : 0;
}

void JuSynth08Controller::setVoiceMode(int value)
{
    if (m_device) {
        m_device->setVoiceMode(value);
    }
}

int JuSynth08Controller::unisonDetune() const
{
    return m_device ? static_cast<int>(std::round(m_device->unisonDetune() * Constants::uiInternalScaling())) : 200;
}

void JuSynth08Controller::setUnisonDetune(int value)
{
    if (m_device) {
        m_device->setUnisonDetune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::portamento() const
{
    return m_device ? static_cast<int>(std::round(m_device->portamento() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setPortamento(int value)
{
    if (m_device) {
        m_device->setPortamento(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::pitchBendRange() const
{
    return m_device ? m_device->pitchBendRange() : 2;
}

void JuSynth08Controller::setPitchBendRange(int value)
{
    if (m_device) {
        m_device->setPitchBendRange(value);
        emit pitchBendRangeChanged();
    }
}

int JuSynth08Controller::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::oscillatorDrift() const
{
    return m_device ? static_cast<int>(std::round(m_device->oscillatorDrift() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setOscillatorDrift(int value)
{
    if (m_device) {
        m_device->setOscillatorDrift(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::oscillatorInstability() const
{
    return m_device ? static_cast<int>(std::round(m_device->oscillatorInstability() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setOscillatorInstability(int value)
{
    if (m_device) {
        m_device->setOscillatorInstability(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynth08Controller::voiceTolerance() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceTolerance() * Constants::uiInternalScaling())) : 0;
}

void JuSynth08Controller::setVoiceTolerance(int value)
{
    if (m_device) {
        m_device->setVoiceTolerance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void JuSynth08Controller::requestSettings()
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
