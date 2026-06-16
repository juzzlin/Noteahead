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

#include "../../application/service/device_service.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/synth_presets.hpp"

#include <QDebug>
#include <cmath>

namespace noteahead {

SynthController::SynthController(std::shared_ptr<SynthDevice> synth, QObject * parent)
  : DeviceController { parent }
  , m_synth { std::move(synth) }
{
    connectDeviceSignals();

    for (int i = 0; i < 128; ++i) {
        m_userPresets[i] = SynthPresets::initPreset();
    }
}

SynthController::~SynthController() = default;

DeviceController::DeviceS SynthController::device() const
{
    return m_synth;
}

bool SynthController::setDevice(DeviceS device)
{
    if (const auto synth = std::dynamic_pointer_cast<SynthDevice>(device)) {
        setSynth(synth);
        return true;
    }
    return false;
}

std::shared_ptr<SynthDevice> SynthController::synth() const
{
    return m_synth;
}

QStringList SynthController::vcoWaveformNames() const
{
    QStringList list;
    for (auto && name : PolyBlepOscillator::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList SynthController::lfoWaveformNames() const
{
    QStringList list;
    for (auto && name : Lfo::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList SynthController::voiceModes() const
{
    return { tr("Poly"), tr("Unison") };
}

QStringList SynthController::octaveNames() const
{
    return { "16'", "8'", "4'", "2'" };
}

QStringList SynthController::multiTypeNames() const
{
    return { tr("High"), tr("Low"), tr("Peak"), tr("Decim") };
}

QStringList SynthController::modTargetNames() const
{
    return { tr("Pitch 1"), tr("Pitch 2"), tr("Pitch 3"), tr("Cutoff") };
}

QStringList SynthController::lfoModeNames() const
{
    return { tr("Normal"), tr("BPM"), tr("1-Shot") };
}

QStringList SynthController::lfoTargetNames() const
{
    return { tr("Pitch"), tr("Shape"), tr("Cutoff") };
}

// VCO1
int SynthController::vco1Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->vco1Waveform()) : 0;
}

void SynthController::setVco1Waveform(int wave)
{
    if (m_synth) {
        m_synth->setVco1Waveform(static_cast<PolyBlepOscillator::Waveform>(wave));
    }
}

int SynthController::vco1Octave() const
{
    return m_synth ? m_synth->vco1Octave() : 0;
}

void SynthController::setVco1Octave(int oct)
{
    if (m_synth) {
        m_synth->setVco1Octave(oct);
    }
}

int SynthController::vco1Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco1Pitch() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setVco1Pitch(int p)
{
    if (m_synth) {
        m_synth->setVco1Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int SynthController::vco1Shape() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco1Shape() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setVco1Shape(int s)
{
    if (m_synth) {
        m_synth->setVco1Shape(s / Constants::uiInternalScaling());
    }
}

bool SynthController::vco1Sync() const
{
    return m_synth ? m_synth->vco1Sync() : false;
}

void SynthController::setVco1Sync(bool s)
{
    if (m_synth) {
        m_synth->setVco1Sync(s);
    }
}

// VCO2
int SynthController::vco2Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->vco2Waveform()) : 0;
}

void SynthController::setVco2Waveform(int wave)
{
    if (m_synth) {
        m_synth->setVco2Waveform(static_cast<PolyBlepOscillator::Waveform>(wave));
    }
}

int SynthController::vco2Octave() const
{
    return m_synth ? m_synth->vco2Octave() : 0;
}

void SynthController::setVco2Octave(int oct)
{
    if (m_synth) {
        m_synth->setVco2Octave(oct);
    }
}

int SynthController::vco2Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco2Pitch() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setVco2Pitch(int p)
{
    if (m_synth) {
        m_synth->setVco2Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int SynthController::vco2Shape() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco2Shape() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setVco2Shape(int s)
{
    if (m_synth) {
        m_synth->setVco2Shape(s / Constants::uiInternalScaling());
    }
}

bool SynthController::vco2Sync() const
{
    return m_synth ? m_synth->vco2Sync() : false;
}

void SynthController::setVco2Sync(bool s)
{
    if (m_synth) {
        m_synth->setVco2Sync(s);
    }
}

// Multi Engine
int SynthController::multiType() const
{
    return m_synth ? static_cast<int>(m_synth->multiType()) : 0;
}

void SynthController::setMultiType(int type)
{
    if (m_synth) {
        m_synth->setMultiType(static_cast<MultiEngine::Type>(type));
    }
}

int SynthController::multiShape() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->multiShape() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setMultiShape(int s)
{
    if (m_synth) {
        m_synth->setMultiShape(s / Constants::uiInternalScaling());
    }
}

int SynthController::multiLevel() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->multiLevel() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setMultiLevel(int lvl)
{
    if (m_synth) {
        m_synth->setMultiLevel(lvl / Constants::uiInternalScaling());
    }
}

int SynthController::multiKeyTrack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->multiKeyTrack() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setMultiKeyTrack(int t)
{
    if (m_synth) {
        m_synth->setMultiKeyTrack(t / Constants::uiInternalScaling());
    }
}

// Mixer
int SynthController::mixVco1() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->mixVco1() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setMixVco1(int lvl)
{
    if (m_synth) {
        m_synth->setMixVco1(lvl / Constants::uiInternalScaling());
    }
}

int SynthController::mixVco2() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->mixVco2() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setMixVco2(int lvl)
{
    if (m_synth) {
        m_synth->setMixVco2(lvl / Constants::uiInternalScaling());
    }
}

// Filter
int SynthController::lpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setLpfCutoff(int c)
{
    if (m_synth) {
        m_synth->setLpfCutoff(c / Constants::uiInternalScaling());
    }
}

int SynthController::lpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setLpfResonance(int r)
{
    if (m_synth) {
        m_synth->setLpfResonance(r / Constants::uiInternalScaling());
    }
}

int SynthController::hpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setHpfCutoff(int c)
{
    if (m_synth) {
        m_synth->setHpfCutoff(c / Constants::uiInternalScaling());
    }
}

int SynthController::filterKeyTrack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->filterKeyTrack() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setFilterKeyTrack(int t)
{
    if (m_synth) {
        m_synth->setFilterKeyTrack(t / Constants::uiInternalScaling());
    }
}

// Amp EG
int SynthController::ampAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampAttack() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setAmpAttack(int a)
{
    if (m_synth) {
        m_synth->setAmpAttack(a / Constants::uiInternalScaling());
    }
}

int SynthController::ampDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampDecay() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setAmpDecay(int d)
{
    if (m_synth) {
        m_synth->setAmpDecay(d / Constants::uiInternalScaling());
    }
}

int SynthController::ampSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampSustain() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setAmpSustain(int s)
{
    if (m_synth) {
        m_synth->setAmpSustain(s / Constants::uiInternalScaling());
    }
}

int SynthController::ampRelease() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampRelease() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setAmpRelease(int r)
{
    if (m_synth) {
        m_synth->setAmpRelease(r / Constants::uiInternalScaling());
    }
}

int SynthController::ampVelocitySensitivity() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampVelocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setAmpVelocitySensitivity(int sensitivity)
{
    if (m_synth) {
        m_synth->setAmpVelocitySensitivity(sensitivity / Constants::uiInternalScaling());
    }
}

// Mod EG
int SynthController::modAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modAttack() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setModAttack(int a)
{
    if (m_synth) {
        m_synth->setModAttack(a / Constants::uiInternalScaling());
    }
}

int SynthController::modDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modDecay() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setModDecay(int d)
{
    if (m_synth) {
        m_synth->setModDecay(d / Constants::uiInternalScaling());
    }
}

int SynthController::modInt() const
{
    if (m_synth) {
        if (auto p = m_synth->parameter(Constants::NahdXml::xmlKeyModIntensity().toStdString()); p) {
            return static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling()));
        }
    }
    return 0;
}

void SynthController::setModInt(int i)
{
    if (m_synth) {
        m_synth->setModInt(i / Constants::uiInternalScaling());
    }
}

int SynthController::modTarget() const
{
    return m_synth ? static_cast<int>(m_synth->modTarget()) : 0;
}

void SynthController::setModTarget(int t)
{
    if (m_synth) {
        m_synth->setModTarget(static_cast<SynthDevice::ModTarget>(t));
    }
}

// Lfo
int SynthController::lfoWaveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfoWaveform()) : 0;
}

void SynthController::setLfoWaveform(int wave)
{
    if (m_synth) {
        m_synth->setLfoWaveform(static_cast<Lfo::Waveform>(wave));
    }
}

int SynthController::lfoMode() const
{
    return m_synth ? static_cast<int>(m_synth->lfoMode()) : 0;
}

void SynthController::setLfoMode(int mode)
{
    if (m_synth) {
        m_synth->setLfoMode(static_cast<Lfo::Mode>(mode));
    }
}

int SynthController::lfoRate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setLfoRate(int rate)
{
    if (m_synth) {
        m_synth->setLfoRate(rate / Constants::uiInternalScaling());
    }
}

int SynthController::lfoInt() const
{
    if (m_synth) {
        if (auto p = m_synth->parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString()); p) {
            return static_cast<int>(std::round(p->get().value() * Constants::uiInternalScaling()));
        }
    }
    return 0;
}

