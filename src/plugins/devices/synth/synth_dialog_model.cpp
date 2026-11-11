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

#include "synth_dialog_model.hpp"

#include "../../../application/service/device_service.hpp"
#include "../../../common/constants.hpp"
#include "../../../common/utils.hpp"
#include "../../../domain/dsp/poly_blep_oscillator.hpp"
#include "synth_device.hpp"
#include "synth_presets.hpp"

#include <QDebug>
#include <cmath>

namespace noteahead {

namespace {
//! The Time knob reads in milliseconds
constexpr float MaxDelayMs { SynthDevice::MaxDelaySeconds * 1000.0f };
} // namespace

SynthDialogModel::SynthDialogModel(std::shared_ptr<SynthDevice> synth, QObject * parent)
  : DeviceDialogModel { parent }
  , m_synth { std::move(synth) }
{
    connectDeviceSignals();
}

SynthDialogModel::~SynthDialogModel() = default;

DeviceDialogModel::DeviceS SynthDialogModel::device() const
{
    return m_synth;
}

bool SynthDialogModel::setDevice(DeviceS device)
{
    if (const auto synth = std::dynamic_pointer_cast<SynthDevice>(device)) {
        setSynth(synth);
        return true;
    }
    return false;
}

std::shared_ptr<SynthDevice> SynthDialogModel::synth() const
{
    return m_synth;
}

QStringList SynthDialogModel::vcoWaveformNames() const
{
    QStringList list;
    for (auto && name : PolyBlepOscillator::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

int SynthDialogModel::squareWaveformIndex() const
{
    return static_cast<int>(PolyBlepOscillator::Waveform::Square);
}

QStringList SynthDialogModel::lfoWaveformNames() const
{
    QStringList list;
    for (auto && name : Lfo::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

void SynthDialogModel::retranslate()
{
    emit translationsChanged();
}

QStringList SynthDialogModel::voiceModes() const
{
    // Order is the persisted VoiceMode ordinal, so Mono trails the stacked modes rather than sitting
    // next to Poly.
    return { tr("Poly"), tr("Unison"), tr("Dual"), tr("Supersaw"), tr("Drift"), tr("Mono") };
}

QStringList SynthDialogModel::octaveNames() const
{
    // Organ footages, so the index is the octave offset from 8' plus two.
    return { "32'", "16'", "8'", "4'", "2'" };
}

QStringList SynthDialogModel::multiTypeNames() const
{
    // The minilogue xd's own four, named after the filter each one is rather than after the band it
    // happens to leave. A name like "high frequency noise" is true at one end of Shape and false at
    // the other: High at the bottom of its range is a high pass at 20 Hz, which passes the whole
    // spectrum. In ordinal order, because the index is what the project stores.
    return { tr("High Pass"), tr("Low Pass"), tr("Band Pass"), tr("Decimate") };
}

QStringList SynthDialogModel::multiEngineNames() const
{
    // In ordinal order, as MultiEngine::Engine has them. VPM is the Minilogue XD's own name for it.
    return { tr("Noise"), "VPM" };
}

QStringList SynthDialogModel::multiOctaveNames() const
{
    return { "16'", "8'", "4'", "2'" };
}

QStringList SynthDialogModel::vpmTypeNames() const
{
    QStringList names;
    for (auto && name : MultiEngine::vpmTypeNames()) {
        names << QString::fromStdString(name);
    }
    return names;
}

QStringList SynthDialogModel::vpmRatioOffsetNames() const
{
    QStringList names;
    for (auto && name : MultiEngine::vpmRatioOffsetNames()) {
        names << QString::fromStdString(name);
    }
    return names;
}

QStringList SynthDialogModel::modTargetNames() const
{
    // In ordinal order, not in a sensible reading order: a ComboBox's index is the value that gets
    // written to the project, so the list has to follow SynthDevice::ModTarget exactly.
    return QStringList { tr("Pitch 1"), tr("Pitch 2"), tr("Pitch 3"), tr("Cutoff"),
                         tr("Pitch"), tr("Shape"), tr("Volume"), tr("Resonance"), tr("Pan"), tr("HPF Cutoff"),
                         tr("Pitch 4") }
    + vcoFilterTargetNames();
}

QStringList SynthDialogModel::vcoFilterTargetNames() const
{
    // The per-oscillator and multi engine filter corners, trailing both target lists because their
    // ordinals are persisted. Shared, since the Mod EG and the LFOs reach exactly the same ten.
    QStringList names;
    for (int vco = 1; vco <= static_cast<int>(SynthDevice::VcoCount); vco++) {
        names << tr("VCO %1 LPF").arg(vco) << tr("VCO %1 HPF").arg(vco);
    }
    names << tr("Multi LPF") << tr("Multi HPF");
    return names;
}

QStringList SynthDialogModel::lfoModeNames() const
{
    return { tr("Normal"), tr("BPM"), tr("1-Shot") };
}

QStringList SynthDialogModel::lfoTargetNames() const
{
    // Ordinal order again, which is why the single-VCO pitches trail the list. See LfoTarget.
    return QStringList { tr("Pitch"), tr("Shape"), tr("Cutoff"), tr("Volume"), tr("Resonance"), tr("Pan"),
                         tr("Pitch 1"), tr("Pitch 2"), tr("Pitch 3"), tr("HPF Cutoff"), tr("Pitch 4") }
    + vcoFilterTargetNames() + QStringList { tr("Delay LPF"), tr("Delay HPF") };
}

QStringList SynthDialogModel::lfo2WaveformNames() const
{
    return lfoWaveformNames();
}

QStringList SynthDialogModel::lfo2ModeNames() const
{
    return lfoModeNames();
}

QStringList SynthDialogModel::lfo2TargetNames() const
{
    return lfoTargetNames();
}

// VCO1
int SynthDialogModel::vco1Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->vco1Waveform()) : 0;
}

void SynthDialogModel::setVco1Waveform(int wave)
{
    if (m_synth) {
        m_synth->setVco1Waveform(static_cast<PolyBlepOscillator::Waveform>(wave));
    }
}

int SynthDialogModel::vco1Octave() const
{
    return m_synth ? m_synth->vco1Octave() : 0;
}

void SynthDialogModel::setVco1Octave(int oct)
{
    if (m_synth) {
        m_synth->setVco1Octave(oct);
    }
}

int SynthDialogModel::vco1Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco1Pitch() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco1Pitch(int p)
{
    if (m_synth) {
        m_synth->setVco1Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

// The per-oscillator filter sections. The device addresses them by index; QML needs a property per
// control, so each one here is a named wrapper over the indexed pair.
int SynthDialogModel::vco1LpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoLpfCutoff(0) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco1LpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfCutoff(0, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco1FilterEngagedChanged();
    }
}

int SynthDialogModel::vco1LpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoLpfResonance(0) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco1LpfResonance(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfResonance(0, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco1FilterEngagedChanged();
    }
}

int SynthDialogModel::vco1LpfSlope() const
{
    return m_synth ? m_synth->vcoLpfSlope(0) : 0;
}

void SynthDialogModel::setVco1LpfSlope(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfSlope(0, value);
        emit vco1LpfSlopeChanged();
    }
}

int SynthDialogModel::vco1HpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoHpfCutoff(0) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco1HpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setVcoHpfCutoff(0, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco1FilterEngagedChanged();
    }
}

int SynthDialogModel::vco1HpfSlope() const
{
    return m_synth ? m_synth->vcoHpfSlope(0) : 0;
}

void SynthDialogModel::setVco1HpfSlope(int value)
{
    if (m_synth) {
        m_synth->setVcoHpfSlope(0, value);
        emit vco1HpfSlopeChanged();
    }
}

bool SynthDialogModel::vco1FilterEngaged() const
{
    return m_synth && m_synth->vcoFilterEngaged(0);
}

int SynthDialogModel::vco2LpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoLpfCutoff(1) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco2LpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfCutoff(1, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco2FilterEngagedChanged();
    }
}

int SynthDialogModel::vco2LpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoLpfResonance(1) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco2LpfResonance(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfResonance(1, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco2FilterEngagedChanged();
    }
}

int SynthDialogModel::vco2LpfSlope() const
{
    return m_synth ? m_synth->vcoLpfSlope(1) : 0;
}

void SynthDialogModel::setVco2LpfSlope(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfSlope(1, value);
        emit vco2LpfSlopeChanged();
    }
}

