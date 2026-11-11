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

#include "wavetable_synth_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/dsp/lfo.hpp"
#include "wavetable_synth_device.hpp"
#include "wavetable_synth_presets.hpp"

#include <cmath>

namespace noteahead {

WavetableSynthDialogModel::WavetableSynthDialogModel(std::shared_ptr<WavetableSynthDevice> synth, QObject * parent)
  : DeviceDialogModel { parent }
  , m_synth { std::move(synth) }
{
    connectDeviceSignals();
}

WavetableSynthDialogModel::~WavetableSynthDialogModel() = default;

DeviceDialogModel::DeviceS WavetableSynthDialogModel::device() const
{
    return m_synth;
}

bool WavetableSynthDialogModel::setDevice(DeviceS device)
{
    if (const auto wavetableSynth = std::dynamic_pointer_cast<WavetableSynthDevice>(device); wavetableSynth) {
        if (m_synth) {
            disconnect(m_synth.get(), nullptr, this, nullptr);
        }
        m_synth = std::move(wavetableSynth);
        connectDeviceSignals();
        requestSettings();
        return true;
    }
    return false;
}

// Accessors (Osc 1)
int WavetableSynthDialogModel::osc1Pos() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc1Pos() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setOsc1Pos(int pos)
{
    if (m_synth) {
        m_synth->setOsc1Pos(static_cast<float>(pos) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::osc1Octave() const
{
    return m_synth ? m_synth->osc1Octave() : 0;
}

void WavetableSynthDialogModel::setOsc1Octave(int oct)
{
    if (m_synth) {
        m_synth->setOsc1Octave(oct);
    }
}

int WavetableSynthDialogModel::osc1Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc1Pitch() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setOsc1Pitch(int p)
{
    if (m_synth) {
        m_synth->setOsc1Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::osc1Level() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc1Level() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setOsc1Level(int lvl)
{
    if (m_synth) {
        m_synth->setOsc1Level(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

// Accessors (Osc 2)
int WavetableSynthDialogModel::osc2Pos() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc2Pos() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setOsc2Pos(int pos)
{
    if (m_synth) {
        m_synth->setOsc2Pos(static_cast<float>(pos) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::osc2Octave() const
{
    return m_synth ? m_synth->osc2Octave() : 0;
}

void WavetableSynthDialogModel::setOsc2Octave(int oct)
{
    if (m_synth) {
        m_synth->setOsc2Octave(oct);
    }
}

int WavetableSynthDialogModel::osc2Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc2Pitch() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setOsc2Pitch(int p)
{
    if (m_synth) {
        m_synth->setOsc2Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::osc2Level() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc2Level() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setOsc2Level(int lvl)
{
    if (m_synth) {
        m_synth->setOsc2Level(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

// Noise
int WavetableSynthDialogModel::noiseLevel() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->noiseLevel() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setNoiseLevel(int lvl)
{
    if (m_synth) {
        m_synth->setNoiseLevel(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

// Filter
int WavetableSynthDialogModel::lpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLpfCutoff(int c)
{
    if (m_synth) {
        m_synth->setLpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::lpfSlope() const
{
    return m_synth ? m_synth->lpfSlope() : 1;
}

void WavetableSynthDialogModel::setLpfSlope(int value)
{
    if (m_synth) {
        m_synth->setLpfSlope(value);
        emit lpfSlopeChanged();
    }
}

int WavetableSynthDialogModel::hpfSlope() const
{
    return m_synth ? m_synth->hpfSlope() : 1;
}

void WavetableSynthDialogModel::setHpfSlope(int value)
{
    if (m_synth) {
        m_synth->setHpfSlope(value);
        emit hpfSlopeChanged();
    }
}

int WavetableSynthDialogModel::lpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLpfResonance(int r)
{
    if (m_synth) {
        m_synth->setLpfResonance(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::hpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setHpfCutoff(int c)
{
    if (m_synth) {
        m_synth->setHpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

// Amp EG
int WavetableSynthDialogModel::ampAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampAttack() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setAmpAttack(int a)
{
    if (m_synth) {
        m_synth->setAmpAttack(static_cast<float>(a) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::ampDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampDecay() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setAmpDecay(int d)
{
    if (m_synth) {
        m_synth->setAmpDecay(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::ampSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampSustain() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setAmpSustain(int s)
{
    if (m_synth) {
        m_synth->setAmpSustain(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::ampRelease() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampRelease() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setAmpRelease(int r)
{
    if (m_synth) {
        m_synth->setAmpRelease(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::ampVelocitySensitivity() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampVelocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setAmpVelocitySensitivity(int sensitivity)
{
    if (m_synth) {
        m_synth->setAmpVelocitySensitivity(static_cast<float>(sensitivity) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::ampCurve() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampCurve() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setAmpCurve(int curve)
{
    if (m_synth) {
        m_synth->setAmpCurve(static_cast<float>(curve) / Constants::uiInternalScaling());
    }
}

// Mod EG
int WavetableSynthDialogModel::modAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modAttack() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setModAttack(int a)
{
    if (m_synth) {
        m_synth->setModAttack(static_cast<float>(a) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::modDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modDecay() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setModDecay(int d)
{
    if (m_synth) {
        m_synth->setModDecay(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::modSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modSustain() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setModSustain(int sustain)
{
    if (m_synth) {
        m_synth->setModSustain(static_cast<float>(sustain) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::modInt() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modInt() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setModInt(int i)
{
    if (m_synth) {
        m_synth->setModInt(static_cast<float>(i) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::modTarget() const
{
    return m_synth ? static_cast<int>(m_synth->modTarget()) : 0;
}

void WavetableSynthDialogModel::setModTarget(int t)
{
    if (m_synth) {
        m_synth->setModTarget(static_cast<WavetableSynthDevice::ModTarget>(t));
    }
}

int WavetableSynthDialogModel::modCurve() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modCurve() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setModCurve(int curve)
{
    if (m_synth) {
        m_synth->setModCurve(static_cast<float>(curve) / Constants::uiInternalScaling());
    }
}

// LFO
int WavetableSynthDialogModel::lfoWaveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfoWaveform()) : 0;
}

void WavetableSynthDialogModel::setLfoWaveform(int wave)
{
    if (m_synth) {
        m_synth->setLfoWaveform(static_cast<Lfo::Waveform>(wave));
    }
}

int WavetableSynthDialogModel::lfoMode() const
{
    return m_synth ? static_cast<int>(m_synth->lfoMode()) : 0;
}

void WavetableSynthDialogModel::setLfoMode(int mode)
{
    if (m_synth) {
        m_synth->setLfoMode(static_cast<Lfo::Mode>(mode));
    }
}

int WavetableSynthDialogModel::lfoRate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLfoRate(int rate)
{
    if (m_synth) {
        m_synth->setLfoRate(static_cast<float>(rate) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::lfoInt() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoInt() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLfoInt(int i)
{
    if (m_synth) {
        m_synth->setLfoInt(static_cast<float>(i) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::lfoDelay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoDelay() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLfoDelay(int delay)
{
    if (m_synth) {
        m_synth->setLfoDelay(static_cast<float>(delay) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::lfoFade() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoFade() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLfoFade(int fade)
{
    if (m_synth) {
        m_synth->setLfoFade(static_cast<float>(fade) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::lfoTarget() const
{
    return m_synth ? static_cast<int>(m_synth->lfoTarget()) : 0;
}

void WavetableSynthDialogModel::setLfoTarget(int target)
{
    // Bounded by the list the dialog offers, so a target added to one cannot go missing from the other
    if (m_synth && target >= 0 && target < static_cast<int>(lfoTargetNames().size())) {
        m_synth->setLfoTarget(static_cast<WavetableSynthDevice::LfoTarget>(target));
    }
}

// LFO 2
int WavetableSynthDialogModel::lfo2Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Waveform()) : 0;
}

void WavetableSynthDialogModel::setLfo2Waveform(int wave)
{
    if (m_synth) {
        m_synth->setLfo2Waveform(static_cast<Lfo::Waveform>(wave));
    }
}

int WavetableSynthDialogModel::lfo2Mode() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Mode()) : 0;
}

void WavetableSynthDialogModel::setLfo2Mode(int mode)
{
    if (m_synth) {
        m_synth->setLfo2Mode(static_cast<Lfo::Mode>(mode));
    }
}

int WavetableSynthDialogModel::lfo2Rate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Rate() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLfo2Rate(int rate)
{
    if (m_synth) {
        m_synth->setLfo2Rate(static_cast<float>(rate) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::lfo2Int() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Int() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLfo2Int(int i)
{
    if (m_synth) {
        m_synth->setLfo2Int(static_cast<float>(i) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::lfo2Delay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Delay() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLfo2Delay(int delay)
{
    if (m_synth) {
        m_synth->setLfo2Delay(static_cast<float>(delay) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::lfo2Fade() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Fade() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setLfo2Fade(int fade)
{
    if (m_synth) {
        m_synth->setLfo2Fade(static_cast<float>(fade) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::lfoRateDivider() const
{
    return m_synth ? m_synth->lfoRateDivider() : 1;
}

void WavetableSynthDialogModel::setLfoRateDivider(int divider)
{
    if (m_synth) {
        m_synth->setLfoRateDivider(divider);
    }
}

int WavetableSynthDialogModel::lfo2RateDivider() const
{
    return m_synth ? m_synth->lfo2RateDivider() : 1;
}

void WavetableSynthDialogModel::setLfo2RateDivider(int divider)
{
    if (m_synth) {
        m_synth->setLfo2RateDivider(divider);
    }
}

bool WavetableSynthDialogModel::lfoFreeRun() const
{
    return m_synth ? m_synth->lfoFreeRun() : false;
}

void WavetableSynthDialogModel::setLfoFreeRun(bool freeRun)
{
    if (m_synth) {
        m_synth->setLfoFreeRun(freeRun);
    }
}

bool WavetableSynthDialogModel::lfo2FreeRun() const
{
    return m_synth ? m_synth->lfo2FreeRun() : false;
}

void WavetableSynthDialogModel::setLfo2FreeRun(bool freeRun)
{
    if (m_synth) {
        m_synth->setLfo2FreeRun(freeRun);
    }
}

int WavetableSynthDialogModel::maxLfoRateDivider() const
{
    return Lfo::MaxRateDivider;
}

int WavetableSynthDialogModel::lfo2Target() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Target()) : 0;
}

void WavetableSynthDialogModel::setLfo2Target(int target)
{
    // Bounded by the list the dialog offers, so a target added to one cannot go missing from the other
    if (m_synth && target >= 0 && target < static_cast<int>(lfo2TargetNames().size())) {
        m_synth->setLfo2Target(static_cast<WavetableSynthDevice::LfoTarget>(target));
    }
}

// Global
int WavetableSynthDialogModel::voiceMode() const
{
    return m_synth ? static_cast<int>(m_synth->voiceMode()) : 0;
}

void WavetableSynthDialogModel::setVoiceMode(int mode)
{
    // Bounded by the list the dialog offers, so a mode added to one cannot go missing from the other
    if (m_synth && mode >= 0 && mode < static_cast<int>(voiceModes().size())) {
        m_synth->setVoiceMode(static_cast<WavetableSynthDevice::VoiceMode>(mode));
    }
}

int WavetableSynthDialogModel::voiceDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->voiceDepth() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setVoiceDepth(int depth)
{
    if (m_synth) {
        m_synth->setVoiceDepth(static_cast<float>(depth) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::phaseRandomness() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->phaseRandomness() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setPhaseRandomness(int randomness)
{
    if (m_synth) {
        m_synth->setPhaseRandomness(static_cast<float>(randomness) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::panSpread() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->panSpread() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setPanSpread(int spread)
{
    if (m_synth) {
        m_synth->setPanSpread(static_cast<float>(spread) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::portamento() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->portamento() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthDialogModel::setPortamento(int p)
{
    if (m_synth) {
        m_synth->setPortamento(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int WavetableSynthDialogModel::pitchBendRange() const
{
    return m_synth ? m_synth->pitchBendRange() : 2;
}

void WavetableSynthDialogModel::setPitchBendRange(int range)
{
    if (m_synth) {
        m_synth->setPitchBendRange(range);
    }
}

int WavetableSynthDialogModel::wavetableIndex() const
{
    return m_synth ? m_synth->wavetableIndex() : 0;
}

void WavetableSynthDialogModel::setWavetableIndex(int index)
{
    if (m_synth) {
        // Build the set here, on the UI thread, before the synth takes its lock to switch to it.
        WavetableSynthDevice::prepareWavetable(index);
        m_synth->setWavetableIndex(index);
    }
}

QStringList WavetableSynthDialogModel::wavetableNames() const
{
    QStringList names;
    if (m_synth) {
        for (const auto & name : m_synth->wavetableNames()) {
            names << QString::fromStdString(name);
        }
    }
    return names;
}

void WavetableSynthDialogModel::retranslate()
{
    emit translationsChanged();
}

QStringList WavetableSynthDialogModel::voiceModes() const
{
    // Order is the persisted VoiceMode ordinal, so Mono trails the stacked modes rather than sitting
    // next to Poly.
    return { tr("Poly"), tr("Unison"), tr("Dual"), tr("Supersaw"), tr("Drift"), tr("Mono") };
}

QStringList WavetableSynthDialogModel::octaveNames() const
{
    return { "32'", "16'", "8'", "4'", "2'" };
}

QStringList WavetableSynthDialogModel::modTargetNames() const
{
    return { tr("Cutoff"), tr("Pitch 1"), tr("Pitch 2"), tr("Osc 1 Pos"), tr("Osc 2 Pos") };
}

QStringList WavetableSynthDialogModel::lfoWaveformNames() const
{
    QStringList list;
    for (auto && name : Lfo::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList WavetableSynthDialogModel::lfoModeNames() const
{
    return { tr("Normal"), tr("BPM"), tr("1-Shot") };
}

QStringList WavetableSynthDialogModel::lfoTargetNames() const
{
    return { tr("Pitch"), tr("Cutoff"), tr("Osc 1 Pos"), tr("Osc 2 Pos"), tr("Volume"), tr("Resonance"), tr("Pan") };
}

QStringList WavetableSynthDialogModel::lfo2WaveformNames() const
{
    return lfoWaveformNames();
}

QStringList WavetableSynthDialogModel::lfo2ModeNames() const
{
    return lfoModeNames();
}

QStringList WavetableSynthDialogModel::lfo2TargetNames() const
{
    return lfoTargetNames();
}

QStringList WavetableSynthDialogModel::factoryPresetNames() const
{
    QStringList list;
    for (const auto & preset : WavetableSynthPresets::presets()) {
        list << QString::fromStdString(preset.name);
    }
    return list;
}

void WavetableSynthDialogModel::loadFactoryPreset(int index)
{
    if (m_synth) {
        m_synth->loadPreset(index);
        requestSettings();
    }
}

void WavetableSynthDialogModel::requestSettings()
{
    emit osc1PosChanged();
    emit osc1OctaveChanged();
    emit osc1PitchChanged();
    emit osc1LevelChanged();
    emit osc2PosChanged();
    emit osc2OctaveChanged();
    emit osc2PitchChanged();
    emit osc2LevelChanged();
    emit noiseLevelChanged();
    emit lpfCutoffChanged();
    emit lpfResonanceChanged();
    emit lpfSlopeChanged();
    emit hpfSlopeChanged();
    emit hpfCutoffChanged();
    emit ampAttackChanged();
    emit ampDecayChanged();
    emit ampSustainChanged();
    emit ampReleaseChanged();
    emit ampVelocitySensitivityChanged();
    emit ampCurveChanged();
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
    emit wavetableIndexChanged();
    emit wavetableNamesChanged();

    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
}

void WavetableSynthDialogModel::setDeviceService(std::shared_ptr<DeviceService> deviceService)
{
    m_deviceService = std::move(deviceService);
}

} // namespace noteahead