void SynthController::setLfoInt(int intensity)
{
    if (m_synth) {
        m_synth->setLfoInt(intensity / Constants::uiInternalScaling());
    }
}

int SynthController::lfoTarget() const
{
    return m_synth ? static_cast<int>(m_synth->lfoTarget()) : 0;
}

void SynthController::setLfoTarget(int target)
{
    if (m_synth) {
        m_synth->setLfoTarget(static_cast<SynthDevice::LfoTarget>(target));
    }
}

// Global
int SynthController::voiceMode() const
{
    return m_synth ? static_cast<int>(m_synth->voiceMode()) : 0;
}

void SynthController::setVoiceMode(int m)
{
    if (m_synth) {
        m_synth->setVoiceMode(static_cast<SynthDevice::VoiceMode>(m));
    }
}

int SynthController::voiceDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->voiceDepth() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setVoiceDepth(int d)
{
    if (m_synth) {
        m_synth->setVoiceDepth(d / Constants::uiInternalScaling());
    }
}

int SynthController::portamento() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->portamento() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setPortamento(int p)
{
    if (m_synth) {
        m_synth->setPortamento(p / Constants::uiInternalScaling());
    }
}

int SynthController::panSpread() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->panSpread() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setPanSpread(int s)
{
    if (m_synth) {
        m_synth->setPanSpread(s / Constants::uiInternalScaling());
    }
}

