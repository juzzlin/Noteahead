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

#include "synth_d_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "synth_d_device.hpp"
#include "synth_d_presets.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

SynthDDialogModel::SynthDDialogModel(std::shared_ptr<SynthDDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

SynthDDialogModel::~SynthDDialogModel() = default;

DeviceDialogModel::DeviceS SynthDDialogModel::device() const
{
    return m_device;
}

bool SynthDDialogModel::setDevice(DeviceDialogModel::DeviceS device)
{
    if (const auto synth = std::dynamic_pointer_cast<SynthDDevice>(device)) {
        setDevice(synth);
        return true;
    }
    return false;
}

void SynthDDialogModel::setDevice(std::shared_ptr<SynthDDevice> device)
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

QStringList SynthDDialogModel::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : SynthDPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void SynthDDialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int SynthDDialogModel::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void SynthDDialogModel::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDDialogModel::choice(const QString & key) const
{
    return m_device ? m_device->choice(key) : 0;
}

void SynthDDialogModel::setChoice(const QString & key, int value)
{
    if (m_device) {
        m_device->setChoice(key, value);
    }
}

int SynthDDialogModel::tune() const
{
    return knob(Xml::xmlKeyTune());
}

void SynthDDialogModel::setTune(int value)
{
    setKnob(Xml::xmlKeyTune(), value);
}

int SynthDDialogModel::glide() const
{
    return knob(Xml::xmlKeyGlide());
}

void SynthDDialogModel::setGlide(int value)
{
    setKnob(Xml::xmlKeyGlide(), value);
}

int SynthDDialogModel::modMix() const
{
    return knob(Xml::xmlKeyModMix());
}

void SynthDDialogModel::setModMix(int value)
{
    setKnob(Xml::xmlKeyModMix(), value);
}

int SynthDDialogModel::modDepth() const
{
    return knob(Xml::xmlKeyModDepth());
}

void SynthDDialogModel::setModDepth(int value)
{
    setKnob(Xml::xmlKeyModDepth(), value);
}

int SynthDDialogModel::modSource1() const
{
    return choice(Xml::xmlKeyModSource1());
}

void SynthDDialogModel::setModSource1(int value)
{
    setChoice(Xml::xmlKeyModSource1(), value);
}

int SynthDDialogModel::modSource2() const
{
    return choice(Xml::xmlKeyModSource2());
}

void SynthDDialogModel::setModSource2(int value)
{
    setChoice(Xml::xmlKeyModSource2(), value);
}

bool SynthDDialogModel::oscillatorModulation() const
{
    return choice(Xml::xmlKeyOscillatorModulation()) == 1;
}

void SynthDDialogModel::setOscillatorModulation(bool value)
{
    setChoice(Xml::xmlKeyOscillatorModulation(), value ? 1 : 0);
}

bool SynthDDialogModel::filterModulation() const
{
    return choice(Xml::xmlKeyFilterModulation()) == 1;
}

void SynthDDialogModel::setFilterModulation(bool value)
{
    setChoice(Xml::xmlKeyFilterModulation(), value ? 1 : 0);
}

int SynthDDialogModel::lfoWaveform() const
{
    return choice(Xml::xmlKeyLfoWaveform());
}

void SynthDDialogModel::setLfoWaveform(int value)
{
    setChoice(Xml::xmlKeyLfoWaveform(), value);
}

int SynthDDialogModel::lfoRate() const
{
    return knob(Xml::xmlKeyLfoRate());
}

void SynthDDialogModel::setLfoRate(int value)
{
    setKnob(Xml::xmlKeyLfoRate(), value);
}

int SynthDDialogModel::osc1Range() const
{
    return choice(Xml::xmlKeyVco1Range());
}

void SynthDDialogModel::setOsc1Range(int value)
{
    setChoice(Xml::xmlKeyVco1Range(), value);
}

int SynthDDialogModel::osc2Range() const
{
    return choice(Xml::xmlKeyVco2Range());
}

void SynthDDialogModel::setOsc2Range(int value)
{
    setChoice(Xml::xmlKeyVco2Range(), value);
}

int SynthDDialogModel::osc3Range() const
{
    return choice(Xml::xmlKeyVco3Range());
}

void SynthDDialogModel::setOsc3Range(int value)
{
    setChoice(Xml::xmlKeyVco3Range(), value);
}

int SynthDDialogModel::osc1Waveform() const
{
    return choice(Xml::xmlKeyVco1Waveform());
}

void SynthDDialogModel::setOsc1Waveform(int value)
{
    setChoice(Xml::xmlKeyVco1Waveform(), value);
}

int SynthDDialogModel::osc2Waveform() const
{
    return choice(Xml::xmlKeyVco2Waveform());
}

void SynthDDialogModel::setOsc2Waveform(int value)
{
    setChoice(Xml::xmlKeyVco2Waveform(), value);
}

int SynthDDialogModel::osc3Waveform() const
{
    return choice(Xml::xmlKeyVco3Waveform());
}

void SynthDDialogModel::setOsc3Waveform(int value)
{
    setChoice(Xml::xmlKeyVco3Waveform(), value);
}

int SynthDDialogModel::osc2Frequency() const
{
    return knob(Xml::xmlKeyVco2Pitch());
}

void SynthDDialogModel::setOsc2Frequency(int value)
{
    setKnob(Xml::xmlKeyVco2Pitch(), value);
}

int SynthDDialogModel::osc3Frequency() const
{
    return knob(Xml::xmlKeyVco3Pitch());
}

void SynthDDialogModel::setOsc3Frequency(int value)
{
    setKnob(Xml::xmlKeyVco3Pitch(), value);
}

bool SynthDDialogModel::osc3KeyboardControl() const
{
    return choice(Xml::xmlKeyVco3KeyboardControl()) == 1;
}

void SynthDDialogModel::setOsc3KeyboardControl(bool value)
{
    setChoice(Xml::xmlKeyVco3KeyboardControl(), value ? 1 : 0);
}

int SynthDDialogModel::osc1Volume() const
{
    return knob(Xml::xmlKeyVco1Level());
}

void SynthDDialogModel::setOsc1Volume(int value)
{
    setKnob(Xml::xmlKeyVco1Level(), value);
}

int SynthDDialogModel::osc2Volume() const
{
    return knob(Xml::xmlKeyVco2Level());
}

void SynthDDialogModel::setOsc2Volume(int value)
{
    setKnob(Xml::xmlKeyVco2Level(), value);
}

int SynthDDialogModel::osc3Volume() const
{
    return knob(Xml::xmlKeyVco3Level());
}

void SynthDDialogModel::setOsc3Volume(int value)
{
    setKnob(Xml::xmlKeyVco3Level(), value);
}

bool SynthDDialogModel::osc1On() const
{
    return choice(Xml::xmlKeyVco1On()) == 1;
}

void SynthDDialogModel::setOsc1On(bool value)
{
    setChoice(Xml::xmlKeyVco1On(), value ? 1 : 0);
}

bool SynthDDialogModel::osc2On() const
{
    return choice(Xml::xmlKeyVco2On()) == 1;
}

void SynthDDialogModel::setOsc2On(bool value)
{
    setChoice(Xml::xmlKeyVco2On(), value ? 1 : 0);
}

bool SynthDDialogModel::osc3On() const
{
    return choice(Xml::xmlKeyVco3On()) == 1;
}

void SynthDDialogModel::setOsc3On(bool value)
{
    setChoice(Xml::xmlKeyVco3On(), value ? 1 : 0);
}

int SynthDDialogModel::extVolume() const
{
    return knob(Xml::xmlKeyFeedback());
}

void SynthDDialogModel::setExtVolume(int value)
{
    setKnob(Xml::xmlKeyFeedback(), value);
}

bool SynthDDialogModel::extOn() const
{
    return choice(Xml::xmlKeyFeedbackOn()) == 1;
}

void SynthDDialogModel::setExtOn(bool value)
{
    setChoice(Xml::xmlKeyFeedbackOn(), value ? 1 : 0);
}

int SynthDDialogModel::noiseVolume() const
{
    return knob(Xml::xmlKeyNoiseLevel());
}

void SynthDDialogModel::setNoiseVolume(int value)
{
    setKnob(Xml::xmlKeyNoiseLevel(), value);
}

bool SynthDDialogModel::noiseOn() const
{
    return choice(Xml::xmlKeyNoiseOn()) == 1;
}

void SynthDDialogModel::setNoiseOn(bool value)
{
    setChoice(Xml::xmlKeyNoiseOn(), value ? 1 : 0);
}

int SynthDDialogModel::noiseColour() const
{
    return choice(Xml::xmlKeyColor());
}

void SynthDDialogModel::setNoiseColour(int value)
{
    setChoice(Xml::xmlKeyColor(), value);
}

int SynthDDialogModel::filterMode() const
{
    return choice(Xml::xmlKeyFilterType());
}

void SynthDDialogModel::setFilterMode(int value)
{
    setChoice(Xml::xmlKeyFilterType(), value);
}

int SynthDDialogModel::cutoff() const
{
    return knob(Xml::xmlKeyCutoff());
}

void SynthDDialogModel::setCutoff(int value)
{
    setKnob(Xml::xmlKeyCutoff(), value);
}

int SynthDDialogModel::emphasis() const
{
    return knob(Xml::xmlKeyResonance());
}

void SynthDDialogModel::setEmphasis(int value)
{
    setKnob(Xml::xmlKeyResonance(), value);
}

int SynthDDialogModel::contourAmount() const
{
    return knob(Xml::xmlKeyEnvMod());
}

void SynthDDialogModel::setContourAmount(int value)
{
    setKnob(Xml::xmlKeyEnvMod(), value);
}

bool SynthDDialogModel::keyboardControl1() const
{
    return choice(Xml::xmlKeyKeyboardControl1()) == 1;
}

void SynthDDialogModel::setKeyboardControl1(bool value)
{
    setChoice(Xml::xmlKeyKeyboardControl1(), value ? 1 : 0);
}

bool SynthDDialogModel::keyboardControl2() const
{
    return choice(Xml::xmlKeyKeyboardControl2()) == 1;
}

void SynthDDialogModel::setKeyboardControl2(bool value)
{
    setChoice(Xml::xmlKeyKeyboardControl2(), value ? 1 : 0);
}

int SynthDDialogModel::filterAttack() const
{
    return knob(Xml::xmlKeyEnv1Attack());
}

void SynthDDialogModel::setFilterAttack(int value)
{
    setKnob(Xml::xmlKeyEnv1Attack(), value);
}

int SynthDDialogModel::filterDecay() const
{
    return knob(Xml::xmlKeyEnv1Decay());
}

void SynthDDialogModel::setFilterDecay(int value)
{
    setKnob(Xml::xmlKeyEnv1Decay(), value);
}

int SynthDDialogModel::filterSustain() const
{
    return knob(Xml::xmlKeyEnv1Sustain());
}

void SynthDDialogModel::setFilterSustain(int value)
{
    setKnob(Xml::xmlKeyEnv1Sustain(), value);
}

bool SynthDDialogModel::filterDecaySwitch() const
{
    return choice(Xml::xmlKeyEnv1DecaySwitch()) == 1;
}

void SynthDDialogModel::setFilterDecaySwitch(bool value)
{
    setChoice(Xml::xmlKeyEnv1DecaySwitch(), value ? 1 : 0);
}

int SynthDDialogModel::loudnessAttack() const
{
    return knob(Xml::xmlKeyEnv2Attack());
}

void SynthDDialogModel::setLoudnessAttack(int value)
{
    setKnob(Xml::xmlKeyEnv2Attack(), value);
}

int SynthDDialogModel::loudnessDecay() const
{
    return knob(Xml::xmlKeyEnv2Decay());
}

void SynthDDialogModel::setLoudnessDecay(int value)
{
    setKnob(Xml::xmlKeyEnv2Decay(), value);
}

int SynthDDialogModel::loudnessSustain() const
{
    return knob(Xml::xmlKeyEnv2Sustain());
}

void SynthDDialogModel::setLoudnessSustain(int value)
{
    setKnob(Xml::xmlKeyEnv2Sustain(), value);
}

bool SynthDDialogModel::loudnessDecaySwitch() const
{
    return choice(Xml::xmlKeyEnv2DecaySwitch()) == 1;
}

void SynthDDialogModel::setLoudnessDecaySwitch(bool value)
{
    setChoice(Xml::xmlKeyEnv2DecaySwitch(), value ? 1 : 0);
}

int SynthDDialogModel::notePriority() const
{
    return choice(Xml::xmlKeyNotePriority());
}

void SynthDDialogModel::setNotePriority(int value)
{
    setChoice(Xml::xmlKeyNotePriority(), value);
}

bool SynthDDialogModel::multiTrigger() const
{
    return choice(Xml::xmlKeyRetrigger()) == 1;
}

void SynthDDialogModel::setMultiTrigger(bool value)
{
    setChoice(Xml::xmlKeyRetrigger(), value ? 1 : 0);
}

int SynthDDialogModel::pitchBendRange() const
{
    return choice(Xml::xmlKeyPitchBendRange());
}

void SynthDDialogModel::setPitchBendRange(int value)
{
    setChoice(Xml::xmlKeyPitchBendRange(), value);
}

int SynthDDialogModel::drift() const
{
    return knob(Xml::xmlKeyOscillatorDrift());
}

void SynthDDialogModel::setDrift(int value)
{
    setKnob(Xml::xmlKeyOscillatorDrift(), value);
}

int SynthDDialogModel::instability() const
{
    return knob(Xml::xmlKeyOscillatorInstability());
}

void SynthDDialogModel::setInstability(int value)
{
    setKnob(Xml::xmlKeyOscillatorInstability(), value);
}

int SynthDDialogModel::tolerance() const
{
    return knob(Xml::xmlKeyVoiceTolerance());
}

void SynthDDialogModel::setTolerance(int value)
{
    setKnob(Xml::xmlKeyVoiceTolerance(), value);
}

int SynthDDialogModel::panSpread() const
{
    return knob(Xml::xmlKeyPanSpread());
}

void SynthDDialogModel::setPanSpread(int value)
{
    setKnob(Xml::xmlKeyPanSpread(), value);
}

int SynthDDialogModel::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void SynthDDialogModel::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int SynthDDialogModel::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void SynthDDialogModel::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int SynthDDialogModel::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void SynthDDialogModel::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

void SynthDDialogModel::requestSettings()
{
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit controlsChanged();
}

} // namespace noteahead
