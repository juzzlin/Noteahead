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

#include "synth_controller.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/synth_presets.hpp"
#include "device_service.hpp"

#include <cmath>

namespace noteahead {

SynthController::SynthController(std::shared_ptr<SynthDevice> synth, QObject * parent)
  : QObject { parent }
  , m_synth { std::move(synth) }
{
    if (m_synth) {
        connect(m_synth.get(), &Device::dataChanged, this, &SynthController::sampleRateChanged);
    }

    for (int i = 0; i < 128; ++i) {
        m_userPresets[i] = SynthPresets::initPreset();
    }
}

SynthController::~SynthController() = default;

std::shared_ptr<SynthDevice> SynthController::synth() const { return m_synth; }

// VCO1
int SynthController::vco1Waveform() const { return m_synth ? static_cast<int>(m_synth->vco1Waveform()) : 0; }
void SynthController::setVco1Waveform(int wave) { if (m_synth) { m_synth->setVco1Waveform(static_cast<PolyBLEPOscillator::Waveform>(wave)); emit vco1WaveformChanged(); } }
int SynthController::vco1Octave() const { return m_synth ? m_synth->vco1Octave() : 0; }
void SynthController::setVco1Octave(int oct) { if (m_synth) { m_synth->setVco1Octave(oct); emit vco1OctaveChanged(); } }
int SynthController::vco1Pitch() const { return m_synth ? m_synth->vco1Pitch() : 0; }
void SynthController::setVco1Pitch(int p) { if (m_synth) { m_synth->setVco1Pitch(p); emit vco1PitchChanged(); } }
int SynthController::vco1Shape() const { return m_synth ? static_cast<int>(std::round(m_synth->vco1Shape() * 1000.0f)) : 0; }
void SynthController::setVco1Shape(int s) { if (m_synth) { m_synth->setVco1Shape(s / 1000.0f); emit vco1ShapeChanged(); } }
bool SynthController::vco1Sync() const { return m_synth ? m_synth->vco1Sync() : false; }
void SynthController::setVco1Sync(bool s) { if (m_synth) { m_synth->setVco1Sync(s); emit vco1SyncChanged(); } }

// VCO2
int SynthController::vco2Waveform() const { return m_synth ? static_cast<int>(m_synth->vco2Waveform()) : 0; }
void SynthController::setVco2Waveform(int wave) { if (m_synth) { m_synth->setVco2Waveform(static_cast<PolyBLEPOscillator::Waveform>(wave)); emit vco2WaveformChanged(); } }
int SynthController::vco2Octave() const { return m_synth ? m_synth->vco2Octave() : 0; }
void SynthController::setVco2Octave(int oct) { if (m_synth) { m_synth->setVco2Octave(oct); emit vco2OctaveChanged(); } }
int SynthController::vco2Pitch() const { return m_synth ? m_synth->vco2Pitch() : 0; }
void SynthController::setVco2Pitch(int p) { if (m_synth) { m_synth->setVco2Pitch(p); emit vco2PitchChanged(); } }
int SynthController::vco2Shape() const { return m_synth ? static_cast<int>(std::round(m_synth->vco2Shape() * 1000.0f)) : 0; }
void SynthController::setVco2Shape(int s) { if (m_synth) { m_synth->setVco2Shape(s / 1000.0f); emit vco2ShapeChanged(); } }
bool SynthController::vco2Sync() const { return m_synth ? m_synth->vco2Sync() : false; }
void SynthController::setVco2Sync(bool s) { if (m_synth) { m_synth->setVco2Sync(s); emit vco2SyncChanged(); } }

// Multi Engine
int SynthController::multiType() const { return m_synth ? static_cast<int>(m_synth->multiType()) : 0; }
void SynthController::setMultiType(int type) { if (m_synth) { m_synth->setMultiType(static_cast<MultiEngine::Type>(type)); emit multiTypeChanged(); } }
int SynthController::multiShape() const { return m_synth ? static_cast<int>(std::round(m_synth->multiShape() * 1000.0f)) : 0; }
void SynthController::setMultiShape(int s) { if (m_synth) { m_synth->setMultiShape(s / 1000.0f); emit multiShapeChanged(); } }
int SynthController::multiLevel() const { return m_synth ? static_cast<int>(std::round(m_synth->multiLevel() * 1000.0f)) : 0; }
void SynthController::setMultiLevel(int lvl) { if (m_synth) { m_synth->setMultiLevel(lvl / 1000.0f); emit multiLevelChanged(); } }
int SynthController::multiKeyTrack() const { return m_synth ? static_cast<int>(std::round(m_synth->multiKeyTrack() * 1000.0f)) : 0; }
void SynthController::setMultiKeyTrack(int t) { if (m_synth) { m_synth->setMultiKeyTrack(t / 1000.0f); emit multiKeyTrackChanged(); } }

// Mixer
int SynthController::mixVco1() const { return m_synth ? static_cast<int>(std::round(m_synth->mixVco1() * 1000.0f)) : 0; }
void SynthController::setMixVco1(int lvl) { if (m_synth) { m_synth->setMixVco1(lvl / 1000.0f); emit mixVco1Changed(); } }
int SynthController::mixVco2() const { return m_synth ? static_cast<int>(std::round(m_synth->mixVco2() * 1000.0f)) : 0; }
void SynthController::setMixVco2(int lvl) { if (m_synth) { m_synth->setMixVco2(lvl / 1000.0f); emit mixVco2Changed(); } }

// Filter
int SynthController::lpfCutoff() const { return m_synth ? static_cast<int>(std::round(m_synth->lpfCutoff() * 1000.0f)) : 0; }
void SynthController::setLpfCutoff(int c) { if (m_synth) { m_synth->setLpfCutoff(c / 1000.0f); emit lpfCutoffChanged(); } }
int SynthController::lpfResonance() const { return m_synth ? static_cast<int>(std::round(m_synth->lpfResonance() * 1000.0f)) : 0; }
void SynthController::setLpfResonance(int r) { if (m_synth) { m_synth->setLpfResonance(r / 1000.0f); emit lpfResonanceChanged(); } }
int SynthController::hpfCutoff() const { return m_synth ? static_cast<int>(std::round(m_synth->hpfCutoff() * 1000.0f)) : 0; }
void SynthController::setHpfCutoff(int c) { if (m_synth) { m_synth->setHpfCutoff(c / 1000.0f); emit hpfCutoffChanged(); } }
int SynthController::filterKeyTrack() const { return m_synth ? static_cast<int>(std::round(m_synth->filterKeyTrack() * 1000.0f)) : 0; }
void SynthController::setFilterKeyTrack(int t) { if (m_synth) { m_synth->setFilterKeyTrack(t / 1000.0f); emit filterKeyTrackChanged(); } }

// Amp EG
int SynthController::ampAttack() const { return m_synth ? static_cast<int>(std::round(m_synth->ampAttack() * 1000.0f)) : 0; }
void SynthController::setAmpAttack(int a) { if (m_synth) { m_synth->setAmpAttack(a / 1000.0f); emit ampAttackChanged(); } }
int SynthController::ampDecay() const { return m_synth ? static_cast<int>(std::round(m_synth->ampDecay() * 1000.0f)) : 0; }
void SynthController::setAmpDecay(int d) { if (m_synth) { m_synth->setAmpDecay(d / 1000.0f); emit ampDecayChanged(); } }
int SynthController::ampSustain() const { return m_synth ? static_cast<int>(std::round(m_synth->ampSustain() * 1000.0f)) : 0; }
void SynthController::setAmpSustain(int s) { if (m_synth) { m_synth->setAmpSustain(s / 1000.0f); emit ampSustainChanged(); } }
int SynthController::ampRelease() const { return m_synth ? static_cast<int>(std::round(m_synth->ampRelease() * 1000.0f)) : 0; }
void SynthController::setAmpRelease(int r) { if (m_synth) { m_synth->setAmpRelease(r / 1000.0f); emit ampReleaseChanged(); } }

// Mod EG
int SynthController::modAttack() const { return m_synth ? static_cast<int>(std::round(m_synth->modAttack() * 1000.0f)) : 0; }
void SynthController::setModAttack(int a) { if (m_synth) { m_synth->setModAttack(a / 1000.0f); emit modAttackChanged(); } }
int SynthController::modDecay() const { return m_synth ? static_cast<int>(std::round(m_synth->modDecay() * 1000.0f)) : 0; }
void SynthController::setModDecay(int d) { if (m_synth) { m_synth->setModDecay(d / 1000.0f); emit modDecayChanged(); } }
int SynthController::modInt() const { return m_synth ? static_cast<int>(std::round(m_synth->modInt() * 1000.0f)) : 0; }
void SynthController::setModInt(int i) { if (m_synth) { m_synth->setModInt(i / 1000.0f); emit modIntChanged(); } }
int SynthController::modTarget() const { return m_synth ? static_cast<int>(m_synth->modTarget()) : 0; }
void SynthController::setModTarget(int t) { if (m_synth) { m_synth->setModTarget(static_cast<SynthDevice::ModTarget>(t)); emit modTargetChanged(); } }

// LFO
int SynthController::lfoWaveform() const { return m_synth ? static_cast<int>(m_synth->lfoWaveform()) : 0; }
void SynthController::setLfoWaveform(int wave) { if (m_synth) { m_synth->setLfoWaveform(static_cast<LFO::Waveform>(wave)); emit lfoWaveformChanged(); } }
int SynthController::lfoMode() const { return m_synth ? static_cast<int>(m_synth->lfoMode()) : 0; }
void SynthController::setLfoMode(int mode) { if (m_synth) { m_synth->setLfoMode(static_cast<LFO::Mode>(mode)); emit lfoModeChanged(); } }
int SynthController::lfoRate() const { return m_synth ? static_cast<int>(std::round(m_synth->lfoRate() * 1000.0f)) : 0; }
void SynthController::setLfoRate(int rate) { if (m_synth) { m_synth->setLfoRate(rate / 1000.0f); emit lfoRateChanged(); } }
int SynthController::lfoInt() const { return m_synth ? static_cast<int>(std::round(m_synth->lfoInt() * 1000.0f)) : 0; }
void SynthController::setLfoInt(int intensity) { if (m_synth) { m_synth->setLfoInt(intensity / 1000.0f); emit lfoIntChanged(); } }
int SynthController::lfoTarget() const { return m_synth ? static_cast<int>(m_synth->lfoTarget()) : 0; }
void SynthController::setLfoTarget(int target) { if (m_synth) { m_synth->setLfoTarget(static_cast<SynthDevice::LfoTarget>(target)); emit lfoTargetChanged(); } }

// Global
int SynthController::voiceMode() const { return m_synth ? static_cast<int>(m_synth->voiceMode()) : 0; }
void SynthController::setVoiceMode(int m) { if (m_synth) { m_synth->setVoiceMode(static_cast<SynthDevice::VoiceMode>(m)); emit voiceModeChanged(); } }
int SynthController::voiceDepth() const { return m_synth ? static_cast<int>(std::round(m_synth->voiceDepth() * 1000.0f)) : 0; }
void SynthController::setVoiceDepth(int d) { if (m_synth) { m_synth->setVoiceDepth(d / 1000.0f); emit voiceDepthChanged(); } }
int SynthController::portamento() const { return m_synth ? static_cast<int>(std::round(m_synth->portamento() * 1000.0f)) : 0; }
void SynthController::setPortamento(int p) { if (m_synth) { m_synth->setPortamento(p / 1000.0f); emit portamentoChanged(); } }
int SynthController::panSpread() const { return m_synth ? static_cast<int>(std::round(m_synth->panSpread() * 1000.0f)) : 0; }
void SynthController::setPanSpread(int s) { if (m_synth) { m_synth->setPanSpread(s / 1000.0f); emit panSpreadChanged(); } }
int SynthController::masterPan() const { return m_synth ? static_cast<int>(std::round(m_synth->masterPan() * 1000.0f)) : 0; }
void SynthController::setMasterPan(int p) { if (m_synth) { m_synth->setMasterPan(p / 1000.0f); emit masterPanChanged(); } }
int SynthController::masterVolume() const { return m_synth ? static_cast<int>(std::round(m_synth->masterVolume() * 1000.0f)) : 0; }
void SynthController::setMasterVolume(int v) { if (m_synth) { m_synth->setMasterVolume(v / 1000.0f); emit masterVolumeChanged(); } }
int SynthController::gain() const { return m_synth ? static_cast<int>(std::round(m_synth->gain() * 1000.0f)) : 0; }
void SynthController::setGain(int g) { if (m_synth) { m_synth->setGain(g / 1000.0f); emit gainChanged(); } }

uint32_t SynthController::sampleRate() const { return m_synth ? m_synth->sampleRate() : static_cast<uint32_t>(Constants::defaultSampleRate()); }

float SynthController::cutoffToHz(float cutoff) const
{
    return Utils::Dsp::cutoffToHz(cutoff / 1000.0f, static_cast<float>(sampleRate()));
}

QStringList SynthController::presetNames() const
{
    QStringList names;
    const auto & presetList = SynthPresets::presets();
    for (size_t i = 0; i < presetList.size(); ++i) {
        names.append(QString { "%1: %2" }
                       .arg(static_cast<int>(i), 3, 10, QChar { '0' })
                       .arg(QString::fromStdString(presetList.at(i).name)));
    }
    return names;
}

int SynthController::currentBank() const
{
    return m_currentBank;
}

void SynthController::setCurrentBank(int bank)
{
    if (m_currentBank != bank) {
        m_currentBank = bank;
        emit currentBankChanged();
        setCurrentPresetIndex(0);
    }
}

int SynthController::currentPresetIndex() const
{
    return m_currentPresetIndex;
}

void SynthController::setCurrentPresetIndex(int index)
{
    if (m_currentPresetIndex != index) {
        m_currentPresetIndex = index;
        emit currentPresetIndexChanged();
    }
}

QStringList SynthController::userPresetNames() const
{
    QStringList names;
    for (int i = 0; i < 128; ++i) {
        names << QString { "%1: %2" }
                   .arg(i, 3, 10, QChar { '0' })
                   .arg(QString::fromStdString(m_userPresets.at(i).name));
    }
    return names;
}

void SynthController::setUserPresets(const UserPresets & presets)
{
    m_userPresets = presets;
    emit userPresetNamesChanged();
}

// Delay
int SynthController::delayType() const { return m_synth ? static_cast<int>(m_synth->delayType()) : 0; }
void SynthController::setDelayType(int type) { if (m_synth) { m_synth->setDelayType(static_cast<DelayEffect::Type>(type)); emit delayTypeChanged(); } }
int SynthController::delayTime() const { return m_synth ? static_cast<int>(std::round(m_synth->delayTime() * 1000.0f)) : 0; }
void SynthController::setDelayTime(int time) { if (m_synth) { m_synth->setDelayTime(time / 1000.0f); emit delayTimeChanged(); } }
int SynthController::delayFeedback() const { return m_synth ? static_cast<int>(std::round(m_synth->delayFeedback() * 1000.0f)) : 0; }
void SynthController::setDelayFeedback(int fb) { if (m_synth) { m_synth->setDelayFeedback(fb / 1000.0f); emit delayFeedbackChanged(); } }
int SynthController::delayDepth() const { return m_synth ? static_cast<int>(std::round(m_synth->delayDepth() * 1000.0f)) : 0; }
void SynthController::setDelayDepth(int d) { if (m_synth) { m_synth->setDelayDepth(d / 1000.0f); emit delayDepthChanged(); } }
int SynthController::delayMix() const { return m_synth ? static_cast<int>(std::round(m_synth->delayMix() * 1000.0f)) : 0; }
void SynthController::setDelayMix(int mix) { if (m_synth) { m_synth->setDelayMix(mix / 1000.0f); emit delayMixChanged(); } }
bool SynthController::delaySync() const { return m_synth ? m_synth->delaySync() : false; }
void SynthController::setDelaySync(bool sync) { if (m_synth) { m_synth->setDelaySync(sync); emit delaySyncChanged(); } }
int SynthController::delaySyncDivision() const { return m_synth ? static_cast<int>(std::round(m_synth->delaySyncDivision() * 1000.0f)) : 0; }
void SynthController::setDelaySyncDivision(int div) { if (m_synth) { m_synth->setDelaySyncDivision(div / 1000.0f); emit delaySyncDivisionChanged(); } }
int SynthController::delayFeedbackLpf() const { return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackLpf() * 1000.0f)) : 0; }
void SynthController::setDelayFeedbackLpf(int cutoff) { if (m_synth) { m_synth->setFeedbackLpf(cutoff / 1000.0f); emit delayFeedbackLpfChanged(); } }
int SynthController::delayFeedbackHpf() const { return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackHpf() * 1000.0f)) : 0; }
void SynthController::setDelayFeedbackHpf(int cutoff) { if (m_synth) { m_synth->setFeedbackHpf(cutoff / 1000.0f); emit delayFeedbackHpfChanged(); } }

