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
#include "../../domain/devices/synth_device.hpp"
#include <cmath>

namespace noteahead {

SynthController::SynthController(std::shared_ptr<SynthDevice> synth, QObject * parent)
    : QObject { parent }
    , m_synth { std::move(synth) }
{
}

SynthController::~SynthController() = default;

std::shared_ptr<SynthDevice> SynthController::synth() const
{
    return m_synth;
}

int SynthController::detune() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->detune() * 100.0f)) : 0;
}

void SynthController::setDetune(int detune)
{
    if (m_synth) {
        m_synth->setDetune(detune / 100.0f);
        emit detuneChanged();
    }
}

int SynthController::freqModAmount() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->freqModAmount() * 100.0f)) : 0;
}

void SynthController::setFreqModAmount(int amount)
{
    if (m_synth) {
        m_synth->setFreqModAmount(amount / 100.0f);
        emit freqModAmountChanged();
    }
}

int SynthController::freqModSource() const
{
    return m_synth ? static_cast<int>(m_synth->freqModSource()) : 0;
}

void SynthController::setFreqModSource(int source)
{
    if (m_synth) {
        m_synth->setFreqModSource(static_cast<SynthDevice::FreqModSource>(source));
        emit freqModSourceChanged();
    }
}

int SynthController::keyAssignMode() const
{
    return m_synth ? static_cast<int>(m_synth->keyAssignMode()) : 0;
}

void SynthController::setKeyAssignMode(int mode)
{
    if (m_synth) {
        m_synth->setKeyAssignMode(static_cast<SynthDevice::KeyAssignMode>(mode));
        emit keyAssignModeChanged();
    }
}

int SynthController::pulseWidth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->pulseWidth() * 100.0f)) : 50;
}

void SynthController::setPulseWidth(int pw)
{
    if (m_synth) {
        m_synth->setPulseWidth(pw / 100.0f);
        emit pulseWidthChanged();
    }
}

int SynthController::filterCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->filterCutoff() * 100.0f)) : 50;
}

void SynthController::setFilterCutoff(int cutoff)
{
    if (m_synth) {
        m_synth->setFilterCutoff(cutoff / 100.0f);
        emit filterCutoffChanged();
    }
}

int SynthController::filterResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->filterResonance() * 100.0f)) : 0;
}

void SynthController::setFilterResonance(int resonance)
{
    if (m_synth) {
        m_synth->setFilterResonance(resonance / 100.0f);
        emit filterResonanceChanged();
    }
}

int SynthController::filterEnvAmount() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->filterEnvAmount() * 100.0f)) : 0;
}

void SynthController::setFilterEnvAmount(int amount)
{
    if (m_synth) {
        m_synth->setFilterEnvAmount(amount / 100.0f);
        emit filterEnvAmountChanged();
    }
}

int SynthController::filterKeyTrack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->filterKeyTrack() * 100.0f)) : 0;
}

void SynthController::setFilterKeyTrack(int track)
{
    if (m_synth) {
        m_synth->setFilterKeyTrack(track / 100.0f);
        emit filterKeyTrackChanged();
    }
}

int SynthController::filterModAmount() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->filterModAmount() * 100.0f)) : 0;
}

void SynthController::setFilterModAmount(int amount)
{
    if (m_synth) {
        m_synth->setFilterModAmount(amount / 100.0f);
        emit filterModAmountChanged();
    }
}

int SynthController::volume() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->volume() * 100.0f)) : 100;
}

void SynthController::setVolume(int volume)
{
    if (m_synth) {
        m_synth->setVolume(volume / 100.0f);
        emit volumeChanged();
    }
}

int SynthController::mg1Frequency() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->mg1Frequency() * 100.0f)) : 50;
}

void SynthController::setMg1Frequency(int freq)
{
    if (m_synth) {
        m_synth->setMg1Frequency(freq / 100.0f);
        emit mg1FrequencyChanged();
    }
}

int SynthController::mg2Frequency() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->mg2Frequency() * 100.0f)) : 20;
}

void SynthController::setMg2Frequency(int freq)
{
    if (m_synth) {
        m_synth->setMg2Frequency(freq / 100.0f);
        emit mg2FrequencyChanged();
    }
}

int SynthController::osc1Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->oscWaveform(0)) : 0;
}