int SynthController::pitchBendRange() const
{
    return m_synth ? m_synth->pitchBendRange() : 0;
}

void SynthController::setPitchBendRange(int r)
{
    if (m_synth) {
        m_synth->setPitchBendRange(r);
    }
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

// Oscillator drift
int SynthController::oscillatorDrift() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscillatorDrift() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setOscillatorDrift(int drift)
{
    if (m_synth) {
        m_synth->setOscillatorDrift(drift / Constants::uiInternalScaling());
    }
}

// Cross modulation
int SynthController::crossModDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->crossModDepth() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setCrossModDepth(int depth)
{
    if (m_synth) {
        m_synth->setCrossModDepth(depth / Constants::uiInternalScaling());
    }
}

// Delay
int SynthController::delayType() const
{
    return m_synth ? static_cast<int>(m_synth->delayType()) : 0;
}

void SynthController::setDelayType(int type)
{
    if (m_synth) {
        m_synth->setDelayType(static_cast<DelayEffect::Type>(type));
    }
}

int SynthController::delayTime() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayTime() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setDelayTime(int time)
{
    if (m_synth) {
        m_synth->setDelayTime(time / 10000.0);
    }
}

int SynthController::delayFeedback() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedback() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setDelayFeedback(int fb)
{
    if (m_synth) {
        m_synth->setDelayFeedback(fb / Constants::uiInternalScaling());
    }
}

int SynthController::delayDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayDepth() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setDelayDepth(int d)
{
    if (m_synth) {
        m_synth->setDelayDepth(d / Constants::uiInternalScaling());
    }
}

int SynthController::delayMix() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayMix() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setDelayMix(int mix)
{
    if (m_synth) {
        m_synth->setDelayMix(mix / Constants::uiInternalScaling());
    }
}

bool SynthController::delaySync() const
{
    return m_synth ? m_synth->delaySync() : false;
}

void SynthController::setDelaySync(bool sync)
{
    if (m_synth) {
        m_synth->setDelaySync(sync);
    }
}

int SynthController::delaySyncDivision() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delaySyncDivision() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setDelaySyncDivision(int div)
{
    if (m_synth) {
        m_synth->setDelaySyncDivision(div / Constants::uiInternalScaling());
    }
}

int SynthController::delayFeedbackLpf() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackLpf() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setDelayFeedbackLpf(int cutoff)
{
    if (m_synth) {
        m_synth->setFeedbackLpf(cutoff / Constants::uiInternalScaling());
    }
}

int SynthController::delayFeedbackHpf() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackHpf() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setDelayFeedbackHpf(int cutoff)
{
    if (m_synth) {
        m_synth->setFeedbackHpf(cutoff / Constants::uiInternalScaling());
    }
}

void SynthController::initialize()
{
}

void SynthController::requestSettings()
{
    emit vco1WaveformChanged();
    emit vco1OctaveChanged();
    emit vco1PitchChanged();
    emit vco1ShapeChanged();
    emit vco1SyncChanged();

    emit vco2WaveformChanged();
    emit vco2OctaveChanged();
    emit vco2PitchChanged();
    emit vco2ShapeChanged();
    emit vco2SyncChanged();

    emit vco3WaveformChanged();
    emit vco3OctaveChanged();
    emit vco3PitchChanged();
    emit vco3ShapeChanged();
    emit vco3SyncChanged();

    emit multiTypeChanged();
    emit multiShapeChanged();
    emit multiLevelChanged();
    emit multiKeyTrackChanged();

    emit mixVco1Changed();
    emit mixVco2Changed();
    emit mixVco3Changed();

    emit lpfCutoffChanged();
    emit lpfResonanceChanged();
    emit hpfCutoffChanged();
    emit filterKeyTrackChanged();

    emit ampAttackChanged();
    emit ampDecayChanged();
    emit ampSustainChanged();
    emit ampReleaseChanged();
    emit ampVelocitySensitivityChanged();

    emit modAttackChanged();
    emit modDecayChanged();
    emit modIntChanged();
    emit modTargetChanged();

    emit lfoWaveformChanged();
    emit lfoModeChanged();
    emit lfoRateChanged();
    emit lfoIntChanged();
    emit lfoTargetChanged();

    emit voiceModeChanged();
    emit voiceDepthChanged();
    emit portamentoChanged();
    emit panSpreadChanged();
    emit pitchBendRangeChanged();

    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();

    emit oscillatorDriftChanged();
    emit crossModDepthChanged();
    emit delayTypeChanged();
    emit delayTimeChanged();
    emit delayFeedbackChanged();
    emit delayDepthChanged();
    emit delayMixChanged();
    emit delaySyncChanged();
    emit delaySyncDivisionChanged();
    emit delayFeedbackLpfChanged();
    emit delayFeedbackHpfChanged();
}

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

void SynthController::setSynth(std::shared_ptr<SynthDevice> synth)
{
    if (m_synth != synth) {
        if (m_synth) {
            disconnect(m_synth.get(), nullptr, this, nullptr);
        }
        m_synth = std::move(synth);
        connectDeviceSignals();
        emit synthChanged();
        requestSettings();
    }
}

// VCO3
int SynthController::vco3Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->vco3Waveform()) : 0;
}

void SynthController::setVco3Waveform(int wave)
{
    if (m_synth) {
        m_synth->setVco3Waveform(static_cast<PolyBlepOscillator::Waveform>(wave));
    }
}

int SynthController::vco3Octave() const
{
    return m_synth ? m_synth->vco3Octave() : 0;
}

void SynthController::setVco3Octave(int oct)
{
    if (m_synth) {
        m_synth->setVco3Octave(oct);
    }
}

int SynthController::vco3Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco3Pitch() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setVco3Pitch(int p)
{
    if (m_synth) {
        m_synth->setVco3Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int SynthController::vco3Shape() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco3Shape() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setVco3Shape(int s)
{
    if (m_synth) {
        m_synth->setVco3Shape(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

bool SynthController::vco3Sync() const
{
    return m_synth ? m_synth->vco3Sync() : false;
}

void SynthController::setVco3Sync(bool s)
{
    if (m_synth) {
        m_synth->setVco3Sync(s);
    }
}

int SynthController::mixVco3() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->mixVco3() * Constants::uiInternalScaling())) : 0;
}

void SynthController::setMixVco3(int lvl)
{
    if (m_synth) {
        m_synth->setMixVco3(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

} // namespace noteahead