int SynthDialogModel::vco2HpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoHpfCutoff(1) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco2HpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setVcoHpfCutoff(1, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco2FilterEngagedChanged();
    }
}

int SynthDialogModel::vco2HpfSlope() const
{
    return m_synth ? m_synth->vcoHpfSlope(1) : 0;
}

void SynthDialogModel::setVco2HpfSlope(int value)
{
    if (m_synth) {
        m_synth->setVcoHpfSlope(1, value);
        emit vco2HpfSlopeChanged();
    }
}

bool SynthDialogModel::vco2FilterEngaged() const
{
    return m_synth && m_synth->vcoFilterEngaged(1);
}

int SynthDialogModel::vco3LpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoLpfCutoff(2) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco3LpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfCutoff(2, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco3FilterEngagedChanged();
    }
}

int SynthDialogModel::vco3LpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoLpfResonance(2) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco3LpfResonance(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfResonance(2, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco3FilterEngagedChanged();
    }
}

int SynthDialogModel::vco3LpfSlope() const
{
    return m_synth ? m_synth->vcoLpfSlope(2) : 0;
}

void SynthDialogModel::setVco3LpfSlope(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfSlope(2, value);
        emit vco3LpfSlopeChanged();
    }
}

int SynthDialogModel::vco3HpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoHpfCutoff(2) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco3HpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setVcoHpfCutoff(2, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco3FilterEngagedChanged();
    }
}

int SynthDialogModel::vco3HpfSlope() const
{
    return m_synth ? m_synth->vcoHpfSlope(2) : 0;
}

void SynthDialogModel::setVco3HpfSlope(int value)
{
    if (m_synth) {
        m_synth->setVcoHpfSlope(2, value);
        emit vco3HpfSlopeChanged();
    }
}

bool SynthDialogModel::vco3FilterEngaged() const
{
    return m_synth && m_synth->vcoFilterEngaged(2);
}

int SynthDialogModel::vco4LpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoLpfCutoff(3) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco4LpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfCutoff(3, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco4FilterEngagedChanged();
    }
}

int SynthDialogModel::vco4LpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoLpfResonance(3) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco4LpfResonance(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfResonance(3, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco4FilterEngagedChanged();
    }
}