void SynthController::setOsc1Waveform(int waveform)
{
    if (m_synth) {
        m_synth->setOscWaveform(0, static_cast<PolyBLEPOscillator::Waveform>(waveform));
        emit osc1WaveformChanged();
    }
}

int SynthController::osc1Level() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscLevel(0) * 100.0f)) : 100;
}

void SynthController::setOsc1Level(int level)
{
    if (m_synth) {
        m_synth->setOscLevel(0, level / 100.0f);
        emit osc1LevelChanged();
    }
}

int SynthController::osc1Tune() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscTune(0) * 100.0f)) : 0;
}

void SynthController::setOsc1Tune(int tune)
{
    if (m_synth) {
        m_synth->setOscTune(0, tune / 100.0f);
        emit osc1TuneChanged();
    }
}

int SynthController::osc1Octave() const
{
    return m_synth ? m_synth->oscOctave(0) : 0;
}

void SynthController::setOsc1Octave(int octave)
{
    if (m_synth) {
        m_synth->setOscOctave(0, octave);
        emit osc1OctaveChanged();
    }
}

int SynthController::osc2Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->oscWaveform(1)) : 0;
}

void SynthController::setOsc2Waveform(int waveform)
{
    if (m_synth) {
        m_synth->setOscWaveform(1, static_cast<PolyBLEPOscillator::Waveform>(waveform));
        emit osc2WaveformChanged();
    }
}

int SynthController::osc2Level() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscLevel(1) * 100.0f)) : 100;
}

void SynthController::setOsc2Level(int level)
{
    if (m_synth) {
        m_synth->setOscLevel(1, level / 100.0f);
        emit osc2LevelChanged();
    }
}

int SynthController::osc2Tune() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscTune(1) * 100.0f)) : 0;
}

void SynthController::setOsc2Tune(int tune)
{
    if (m_synth) {
        m_synth->setOscTune(1, tune / 100.0f);
        emit osc2TuneChanged();
    }
}

int SynthController::osc2Octave() const
{
    return m_synth ? m_synth->oscOctave(1) : 0;
}

void SynthController::setOsc2Octave(int octave)
{
    if (m_synth) {
        m_synth->setOscOctave(1, octave);
        emit osc2OctaveChanged();
    }
}

int SynthController::osc3Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->oscWaveform(2)) : 0;
}

void SynthController::setOsc3Waveform(int waveform)
{
    if (m_synth) {
        m_synth->setOscWaveform(2, static_cast<PolyBLEPOscillator::Waveform>(waveform));
        emit osc3WaveformChanged();
    }
}

int SynthController::osc3Level() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscLevel(2) * 100.0f)) : 100;
}

void SynthController::setOsc3Level(int level)
{
    if (m_synth) {
        m_synth->setOscLevel(2, level / 100.0f);
        emit osc3LevelChanged();
    }
}

int SynthController::osc3Tune() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscTune(2) * 100.0f)) : 0;
}

void SynthController::setOsc3Tune(int tune)
{
    if (m_synth) {
        m_synth->setOscTune(2, tune / 100.0f);
        emit osc3TuneChanged();
    }
}

int SynthController::osc3Octave() const
{
    return m_synth ? m_synth->oscOctave(2) : 0;
}

void SynthController::setOsc3Octave(int octave)
{
    if (m_synth) {
        m_synth->setOscOctave(2, octave);
        emit osc3OctaveChanged();
    }
}

int SynthController::osc4Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->oscWaveform(3)) : 0;
}

void SynthController::setOsc4Waveform(int waveform)
{
    if (m_synth) {
        m_synth->setOscWaveform(3, static_cast<PolyBLEPOscillator::Waveform>(waveform));
        emit osc4WaveformChanged();
    }
}

int SynthController::osc4Level() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscLevel(3) * 100.0f)) : 100;
}

void SynthController::setOsc4Level(int level)
{
    if (m_synth) {
        m_synth->setOscLevel(3, level / 100.0f);
        emit osc4LevelChanged();
    }
}

int SynthController::osc4Tune() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscTune(3) * 100.0f)) : 0;
}

void SynthController::setOsc4Tune(int tune)
{
    if (m_synth) {
        m_synth->setOscTune(3, tune / 100.0f);
        emit osc4TuneChanged();
    }
}

