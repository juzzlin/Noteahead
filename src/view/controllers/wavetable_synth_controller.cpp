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

#include "wavetable_synth_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/wavetable_synth_device.hpp"
#include "../../domain/dsp/lfo.hpp"

#include <cmath>

namespace noteahead {

WavetableSynthController::WavetableSynthController(std::shared_ptr<WavetableSynthDevice> synth, QObject * parent)
  : DeviceController { parent }
  , m_synth { std::move(synth) }
{
    connectDeviceSignals();
}

WavetableSynthController::~WavetableSynthController() = default;

DeviceController::DeviceS WavetableSynthController::device() const
{
    return m_synth;
}

bool WavetableSynthController::setDevice(DeviceS device)
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
int WavetableSynthController::osc1Pos() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc1Pos() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setOsc1Pos(int pos)
{
    if (m_synth) {
        m_synth->setOsc1Pos(static_cast<float>(pos) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::osc1Octave() const
{
    return m_synth ? m_synth->osc1Octave() : 0;
}

void WavetableSynthController::setOsc1Octave(int oct)
{
    if (m_synth) {
        m_synth->setOsc1Octave(oct);
    }
}

int WavetableSynthController::osc1Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc1Pitch() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setOsc1Pitch(int p)
{
    if (m_synth) {
        m_synth->setOsc1Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::osc1Level() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc1Level() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setOsc1Level(int lvl)
{
    if (m_synth) {
        m_synth->setOsc1Level(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

// Accessors (Osc 2)
int WavetableSynthController::osc2Pos() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc2Pos() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setOsc2Pos(int pos)
{
    if (m_synth) {
        m_synth->setOsc2Pos(static_cast<float>(pos) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::osc2Octave() const
{
    return m_synth ? m_synth->osc2Octave() : 0;
}

void WavetableSynthController::setOsc2Octave(int oct)
{
    if (m_synth) {
        m_synth->setOsc2Octave(oct);
    }
}

int WavetableSynthController::osc2Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc2Pitch() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setOsc2Pitch(int p)
{
    if (m_synth) {
        m_synth->setOsc2Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::osc2Level() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->osc2Level() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setOsc2Level(int lvl)
{
    if (m_synth) {
        m_synth->setOsc2Level(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

// Noise
int WavetableSynthController::noiseLevel() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->noiseLevel() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setNoiseLevel(int lvl)
{
    if (m_synth) {
        m_synth->setNoiseLevel(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

// Filter
int WavetableSynthController::lpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setLpfCutoff(int c)
{
    if (m_synth) {
        m_synth->setLpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::lpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setLpfResonance(int r)
{
    if (m_synth) {
        m_synth->setLpfResonance(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::hpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setHpfCutoff(int c)
{
    if (m_synth) {
        m_synth->setHpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

// Amp EG
int WavetableSynthController::ampAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampAttack() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setAmpAttack(int a)
{
    if (m_synth) {
        m_synth->setAmpAttack(static_cast<float>(a) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::ampDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampDecay() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setAmpDecay(int d)
{
    if (m_synth) {
        m_synth->setAmpDecay(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::ampSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampSustain() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setAmpSustain(int s)
{
    if (m_synth) {
        m_synth->setAmpSustain(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::ampRelease() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampRelease() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setAmpRelease(int r)
{
    if (m_synth) {
        m_synth->setAmpRelease(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

// Mod EG
int WavetableSynthController::modAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modAttack() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setModAttack(int a)
{
    if (m_synth) {
        m_synth->setModAttack(static_cast<float>(a) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::modDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modDecay() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setModDecay(int d)
{
    if (m_synth) {
        m_synth->setModDecay(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::modInt() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modInt() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setModInt(int i)
{
    if (m_synth) {
        m_synth->setModInt(static_cast<float>(i) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::modTarget() const
{
    return m_synth ? static_cast<int>(m_synth->modTarget()) : 0;
}

void WavetableSynthController::setModTarget(int t)
{
    if (m_synth) {
        m_synth->setModTarget(static_cast<WavetableSynthDevice::ModTarget>(t));
    }
}

// LFO
int WavetableSynthController::lfoWaveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfoWaveform()) : 0;
}

void WavetableSynthController::setLfoWaveform(int wave)
{
    if (m_synth) {
        m_synth->setLfoWaveform(static_cast<Lfo::Waveform>(wave));
    }
}

int WavetableSynthController::lfoMode() const
{
    return m_synth ? static_cast<int>(m_synth->lfoMode()) : 0;
}

void WavetableSynthController::setLfoMode(int mode)
{
    if (m_synth) {
        m_synth->setLfoMode(static_cast<Lfo::Mode>(mode));
    }
}

int WavetableSynthController::lfoRate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setLfoRate(int rate)
{
    if (m_synth) {
        m_synth->setLfoRate(static_cast<float>(rate) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::lfoInt() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoInt() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setLfoInt(int i)
{
    if (m_synth) {
        m_synth->setLfoInt(static_cast<float>(i) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::lfoTarget() const
{
    return m_synth ? static_cast<int>(m_synth->lfoTarget()) : 0;
}

void WavetableSynthController::setLfoTarget(int target)
{
    if (m_synth && target >= 0 && target <= 3) {
        m_synth->setLfoTarget(static_cast<WavetableSynthDevice::LfoTarget>(target));
    }
}

// LFO 2
int WavetableSynthController::lfo2Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Waveform()) : 0;
}

void WavetableSynthController::setLfo2Waveform(int wave)
{
    if (m_synth) {
        m_synth->setLfo2Waveform(static_cast<Lfo::Waveform>(wave));
    }
}

int WavetableSynthController::lfo2Mode() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Mode()) : 0;
}

void WavetableSynthController::setLfo2Mode(int mode)
{
    if (m_synth) {
        m_synth->setLfo2Mode(static_cast<Lfo::Mode>(mode));
    }
}

int WavetableSynthController::lfo2Rate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Rate() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setLfo2Rate(int rate)
{
    if (m_synth) {
        m_synth->setLfo2Rate(static_cast<float>(rate) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::lfo2Int() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Int() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setLfo2Int(int i)
{
    if (m_synth) {
        m_synth->setLfo2Int(static_cast<float>(i) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::lfo2Target() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Target()) : 0;
}

void WavetableSynthController::setLfo2Target(int target)
{
    if (m_synth && target >= 0 && target <= 3) {
        m_synth->setLfo2Target(static_cast<WavetableSynthDevice::LfoTarget>(target));
    }
}

// Global
int WavetableSynthController::voiceMode() const
{
    return m_synth ? static_cast<int>(m_synth->voiceMode()) : 0;
}

void WavetableSynthController::setVoiceMode(int mode)
{
    if (m_synth && mode >= 0 && mode <= 1) {
        m_synth->setVoiceMode(static_cast<WavetableSynthDevice::VoiceMode>(mode));
    }
}

int WavetableSynthController::voiceDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->voiceDepth() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setVoiceDepth(int depth)
{
    if (m_synth) {
        m_synth->setVoiceDepth(static_cast<float>(depth) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::panSpread() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->panSpread() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setPanSpread(int spread)
{
    if (m_synth) {
        m_synth->setPanSpread(static_cast<float>(spread) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::portamento() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->portamento() * Constants::uiInternalScaling())) : 0;
}

void WavetableSynthController::setPortamento(int p)
{
    if (m_synth) {
        m_synth->setPortamento(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int WavetableSynthController::pitchBendRange() const
{
    return m_synth ? m_synth->pitchBendRange() : 2;
}

void WavetableSynthController::setPitchBendRange(int range)
{
    if (m_synth) {
        m_synth->setPitchBendRange(range);
    }
}

int WavetableSynthController::wavetableIndex() const
{
    return m_synth ? m_synth->wavetableIndex() : 0;
}

void WavetableSynthController::setWavetableIndex(int index)
{
    if (m_synth) {
        m_synth->setWavetableIndex(index);
    }
}

QStringList WavetableSynthController::wavetableNames() const
{
    QStringList names;
    if (m_synth) {
        for (const auto & name : m_synth->wavetableNames()) {
            names << QString::fromStdString(name);
        }
    }
    return names;
}

QStringList WavetableSynthController::voiceModes() const
{
    return { tr("Poly"), tr("Unison") };
}

QStringList WavetableSynthController::octaveNames() const
{
    return { "32'", "16'", "8'", "4'", "2'" };
}

QStringList WavetableSynthController::modTargetNames() const
{
    return { tr("Cutoff"), tr("Pitch 1"), tr("Pitch 2"), tr("Osc 1 Pos"), tr("Osc 2 Pos") };
}

QStringList WavetableSynthController::lfoWaveformNames() const
{
    QStringList list;
    for (auto && name : Lfo::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

QStringList WavetableSynthController::lfoModeNames() const
{
    return { tr("Normal"), tr("BPM"), tr("1-Shot") };
}

QStringList WavetableSynthController::lfoTargetNames() const
{
    return { tr("Pitch"), tr("Cutoff"), tr("Osc 1 Pos"), tr("Osc 2 Pos") };
}

QStringList WavetableSynthController::lfo2WaveformNames() const
{
    return lfoWaveformNames();
}

QStringList WavetableSynthController::lfo2ModeNames() const
{
    return lfoModeNames();
}

QStringList WavetableSynthController::lfo2TargetNames() const
{
    return lfoTargetNames();
}

void WavetableSynthController::requestSettings()
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
    emit hpfCutoffChanged();
    emit ampAttackChanged();
    emit ampDecayChanged();
    emit ampSustainChanged();
    emit ampReleaseChanged();
    emit modAttackChanged();
    emit modDecayChanged();
    emit modIntChanged();
    emit modTargetChanged();
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
    emit wavetableIndexChanged();
    emit wavetableNamesChanged();

    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
}

void WavetableSynthController::setDeviceService(std::shared_ptr<DeviceService> deviceService)
{
    m_deviceService = std::move(deviceService);
}

void WavetableSynthController::connectDeviceSignals()
{
    if (!m_synth) {
        return;
    }

    connect(m_synth.get(), &WavetableSynthDevice::dataChanged, this, &WavetableSynthController::requestSettings);
}

} // namespace noteahead