int SynthDialogModel::vco4LpfSlope() const
{
    return m_synth ? m_synth->vcoLpfSlope(3) : 0;
}

void SynthDialogModel::setVco4LpfSlope(int value)
{
    if (m_synth) {
        m_synth->setVcoLpfSlope(3, value);
        emit vco4LpfSlopeChanged();
    }
}

int SynthDialogModel::vco4HpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vcoHpfCutoff(3) * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco4HpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setVcoHpfCutoff(3, static_cast<float>(value) / Constants::uiInternalScaling());
        emit vco4FilterEngagedChanged();
    }
}

int SynthDialogModel::vco4HpfSlope() const
{
    return m_synth ? m_synth->vcoHpfSlope(3) : 0;
}

void SynthDialogModel::setVco4HpfSlope(int value)
{
    if (m_synth) {
        m_synth->setVcoHpfSlope(3, value);
        emit vco4HpfSlopeChanged();
    }
}

bool SynthDialogModel::vco4FilterEngaged() const
{
    return m_synth && m_synth->vcoFilterEngaged(3);
}

int SynthDialogModel::vco1Shape() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco1Shape() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco1Shape(int s)
{
    if (m_synth) {
        m_synth->setVco1Shape(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vco1Roundness() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco1Roundness() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco1Roundness(int r)
{
    if (m_synth) {
        m_synth->setVco1Roundness(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

bool SynthDialogModel::vco1Sync() const
{
    return m_synth ? m_synth->vco1Sync() : false;
}

void SynthDialogModel::setVco1Sync(bool s)
{
    if (m_synth) {
        m_synth->setVco1Sync(s);
    }
}

int SynthDialogModel::phaseRandomness() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->phaseRandomness() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setPhaseRandomness(int randomness)
{
    if (m_synth) {
        m_synth->setPhaseRandomness(static_cast<float>(randomness) / Constants::uiInternalScaling());
    }
}

// VCO2
int SynthDialogModel::vco2Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->vco2Waveform()) : 0;
}

void SynthDialogModel::setVco2Waveform(int wave)
{
    if (m_synth) {
        m_synth->setVco2Waveform(static_cast<PolyBlepOscillator::Waveform>(wave));
    }
}

int SynthDialogModel::vco2Octave() const
{
    return m_synth ? m_synth->vco2Octave() : 0;
}

void SynthDialogModel::setVco2Octave(int oct)
{
    if (m_synth) {
        m_synth->setVco2Octave(oct);
    }
}

int SynthDialogModel::vco2Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco2Pitch() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco2Pitch(int p)
{
    if (m_synth) {
        m_synth->setVco2Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vco2Shape() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco2Shape() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco2Shape(int s)
{
    if (m_synth) {
        m_synth->setVco2Shape(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vco2Roundness() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco2Roundness() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco2Roundness(int r)
{
    if (m_synth) {
        m_synth->setVco2Roundness(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

bool SynthDialogModel::vco2Sync() const
{
    return m_synth ? m_synth->vco2Sync() : false;
}

void SynthDialogModel::setVco2Sync(bool s)
{
    if (m_synth) {
        m_synth->setVco2Sync(s);
    }
}

bool SynthDialogModel::vco2Ring() const
{
    return m_synth ? m_synth->vco2Ring() : false;
}

int SynthDialogModel::oscillatorPreviewRevision() const
{
    return m_oscillatorPreviewRevision;
}

QList<qreal> SynthDialogModel::multiPreview(int points) const
{
    QList<qreal> samples;
    if (m_synth && points > 0) {
        for (auto && value : m_synth->multiPreview(static_cast<size_t>(points))) {
            samples << static_cast<qreal>(value);
        }
    }
    return samples;
}

QList<qreal> SynthDialogModel::oscillatorPreview(int vco, int points) const
{
    QList<qreal> preview;
    if (m_synth && vco >= 0 && points > 0) {
        for (auto && value : m_synth->oscillatorPreview(static_cast<size_t>(vco), static_cast<size_t>(points))) {
            preview.append(static_cast<qreal>(value));
        }
    }
    return preview;
}

void SynthDialogModel::setVco2Ring(bool ring)
{
    if (m_synth) {
        m_synth->setVco2Ring(ring);
    }
}

// Multi Engine
int SynthDialogModel::multiType() const
{
    return m_synth ? static_cast<int>(m_synth->multiType()) : 0;
}

void SynthDialogModel::setMultiType(int type)
{
    if (m_synth) {
        m_synth->setMultiType(static_cast<MultiEngine::Type>(type));
    }
}

int SynthDialogModel::multiShape() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->multiShape() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMultiShape(int s)
{
    if (m_synth) {
        m_synth->setMultiShape(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::multiLevel() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->multiLevel() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMultiLevel(int lvl)
{
    if (m_synth) {
        m_synth->setMultiLevel(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::multiKeyTrack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->multiKeyTrack() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMultiKeyTrack(int t)
{
    if (m_synth) {
        m_synth->setMultiKeyTrack(static_cast<float>(t) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::multiLpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->multiLpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMultiLpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setMultiLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::multiLpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->multiLpfResonance() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMultiLpfResonance(int value)
{
    if (m_synth) {
        m_synth->setMultiLpfResonance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::multiLpfSlope() const
{
    return m_synth ? m_synth->multiLpfSlope() : 0;
}

void SynthDialogModel::setMultiLpfSlope(int value)
{
    if (m_synth) {
        m_synth->setMultiLpfSlope(value);
        emit multiLpfSlopeChanged();
    }
}

int SynthDialogModel::multiHpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->multiHpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMultiHpfCutoff(int value)
{
    if (m_synth) {
        m_synth->setMultiHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::multiHpfSlope() const
{
    return m_synth ? m_synth->multiHpfSlope() : 0;
}

void SynthDialogModel::setMultiHpfSlope(int value)
{
    if (m_synth) {
        m_synth->setMultiHpfSlope(value);
        emit multiHpfSlopeChanged();
    }
}

int SynthDialogModel::multiEngine() const
{
    return m_synth ? static_cast<int>(m_synth->multiEngine()) : 0;
}

void SynthDialogModel::setMultiEngine(int engine)
{
    if (m_synth) {
        m_synth->setMultiEngine(static_cast<MultiEngine::Engine>(engine));
        emit multiEngineChanged();
    }
}

int SynthDialogModel::multiOctave() const
{
    return m_synth ? m_synth->multiOctave() : 1;
}

void SynthDialogModel::setMultiOctave(int octave)
{
    if (m_synth) {
        m_synth->setMultiOctave(octave);
        emit multiOctaveChanged();
    }
}

int SynthDialogModel::vpmType() const
{
    return m_synth ? static_cast<int>(m_synth->vpmType()) : 0;
}

void SynthDialogModel::setVpmType(int type)
{
    if (m_synth) {
        m_synth->setVpmType(static_cast<MultiEngine::VpmType>(type));
        emit vpmTypeChanged();
    }
}

int SynthDialogModel::vpmRatioOffset() const
{
    return m_synth ? m_synth->vpmRatioOffset() : static_cast<int>(MultiEngine::VpmRatioOffsetUnity);
}

void SynthDialogModel::setVpmRatioOffset(int index)
{
    if (m_synth) {
        m_synth->setVpmRatioOffset(index);
        emit vpmRatioOffsetChanged();
    }
}

int SynthDialogModel::vpmFeedback() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vpmFeedback() * Constants::uiInternalScaling())) : static_cast<int>(Constants::uiInternalScaling() / 2.0f);
}

void SynthDialogModel::setVpmFeedback(int value)
{
    if (m_synth) {
        m_synth->setVpmFeedback(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vpmNoiseDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vpmNoiseDepth() * Constants::uiInternalScaling())) : static_cast<int>(Constants::uiInternalScaling() / 2.0f);
}

void SynthDialogModel::setVpmNoiseDepth(int value)
{
    if (m_synth) {
        m_synth->setVpmNoiseDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vpmShapeModInt() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vpmShapeModInt() * Constants::uiInternalScaling())) : static_cast<int>(Constants::uiInternalScaling() / 2.0f);
}

void SynthDialogModel::setVpmShapeModInt(int value)
{
    if (m_synth) {
        m_synth->setVpmShapeModInt(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vpmModAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vpmModAttack() * Constants::uiInternalScaling())) : static_cast<int>(Constants::uiInternalScaling() / 2.0f);
}

void SynthDialogModel::setVpmModAttack(int value)
{
    if (m_synth) {
        m_synth->setVpmModAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vpmModDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vpmModDecay() * Constants::uiInternalScaling())) : static_cast<int>(Constants::uiInternalScaling() / 2.0f);
}

void SynthDialogModel::setVpmModDecay(int value)
{
    if (m_synth) {
        m_synth->setVpmModDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vpmModKeyTrack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vpmModKeyTrack() * Constants::uiInternalScaling())) : static_cast<int>(Constants::uiInternalScaling() / 2.0f);
}

void SynthDialogModel::setVpmModKeyTrack(int value)
{
    if (m_synth) {
        m_synth->setVpmModKeyTrack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

// Mixer
int SynthDialogModel::mixVco1() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->mixVco1() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMixVco1(int lvl)
{
    if (m_synth) {
        m_synth->setMixVco1(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::mixVco2() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->mixVco2() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMixVco2(int lvl)
{
    if (m_synth) {
        m_synth->setMixVco2(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

// Filter
int SynthDialogModel::lpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setLpfCutoff(int c)
{
    if (m_synth) {
        m_synth->setLpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::lpfSlope() const
{
    return m_synth ? m_synth->lpfSlope() : 1;
}

void SynthDialogModel::setLpfSlope(int value)
{
    if (m_synth) {
        m_synth->setLpfSlope(value);
        emit lpfSlopeChanged();
    }
}

int SynthDialogModel::hpfSlope() const
{
    return m_synth ? m_synth->hpfSlope() : 1;
}

void SynthDialogModel::setHpfSlope(int value)
{
    if (m_synth) {
        m_synth->setHpfSlope(value);
        emit hpfSlopeChanged();
    }
}

int SynthDialogModel::lpfResonance() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setLpfResonance(int r)
{
    if (m_synth) {
        m_synth->setLpfResonance(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::hpfCutoff() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setHpfCutoff(int c)
{
    if (m_synth) {
        m_synth->setHpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::filterKeyTrack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->filterKeyTrack() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setFilterKeyTrack(int t)
{
    if (m_synth) {
        m_synth->setFilterKeyTrack(static_cast<float>(t) / Constants::uiInternalScaling());
    }
}

// Amp EG
int SynthDialogModel::ampAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampAttack() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setAmpAttack(int a)
{
    if (m_synth) {
        m_synth->setAmpAttack(static_cast<float>(a) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::ampDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampDecay() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setAmpDecay(int d)
{
    if (m_synth) {
        m_synth->setAmpDecay(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::ampSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampSustain() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setAmpSustain(int s)
{
    if (m_synth) {
        m_synth->setAmpSustain(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::ampRelease() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampRelease() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setAmpRelease(int r)
{
    if (m_synth) {
        m_synth->setAmpRelease(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::ampVelocitySensitivity() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampVelocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setAmpVelocitySensitivity(int sensitivity)
{
    if (m_synth) {
        m_synth->setAmpVelocitySensitivity(static_cast<float>(sensitivity) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::ampCurve() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->ampCurve() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setAmpCurve(int curve)
{
    if (m_synth) {
        m_synth->setAmpCurve(static_cast<float>(curve) / Constants::uiInternalScaling());
    }
}

// Mod EG
int SynthDialogModel::modAttack() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modAttack() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setModAttack(int a)
{
    if (m_synth) {
        m_synth->setModAttack(static_cast<float>(a) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::modDecay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modDecay() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setModDecay(int d)
{
    if (m_synth) {
        m_synth->setModDecay(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::modSustain() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modSustain() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setModSustain(int sustain)
{
    if (m_synth) {
        m_synth->setModSustain(static_cast<float>(sustain) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::modInt() const
{
    if (m_synth) {
        if (auto p = m_synth->parameter(Constants::NahdXml::xmlKeyModIntensity().toStdString()); p) {
            return static_cast<int>(std::round(p->get().shownValue() * Constants::uiInternalScaling()));
        }
    }
    return 0;
}

void SynthDialogModel::setModInt(int i)
{
    if (m_synth) {
        m_synth->setModInt(static_cast<float>(i) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::modTarget() const
{
    return m_synth ? static_cast<int>(m_synth->modTarget()) : 0;
}

void SynthDialogModel::setModTarget(int t)
{
    if (m_synth) {
        m_synth->setModTarget(static_cast<SynthDevice::ModTarget>(t));
        // Pointing a source at a per-oscillator filter engages it even with its controls
        // wide open, so the tab markers have to be told.
        emit vco1FilterEngagedChanged();
        emit vco2FilterEngagedChanged();
        emit vco3FilterEngagedChanged();
        emit vco4FilterEngagedChanged();
    }
}

int SynthDialogModel::modCurve() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->modCurve() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setModCurve(int curve)
{
    if (m_synth) {
        m_synth->setModCurve(static_cast<float>(curve) / Constants::uiInternalScaling());
    }
}

// Lfo
int SynthDialogModel::lfoWaveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfoWaveform()) : 0;
}

void SynthDialogModel::setLfoWaveform(int wave)
{
    if (m_synth) {
        m_synth->setLfoWaveform(static_cast<Lfo::Waveform>(wave));
    }
}

int SynthDialogModel::lfoMode() const
{
    return m_synth ? static_cast<int>(m_synth->lfoMode()) : 0;
}

void SynthDialogModel::setLfoMode(int mode)
{
    if (m_synth) {
        m_synth->setLfoMode(static_cast<Lfo::Mode>(mode));
    }
}

int SynthDialogModel::lfoRate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setLfoRate(int rate)
{
    if (m_synth) {
        m_synth->setLfoRate(static_cast<float>(rate) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::lfoInt() const
{
    if (m_synth) {
        if (auto p = m_synth->parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString()); p) {
            return static_cast<int>(std::round(p->get().shownValue() * Constants::uiInternalScaling()));
        }
    }
    return 0;
}

void SynthDialogModel::setLfoInt(int intensity)
{
    if (m_synth) {
        m_synth->setLfoInt(static_cast<float>(intensity) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::lfoTarget() const
{
    return m_synth ? static_cast<int>(m_synth->lfoTarget()) : 0;
}

int SynthDialogModel::lfoDelay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoDelay() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setLfoDelay(int delay)
{
    if (m_synth) {
        m_synth->setLfoDelay(static_cast<float>(delay) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::lfoFade() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfoFade() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setLfoFade(int fade)
{
    if (m_synth) {
        m_synth->setLfoFade(static_cast<float>(fade) / Constants::uiInternalScaling());
    }
}

void SynthDialogModel::setLfoTarget(int target)
{
    if (m_synth) {
        m_synth->setLfoTarget(static_cast<SynthDevice::LfoTarget>(target));
        // Pointing a source at a per-oscillator filter engages it even with its controls
        // wide open, so the tab markers have to be told.
        emit vco1FilterEngagedChanged();
        emit vco2FilterEngagedChanged();
        emit vco3FilterEngagedChanged();
        emit vco4FilterEngagedChanged();
    }
}

// Lfo 2
int SynthDialogModel::lfo2Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Waveform()) : 0;
}

void SynthDialogModel::setLfo2Waveform(int wave)
{
    if (m_synth) {
        m_synth->setLfo2Waveform(static_cast<Lfo::Waveform>(wave));
    }
}

int SynthDialogModel::lfo2Mode() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Mode()) : 0;
}

void SynthDialogModel::setLfo2Mode(int mode)
{
    if (m_synth) {
        m_synth->setLfo2Mode(static_cast<Lfo::Mode>(mode));
    }
}

int SynthDialogModel::lfo2Rate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Rate() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setLfo2Rate(int rate)
{
    if (m_synth) {
        m_synth->setLfo2Rate(static_cast<float>(rate) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::lfo2Int() const
{
    if (m_synth) {
        if (auto p = m_synth->parameter(Constants::NahdXml::xmlKeyLfo2Intensity().toStdString()); p) {
            return static_cast<int>(std::round(p->get().shownValue() * Constants::uiInternalScaling()));
        }
    }
    return 0;
}

void SynthDialogModel::setLfo2Int(int intensity)
{
    if (m_synth) {
        m_synth->setLfo2Int(static_cast<float>(intensity) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::lfo2Target() const
{
    return m_synth ? static_cast<int>(m_synth->lfo2Target()) : 0;
}

int SynthDialogModel::lfo2Delay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Delay() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setLfo2Delay(int delay)
{
    if (m_synth) {
        m_synth->setLfo2Delay(static_cast<float>(delay) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::lfo2Fade() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->lfo2Fade() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setLfo2Fade(int fade)
{
    if (m_synth) {
        m_synth->setLfo2Fade(static_cast<float>(fade) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::lfoRateDivider() const
{
    return m_synth ? m_synth->lfoRateDivider() : 1;
}

void SynthDialogModel::setLfoRateDivider(int divider)
{
    if (m_synth) {
        m_synth->setLfoRateDivider(divider);
    }
}

int SynthDialogModel::lfo2RateDivider() const
{
    return m_synth ? m_synth->lfo2RateDivider() : 1;
}

void SynthDialogModel::setLfo2RateDivider(int divider)
{
    if (m_synth) {
        m_synth->setLfo2RateDivider(divider);
    }
}

bool SynthDialogModel::lfoFreeRun() const
{
    return m_synth ? m_synth->lfoFreeRun() : false;
}

void SynthDialogModel::setLfoFreeRun(bool freeRun)
{
    if (m_synth) {
        m_synth->setLfoFreeRun(freeRun);
    }
}

bool SynthDialogModel::lfo2FreeRun() const
{
    return m_synth ? m_synth->lfo2FreeRun() : false;
}

void SynthDialogModel::setLfo2FreeRun(bool freeRun)
{
    if (m_synth) {
        m_synth->setLfo2FreeRun(freeRun);
    }
}

int SynthDialogModel::maxLfoRateDivider() const
{
    return Lfo::MaxRateDivider;
}

void SynthDialogModel::setLfo2Target(int target)
{
    if (m_synth) {
        m_synth->setLfo2Target(static_cast<SynthDevice::LfoTarget>(target));
        // Pointing a source at a per-oscillator filter engages it even with its controls
        // wide open, so the tab markers have to be told.
        emit vco1FilterEngagedChanged();
        emit vco2FilterEngagedChanged();
        emit vco3FilterEngagedChanged();
        emit vco4FilterEngagedChanged();
    }
}

// Global
int SynthDialogModel::voiceMode() const
{
    return m_synth ? static_cast<int>(m_synth->voiceMode()) : 0;
}

void SynthDialogModel::setVoiceMode(int m)
{
    if (m_synth) {
        m_synth->setVoiceMode(static_cast<SynthDevice::VoiceMode>(m));
    }
}

int SynthDialogModel::voiceDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->voiceDepth() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVoiceDepth(int d)
{
    if (m_synth) {
        m_synth->setVoiceDepth(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::portamento() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->portamento() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setPortamento(int p)
{
    if (m_synth) {
        m_synth->setPortamento(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::panSpread() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->panSpread() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setPanSpread(int s)
{
    if (m_synth) {
        m_synth->setPanSpread(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::pitchBendRange() const
{
    return m_synth ? m_synth->pitchBendRange() : 0;
}

void SynthDialogModel::setPitchBendRange(int r)
{
    if (m_synth) {
        m_synth->setPitchBendRange(r);
    }
}

QStringList SynthDialogModel::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : SynthPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

// Oscillator drift
int SynthDialogModel::oscillatorDrift() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscillatorDrift() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setOscillatorDrift(int drift)
{
    if (m_synth) {
        m_synth->setOscillatorDrift(static_cast<float>(drift) / Constants::uiInternalScaling());
    }
}

// Oscillator instability
int SynthDialogModel::oscillatorInstability() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->oscillatorInstability() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setOscillatorInstability(int instability)
{
    if (m_synth) {
        m_synth->setOscillatorInstability(static_cast<float>(instability) / Constants::uiInternalScaling());
    }
}

// Filter drive
int SynthDialogModel::filterDrive() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->filterDrive() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setFilterDrive(int drive)
{
    if (m_synth) {
        m_synth->setFilterDrive(static_cast<float>(drive) / Constants::uiInternalScaling());
    }
}

// Cross modulation
int SynthDialogModel::crossModDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->crossModDepth() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setCrossModDepth(int depth)
{
    if (m_synth) {
        m_synth->setCrossModDepth(static_cast<float>(depth) / Constants::uiInternalScaling());
    }
}

// Delay
int SynthDialogModel::delayType() const
{
    return m_synth ? static_cast<int>(m_synth->delayType()) : 0;
}

void SynthDialogModel::setDelayType(int type)
{
    if (m_synth) {
        m_synth->setDelayType(static_cast<Delay::Type>(type));
    }
}

int SynthDialogModel::delayTime() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayTime() * MaxDelayMs)) : 0;
}

void SynthDialogModel::setDelayTime(int time)
{
    if (m_synth) {
        m_synth->setDelayTime(static_cast<float>(time) / MaxDelayMs);
    }
}

int SynthDialogModel::delayFeedback() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedback() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setDelayFeedback(int fb)
{
    if (m_synth) {
        m_synth->setDelayFeedback(static_cast<float>(fb) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::delayDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayDepth() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setDelayDepth(int d)
{
    if (m_synth) {
        m_synth->setDelayDepth(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::delayMix() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayMix() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setDelayMix(int mix)
{
    if (m_synth) {
        m_synth->setDelayMix(static_cast<float>(mix) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::chorusRate() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->chorusRate() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setChorusRate(int value)
{
    if (m_synth) {
        m_synth->setChorusRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::chorusDepth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->chorusDepth() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setChorusDepth(int value)
{
    if (m_synth) {
        m_synth->setChorusDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::chorusDelay() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->chorusDelay() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setChorusDelay(int value)
{
    if (m_synth) {
        m_synth->setChorusDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::chorusWidth() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->chorusWidth() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setChorusWidth(int value)
{
    if (m_synth) {
        m_synth->setChorusWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::chorusMix() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->chorusMix() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setChorusMix(int value)
{
    if (m_synth) {
        m_synth->setChorusMix(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool SynthDialogModel::delaySync() const
{
    return m_synth ? m_synth->delaySync() : false;
}

void SynthDialogModel::setDelaySync(bool sync)
{
    if (m_synth) {
        m_synth->setDelaySync(sync);
    }
}

int SynthDialogModel::delaySyncDivision() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delaySyncDivision() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setDelaySyncDivision(int div)
{
    if (m_synth) {
        m_synth->setDelaySyncDivision(static_cast<float>(div) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::delayFeedbackLpf() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackLpf() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setDelayFeedbackLpf(int cutoff)
{
    if (m_synth) {
        m_synth->setFeedbackLpf(static_cast<float>(cutoff) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::delayFeedbackHpf() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->delayFeedbackHpf() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setDelayFeedbackHpf(int cutoff)
{
    if (m_synth) {
        m_synth->setFeedbackHpf(static_cast<float>(cutoff) / Constants::uiInternalScaling());
    }
}

void SynthDialogModel::initialize()
{
}

void SynthDialogModel::requestSettings()
{
    // Every control change comes through here, and any of a dozen of them reshapes the previews
    m_oscillatorPreviewRevision++;
    emit oscillatorPreviewChanged();
    emit vco1WaveformChanged();
    emit vco1OctaveChanged();
    emit vco1PitchChanged();
    emit vco1LpfCutoffChanged();
    emit vco1LpfResonanceChanged();
    emit vco1LpfSlopeChanged();
    emit vco1HpfCutoffChanged();
    emit vco1HpfSlopeChanged();
    emit vco1FilterEngagedChanged();
    emit vco2LpfCutoffChanged();
    emit vco2LpfResonanceChanged();
    emit vco2LpfSlopeChanged();
    emit vco2HpfCutoffChanged();
    emit vco2HpfSlopeChanged();
    emit vco2FilterEngagedChanged();
    emit vco3LpfCutoffChanged();
    emit vco3LpfResonanceChanged();
    emit vco3LpfSlopeChanged();
    emit vco3HpfCutoffChanged();
    emit vco3HpfSlopeChanged();
    emit vco3FilterEngagedChanged();
    emit vco4LpfCutoffChanged();
    emit vco4LpfResonanceChanged();
    emit vco4LpfSlopeChanged();
    emit vco4HpfCutoffChanged();
    emit vco4HpfSlopeChanged();
    emit vco4FilterEngagedChanged();
    emit vco1ShapeChanged();
    emit vco1RoundnessChanged();
    emit vco1SyncChanged();
    emit phaseRandomnessChanged();

    emit vco2WaveformChanged();
    emit vco2OctaveChanged();
    emit vco2PitchChanged();
    emit vco2ShapeChanged();
    emit vco2RoundnessChanged();
    emit vco2SyncChanged();
    emit vco2RingChanged();

    emit vco3WaveformChanged();
    emit vco3OctaveChanged();
    emit vco3PitchChanged();
    emit vco3ShapeChanged();
    emit vco3RoundnessChanged();
    emit vco3SyncChanged();

    emit vco4WaveformChanged();
    emit vco4OctaveChanged();
    emit vco4PitchChanged();
    emit vco4ShapeChanged();
    emit vco4RoundnessChanged();
    emit vco4SyncChanged();

    emit multiTypeChanged();
    emit multiShapeChanged();
    emit multiLevelChanged();
    emit multiKeyTrackChanged();
    emit multiLpfCutoffChanged();
    emit multiLpfResonanceChanged();
    emit multiLpfSlopeChanged();
    emit multiHpfCutoffChanged();
    emit multiHpfSlopeChanged();
    emit multiEngineChanged();
    emit multiOctaveChanged();
    emit vpmTypeChanged();
    emit vpmRatioOffsetChanged();
    emit vpmFeedbackChanged();
    emit vpmNoiseDepthChanged();
    emit vpmShapeModIntChanged();
    emit vpmModAttackChanged();
    emit vpmModDecayChanged();
    emit vpmModKeyTrackChanged();

    emit mixVco1Changed();
    emit mixVco2Changed();
    emit mixVco3Changed();
    emit mixVco4Changed();

    emit lpfCutoffChanged();
    emit lpfResonanceChanged();
    emit lpfSlopeChanged();
    emit hpfSlopeChanged();
    emit hpfCutoffChanged();
    emit filterKeyTrackChanged();

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
    emit portamentoChanged();
    emit panSpreadChanged();
    emit pitchBendRangeChanged();

    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();

    emit oscillatorDriftChanged();
    emit oscillatorInstabilityChanged();
    emit filterDriveChanged();
    emit crossModDepthChanged();
    emit delayTypeChanged();
    emit delayTimeChanged();
    emit delayFeedbackChanged();
    emit delayDepthChanged();
    emit delayMixChanged();
    emit chorusRateChanged();
    emit chorusDepthChanged();
    emit chorusDelayChanged();
    emit chorusWidthChanged();
    emit chorusMixChanged();
    emit delaySyncChanged();
    emit delaySyncDivisionChanged();
    emit delayFeedbackLpfChanged();
    emit delayFeedbackHpfChanged();
}

void SynthDialogModel::loadFactoryPreset(int index)
{
    if (m_synth) {
        m_synth->loadPreset(index);
        requestSettings();
    }
}

void SynthDialogModel::setDeviceService(std::shared_ptr<DeviceService> deviceService)
{
    m_deviceService = std::move(deviceService);
}

void SynthDialogModel::setSynth(std::shared_ptr<SynthDevice> synth)
{
    if (m_synth != synth) {
        if (m_synth) {
            disconnect(m_synth.get(), nullptr, this, nullptr);
        }
        m_synth = std::move(synth);
        connectDeviceSignals();
        // Keep the oscilloscope bound to the shown instance: disable it on the previous device and
        // re-apply the desired state to the new one.
        applyScopeActive();
        emit synthChanged();
        requestSettings();
    }
}

// VCO3
int SynthDialogModel::vco3Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->vco3Waveform()) : 0;
}

void SynthDialogModel::setVco3Waveform(int wave)
{
    if (m_synth) {
        m_synth->setVco3Waveform(static_cast<PolyBlepOscillator::Waveform>(wave));
    }
}

int SynthDialogModel::vco3Octave() const
{
    return m_synth ? m_synth->vco3Octave() : 0;
}

void SynthDialogModel::setVco3Octave(int oct)
{
    if (m_synth) {
        m_synth->setVco3Octave(oct);
    }
}

int SynthDialogModel::vco3Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco3Pitch() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco3Pitch(int p)
{
    if (m_synth) {
        m_synth->setVco3Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vco3Shape() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco3Shape() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco3Shape(int s)
{
    if (m_synth) {
        m_synth->setVco3Shape(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vco3Roundness() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco3Roundness() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco3Roundness(int r)
{
    if (m_synth) {
        m_synth->setVco3Roundness(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

bool SynthDialogModel::vco3Sync() const
{
    return m_synth ? m_synth->vco3Sync() : false;
}

void SynthDialogModel::setVco3Sync(bool s)
{
    if (m_synth) {
        m_synth->setVco3Sync(s);
    }
}

int SynthDialogModel::mixVco3() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->mixVco3() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMixVco3(int lvl)
{
    if (m_synth) {
        m_synth->setMixVco3(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vco4Waveform() const
{
    return m_synth ? static_cast<int>(m_synth->vco4Waveform()) : 0;
}

void SynthDialogModel::setVco4Waveform(int wave)
{
    if (m_synth) {
        m_synth->setVco4Waveform(static_cast<PolyBlepOscillator::Waveform>(wave));
    }
}

int SynthDialogModel::vco4Octave() const
{
    return m_synth ? m_synth->vco4Octave() : 0;
}

void SynthDialogModel::setVco4Octave(int oct)
{
    if (m_synth) {
        m_synth->setVco4Octave(oct);
    }
}

int SynthDialogModel::vco4Pitch() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco4Pitch() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco4Pitch(int p)
{
    if (m_synth) {
        m_synth->setVco4Pitch(static_cast<float>(p) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vco4Shape() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco4Shape() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco4Shape(int s)
{
    if (m_synth) {
        m_synth->setVco4Shape(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int SynthDialogModel::vco4Roundness() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->vco4Roundness() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setVco4Roundness(int r)
{
    if (m_synth) {
        m_synth->setVco4Roundness(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

bool SynthDialogModel::vco4Sync() const
{
    return m_synth ? m_synth->vco4Sync() : false;
}

void SynthDialogModel::setVco4Sync(bool s)
{
    if (m_synth) {
        m_synth->setVco4Sync(s);
    }
}

int SynthDialogModel::mixVco4() const
{
    return m_synth ? static_cast<int>(std::round(m_synth->mixVco4() * Constants::uiInternalScaling())) : 0;
}

void SynthDialogModel::setMixVco4(int lvl)
{
    if (m_synth) {
        m_synth->setMixVco4(static_cast<float>(lvl) / Constants::uiInternalScaling());
    }
}

} // namespace noteahead