int SynthController::osc4Octave() const
{
    return m_synth ? m_synth->oscOctave(3) : 0;
}

void SynthController::setOsc4Octave(int octave)
{
    if (m_synth) {
        m_synth->setOscOctave(3, octave);
        emit osc4OctaveChanged();
    }
}

int SynthController::vcaAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcaAttack() * 100.0f)) : 10;
}

void SynthController::setVcaAttack(int a)
{
    if (m_synth) {
        m_synth->setVcaAttack(a / 100.0f);
        emit vcaAttackChanged();
    }
}

int SynthController::vcaDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcaDecay() * 100.0f)) : 20;
}

void SynthController::setVcaDecay(int d)
{
    if (m_synth) {
        m_synth->setVcaDecay(d / 100.0f);
        emit vcaDecayChanged();
    }
}

int SynthController::vcaSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcaSustain() * 100.0f)) : 100;
}

void SynthController::setVcaSustain(int s)
{
    if (m_synth) {
        m_synth->setVcaSustain(s / 100.0f);
        emit vcaSustainChanged();
    }
}

int SynthController::vcaRelease() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcaRelease() * 100.0f)) : 20;
}

void SynthController::setVcaRelease(int r)
{
    if (m_synth) {
        m_synth->setVcaRelease(r / 100.0f);
        emit vcaReleaseChanged();
    }
}

int SynthController::vcfAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcfAttack() * 100.0f)) : 10;
}

void SynthController::setVcfAttack(int a)
{
    if (m_synth) {
        m_synth->setVcfAttack(a / 100.0f);
        emit vcfAttackChanged();
    }
}

int SynthController::vcfDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcfDecay() * 100.0f)) : 20;
}

void SynthController::setVcfDecay(int d)
{
    if (m_synth) {
        m_synth->setVcfDecay(d / 100.0f);
        emit vcfDecayChanged();
    }
}

int SynthController::vcfSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcfSustain() * 100.0f)) : 100;
}

void SynthController::setVcfSustain(int s)
{
    if (m_synth) {
        m_synth->setVcfSustain(s / 100.0f);
        emit vcfSustainChanged();
    }
}

int SynthController::vcfRelease() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcfRelease() * 100.0f)) : 20;
}

void SynthController::setVcfRelease(int r)
{
    if (m_synth) {
        m_synth->setVcfRelease(r / 100.0f);
        emit vcfReleaseChanged();
    }
}

void SynthController::initialize()
{
}

void SynthController::reset()
{
    if (m_synth) {
        m_synth->reset();
        requestSettings();
    }
}

void SynthController::requestSettings()
{
    emit detuneChanged();
    emit freqModAmountChanged();
    emit freqModSourceChanged();
    emit keyAssignModeChanged();
    emit pulseWidthChanged();
    emit filterCutoffChanged();
    emit filterResonanceChanged();
    emit filterEnvAmountChanged();
    emit filterKeyTrackChanged();
    emit filterModAmountChanged();
    emit volumeChanged();
    emit mg1FrequencyChanged();
    emit mg2FrequencyChanged();
    emit osc1WaveformChanged();
    emit osc1LevelChanged();
    emit osc1TuneChanged();
    emit osc1OctaveChanged();
    emit osc2WaveformChanged();
    emit osc2LevelChanged();
    emit osc2TuneChanged();
    emit osc2OctaveChanged();
    emit osc3WaveformChanged();
    emit osc3LevelChanged();
    emit osc3TuneChanged();
    emit osc3OctaveChanged();
    emit osc4WaveformChanged();
    emit osc4LevelChanged();
    emit osc4TuneChanged();
    emit osc4OctaveChanged();
    emit vcaAttackChanged();
    emit vcaDecayChanged();
    emit vcaSustainChanged();
    emit vcaReleaseChanged();
    emit vcfAttackChanged();
    emit vcfDecayChanged();
    emit vcfSustainChanged();
    emit vcfReleaseChanged();
}

void SynthController::accept() {}
void SynthController::reject() {}

void SynthController::playNote(int note, double velocity)
{
    if (m_synth) {
        m_synth->processMidiNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity * 127.0));
    }
}

void SynthController::stopNote(int note)
{
    if (m_synth) {
        m_synth->processMidiNoteOff(static_cast<uint8_t>(note));
    }
}

} // namespace noteahead
