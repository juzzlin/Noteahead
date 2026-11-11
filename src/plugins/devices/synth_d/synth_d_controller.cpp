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

#include "synth_d_controller.hpp"

#include "../../../common/constants.hpp"
#include "synth_d_device.hpp"
#include "synth_d_presets.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

SynthDController::SynthDController(std::shared_ptr<SynthDDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

SynthDController::~SynthDController() = default;

DeviceController::DeviceS SynthDController::device() const
{
    return m_device;
}

bool SynthDController::setDevice(DeviceController::DeviceS device)
{
    if (const auto synth = std::dynamic_pointer_cast<SynthDDevice>(device)) {
        setDevice(synth);
        return true;
    }
    return false;
}

void SynthDController::setDevice(std::shared_ptr<SynthDDevice> device)
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

QStringList SynthDController::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : SynthDPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void SynthDController::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int SynthDController::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void SynthDController::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SynthDController::choice(const QString & key) const
{
    return m_device ? m_device->choice(key) : 0;
}

void SynthDController::setChoice(const QString & key, int value)
{
    if (m_device) {
        m_device->setChoice(key, value);
    }
}

int SynthDController::tune() const
{
    return knob(Xml::xmlKeyTune());
}

void SynthDController::setTune(int value)
{
    setKnob(Xml::xmlKeyTune(), value);
}

int SynthDController::glide() const
{
    return knob(Xml::xmlKeyGlide());
}

void SynthDController::setGlide(int value)
{
    setKnob(Xml::xmlKeyGlide(), value);
}

int SynthDController::modMix() const
{
    return knob(Xml::xmlKeyModMix());
}

void SynthDController::setModMix(int value)
{
    setKnob(Xml::xmlKeyModMix(), value);
}

int SynthDController::modDepth() const
{
    return knob(Xml::xmlKeyModDepth());
}

void SynthDController::setModDepth(int value)
{
    setKnob(Xml::xmlKeyModDepth(), value);
}

int SynthDController::modSource1() const
{
    return choice(Xml::xmlKeyModSource1());
}

void SynthDController::setModSource1(int value)
{
    setChoice(Xml::xmlKeyModSource1(), value);
}

int SynthDController::modSource2() const
{
    return choice(Xml::xmlKeyModSource2());
}

void SynthDController::setModSource2(int value)
{
    setChoice(Xml::xmlKeyModSource2(), value);
}

bool SynthDController::oscillatorModulation() const
{
    return choice(Xml::xmlKeyOscillatorModulation()) == 1;
}

void SynthDController::setOscillatorModulation(bool value)
{
    setChoice(Xml::xmlKeyOscillatorModulation(), value ? 1 : 0);
}

bool SynthDController::filterModulation() const
{
    return choice(Xml::xmlKeyFilterModulation()) == 1;
}

void SynthDController::setFilterModulation(bool value)
{
    setChoice(Xml::xmlKeyFilterModulation(), value ? 1 : 0);
}

int SynthDController::lfoWaveform() const
{
    return choice(Xml::xmlKeyLfoWaveform());
}

void SynthDController::setLfoWaveform(int value)
{
    setChoice(Xml::xmlKeyLfoWaveform(), value);
}

int SynthDController::lfoRate() const
{
    return knob(Xml::xmlKeyLfoRate());
}

void SynthDController::setLfoRate(int value)
{
    setKnob(Xml::xmlKeyLfoRate(), value);
}

int SynthDController::osc1Range() const
{
    return choice(Xml::xmlKeyVco1Range());
}

void SynthDController::setOsc1Range(int value)
{
    setChoice(Xml::xmlKeyVco1Range(), value);
}

int SynthDController::osc2Range() const
{
    return choice(Xml::xmlKeyVco2Range());
}

void SynthDController::setOsc2Range(int value)
{
    setChoice(Xml::xmlKeyVco2Range(), value);
}

int SynthDController::osc3Range() const
{
    return choice(Xml::xmlKeyVco3Range());
}

void SynthDController::setOsc3Range(int value)
{
    setChoice(Xml::xmlKeyVco3Range(), value);
}

int SynthDController::osc1Waveform() const
{
    return choice(Xml::xmlKeyVco1Waveform());
}

void SynthDController::setOsc1Waveform(int value)
{
    setChoice(Xml::xmlKeyVco1Waveform(), value);
}

int SynthDController::osc2Waveform() const
{
    return choice(Xml::xmlKeyVco2Waveform());
}

void SynthDController::setOsc2Waveform(int value)
{
    setChoice(Xml::xmlKeyVco2Waveform(), value);
}

int SynthDController::osc3Waveform() const
{
    return choice(Xml::xmlKeyVco3Waveform());
}

void SynthDController::setOsc3Waveform(int value)
{
    setChoice(Xml::xmlKeyVco3Waveform(), value);
}

int SynthDController::osc2Frequency() const
{
    return knob(Xml::xmlKeyVco2Pitch());
}

void SynthDController::setOsc2Frequency(int value)
{
    setKnob(Xml::xmlKeyVco2Pitch(), value);
}

int SynthDController::osc3Frequency() const
{
    return knob(Xml::xmlKeyVco3Pitch());
}

void SynthDController::setOsc3Frequency(int value)
{
    setKnob(Xml::xmlKeyVco3Pitch(), value);
}

bool SynthDController::osc3KeyboardControl() const
{
    return choice(Xml::xmlKeyVco3KeyboardControl()) == 1;
}

void SynthDController::setOsc3KeyboardControl(bool value)
{
    setChoice(Xml::xmlKeyVco3KeyboardControl(), value ? 1 : 0);
}

int SynthDController::osc1Volume() const
{
    return knob(Xml::xmlKeyVco1Level());
}

void SynthDController::setOsc1Volume(int value)
{
    setKnob(Xml::xmlKeyVco1Level(), value);
}

int SynthDController::osc2Volume() const
{
    return knob(Xml::xmlKeyVco2Level());
}

void SynthDController::setOsc2Volume(int value)
{
    setKnob(Xml::xmlKeyVco2Level(), value);
}

int SynthDController::osc3Volume() const
{
    return knob(Xml::xmlKeyVco3Level());
}

void SynthDController::setOsc3Volume(int value)
{
    setKnob(Xml::xmlKeyVco3Level(), value);
}

bool SynthDController::osc1On() const
{
    return choice(Xml::xmlKeyVco1On()) == 1;
}

void SynthDController::setOsc1On(bool value)
{
    setChoice(Xml::xmlKeyVco1On(), value ? 1 : 0);
}

bool SynthDController::osc2On() const
{
    return choice(Xml::xmlKeyVco2On()) == 1;
}

void SynthDController::setOsc2On(bool value)
{
    setChoice(Xml::xmlKeyVco2On(), value ? 1 : 0);
}

bool SynthDController::osc3On() const
{
    return choice(Xml::xmlKeyVco3On()) == 1;
}

void SynthDController::setOsc3On(bool value)
{
    setChoice(Xml::xmlKeyVco3On(), value ? 1 : 0);
}

int SynthDController::extVolume() const
{
    return knob(Xml::xmlKeyFeedback());
}

void SynthDController::setExtVolume(int value)
{
    setKnob(Xml::xmlKeyFeedback(), value);
}

bool SynthDController::extOn() const
{
    return choice(Xml::xmlKeyFeedbackOn()) == 1;
}

void SynthDController::setExtOn(bool value)
{
    setChoice(Xml::xmlKeyFeedbackOn(), value ? 1 : 0);
}

int SynthDController::noiseVolume() const
{
    return knob(Xml::xmlKeyNoiseLevel());
}

void SynthDController::setNoiseVolume(int value)
{
    setKnob(Xml::xmlKeyNoiseLevel(), value);
}

bool SynthDController::noiseOn() const
{
    return choice(Xml::xmlKeyNoiseOn()) == 1;
}

void SynthDController::setNoiseOn(bool value)
{
    setChoice(Xml::xmlKeyNoiseOn(), value ? 1 : 0);
}

int SynthDController::noiseColour() const
{
    return choice(Xml::xmlKeyColor());
}

void SynthDController::setNoiseColour(int value)
{
    setChoice(Xml::xmlKeyColor(), value);
}

int SynthDController::filterMode() const
{
    return choice(Xml::xmlKeyFilterType());
}

void SynthDController::setFilterMode(int value)
{
    setChoice(Xml::xmlKeyFilterType(), value);
}

int SynthDController::cutoff() const
{
    return knob(Xml::xmlKeyCutoff());
}

void SynthDController::setCutoff(int value)
{
    setKnob(Xml::xmlKeyCutoff(), value);
}

int SynthDController::emphasis() const
{
    return knob(Xml::xmlKeyResonance());
}

void SynthDController::setEmphasis(int value)
{
    setKnob(Xml::xmlKeyResonance(), value);
}

int SynthDController::contourAmount() const
{
    return knob(Xml::xmlKeyEnvMod());
}

void SynthDController::setContourAmount(int value)
{
    setKnob(Xml::xmlKeyEnvMod(), value);
}

bool SynthDController::keyboardControl1() const
{
    return choice(Xml::xmlKeyKeyboardControl1()) == 1;
}

void SynthDController::setKeyboardControl1(bool value)
{
    setChoice(Xml::xmlKeyKeyboardControl1(), value ? 1 : 0);
}

bool SynthDController::keyboardControl2() const
{
    return choice(Xml::xmlKeyKeyboardControl2()) == 1;
}

void SynthDController::setKeyboardControl2(bool value)
{
    setChoice(Xml::xmlKeyKeyboardControl2(), value ? 1 : 0);
}

int SynthDController::filterAttack() const
{
    return knob(Xml::xmlKeyEnv1Attack());
}

void SynthDController::setFilterAttack(int value)
{
    setKnob(Xml::xmlKeyEnv1Attack(), value);
}

int SynthDController::filterDecay() const
{
    return knob(Xml::xmlKeyEnv1Decay());
}

void SynthDController::setFilterDecay(int value)
{
    setKnob(Xml::xmlKeyEnv1Decay(), value);
}

int SynthDController::filterSustain() const
{
    return knob(Xml::xmlKeyEnv1Sustain());
}

void SynthDController::setFilterSustain(int value)
{
    setKnob(Xml::xmlKeyEnv1Sustain(), value);
}

bool SynthDController::filterDecaySwitch() const
{
    return choice(Xml::xmlKeyEnv1DecaySwitch()) == 1;
}

void SynthDController::setFilterDecaySwitch(bool value)
{
    setChoice(Xml::xmlKeyEnv1DecaySwitch(), value ? 1 : 0);
}

int SynthDController::loudnessAttack() const
{
    return knob(Xml::xmlKeyEnv2Attack());
}

void SynthDController::setLoudnessAttack(int value)
{
    setKnob(Xml::xmlKeyEnv2Attack(), value);
}

int SynthDController::loudnessDecay() const
{
    return knob(Xml::xmlKeyEnv2Decay());
}

void SynthDController::setLoudnessDecay(int value)
{
    setKnob(Xml::xmlKeyEnv2Decay(), value);
}

int SynthDController::loudnessSustain() const
{
    return knob(Xml::xmlKeyEnv2Sustain());
}

void SynthDController::setLoudnessSustain(int value)
{
    setKnob(Xml::xmlKeyEnv2Sustain(), value);
}

bool SynthDController::loudnessDecaySwitch() const
{
    return choice(Xml::xmlKeyEnv2DecaySwitch()) == 1;
}

void SynthDController::setLoudnessDecaySwitch(bool value)
{
    setChoice(Xml::xmlKeyEnv2DecaySwitch(), value ? 1 : 0);
}

int SynthDController::notePriority() const
{
    return choice(Xml::xmlKeyNotePriority());
}

void SynthDController::setNotePriority(int value)
{
    setChoice(Xml::xmlKeyNotePriority(), value);
}

bool SynthDController::multiTrigger() const
{
    return choice(Xml::xmlKeyRetrigger()) == 1;
}

void SynthDController::setMultiTrigger(bool value)
{
    setChoice(Xml::xmlKeyRetrigger(), value ? 1 : 0);
}

int SynthDController::pitchBendRange() const
{
    return choice(Xml::xmlKeyPitchBendRange());
}

void SynthDController::setPitchBendRange(int value)
{
    setChoice(Xml::xmlKeyPitchBendRange(), value);
}

int SynthDController::drift() const
{
    return knob(Xml::xmlKeyOscillatorDrift());
}

void SynthDController::setDrift(int value)
{
    setKnob(Xml::xmlKeyOscillatorDrift(), value);
}

int SynthDController::instability() const
{
    return knob(Xml::xmlKeyOscillatorInstability());
}

void SynthDController::setInstability(int value)
{
    setKnob(Xml::xmlKeyOscillatorInstability(), value);
}

int SynthDController::tolerance() const
{
    return knob(Xml::xmlKeyVoiceTolerance());
}

void SynthDController::setTolerance(int value)
{
    setKnob(Xml::xmlKeyVoiceTolerance(), value);
}

int SynthDController::panSpread() const
{
    return knob(Xml::xmlKeyPanSpread());
}

void SynthDController::setPanSpread(int value)
{
    setKnob(Xml::xmlKeyPanSpread(), value);
}

int SynthDController::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void SynthDController::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int SynthDController::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void SynthDController::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int SynthDController::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void SynthDController::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

void SynthDController::requestSettings()
{
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit controlsChanged();
}

} // namespace noteahead