void SynthController::initialize() {}
void SynthController::reset() { if (m_synth) m_synth->reset(); requestSettings(); }
void SynthController::requestSettings() {
    emit vco1WaveformChanged(); emit vco1OctaveChanged(); emit vco1PitchChanged(); emit vco1ShapeChanged(); emit vco1SyncChanged();
    emit vco2WaveformChanged(); emit vco2OctaveChanged(); emit vco2PitchChanged(); emit vco2ShapeChanged(); emit vco2SyncChanged();
    emit multiTypeChanged(); emit multiShapeChanged(); emit multiLevelChanged(); emit multiKeyTrackChanged();
    emit mixVco1Changed(); emit mixVco2Changed();
    emit lpfCutoffChanged(); emit lpfResonanceChanged(); emit hpfCutoffChanged(); emit filterKeyTrackChanged();
    emit ampAttackChanged(); emit ampDecayChanged(); emit ampSustainChanged(); emit ampReleaseChanged();
    emit modAttackChanged(); emit modDecayChanged(); emit modIntChanged(); emit modTargetChanged();
    emit lfoWaveformChanged(); emit lfoModeChanged(); emit lfoRateChanged(); emit lfoIntChanged(); emit lfoTargetChanged();
    emit voiceModeChanged(); emit voiceDepthChanged(); emit portamentoChanged(); emit panSpreadChanged(); emit masterPanChanged(); emit masterVolumeChanged(); emit gainChanged();
    emit delayTypeChanged(); emit delayTimeChanged(); emit delayFeedbackChanged(); emit delayDepthChanged(); emit delayMixChanged(); emit delaySyncChanged(); emit delaySyncDivisionChanged(); emit delayFeedbackLpfChanged(); emit delayFeedbackHpfChanged();
}
void SynthController::accept() {}
void SynthController::reject() {}
void SynthController::loadPreset(int index)
{
    if (m_synth) {
        m_synth->loadPreset(m_currentBank, index);
        requestSettings();
    }
}

void SynthController::saveUserPreset(QString name)
{
    if (m_synth && m_deviceService) {
        SynthPreset preset;
        preset.name = name.toStdString();
        for (const auto & [paramName, parameter] : m_synth->parameters()) {
            preset.parameters[paramName] = parameter.value();
        }
        m_deviceService->saveSynthUserPreset(m_currentPresetIndex, preset);
    }
}

void SynthController::setDeviceService(std::shared_ptr<DeviceService> deviceService)
{
    m_deviceService = std::move(deviceService);
}

void SynthController::playNote(int note, double velocity) { if (m_synth) m_synth->processMidiNoteOn(note, static_cast<uint8_t>(velocity * 127.0)); }
void SynthController::stopNote(int note) { if (m_synth) m_synth->processMidiNoteOff(note); }

void SynthController::setSynth(std::shared_ptr<SynthDevice> synth)
{
    if (m_synth != synth) {
        m_synth = std::move(synth);
        if (m_synth) {
            connect(m_synth.get(), &Device::dataChanged, this, &SynthController::sampleRateChanged);
        }
        emit synthChanged();
        requestSettings();
    }
}

} // namespace noteahead
