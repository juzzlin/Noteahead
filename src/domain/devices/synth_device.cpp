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

#include "synth_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "synth_presets.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

void SynthDevice::Voice::reset()
{
    active = false;
    vco1.sync(0.0);
    vco2.sync(0.0);
    vco3.sync(0.0);
    lpf.reset();
    hpf.reset();
    ampEg.reset();
    modEg.reset();
    lfo.reset();
    lfo2.reset();
    unisonDamp.reset();
    glideFrequency = 0.0;
    active = false;
}

void SynthDevice::Voice::trigger(uint8_t n, double freq, float p, float v, bool phaseSync, uint64_t tid)
{
    note = n;
    triggerId = tid;
    frequency = freq;
    if (glideFrequency == 0.0) {
        glideFrequency = freq;
    }
    pan = p;
    velocity = v;

    lfo.reset();
    lfo2.reset();

    // Only sync oscillator phase on a fresh (idle) voice to avoid a pop from a
    // hard phase jump while the voice is still producing audio (retrigger/steal).
    if (phaseSync && !active) {
        vco1.sync(0.0);
        vco2.sync(0.0);
        vco3.sync(0.0);
    }

    active = true;
    ampEg.trigger();
    modEg.trigger();
}

void SynthDevice::Voice::triggerRandomized(uint8_t n, double freq, float p, float v, double randomPhase, uint64_t tid)
{
    note = n;
    triggerId = tid;
    frequency = freq;
    if (glideFrequency == 0.0) {
        glideFrequency = freq;
    }
    pan = p;
    velocity = v;

    lfo.reset();
    lfo2.reset();

    // Only sync oscillator phase on a fresh (idle) voice to avoid a pop from a
    // hard phase jump while the voice is still producing audio (retrigger/steal).
    if (!active) {
        vco1.sync(randomPhase);
        vco2.sync(std::fmod(randomPhase + 0.33, 1.0));
        vco3.sync(std::fmod(randomPhase + 0.66, 1.0));
    }

    active = true;
    ampEg.trigger();
    modEg.trigger();
}

void SynthDevice::Voice::release()
{
    ampEg.release();
    modEg.release();
}

SynthDevice::SynthDevice(std::string name)
  : m_name { std::move(name) }
{
    m_voices.resize(MaxVoices);

    // Initialize Parameters
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco1Waveform().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco1Octave().toStdString(), 0.0f, -1, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco1Pitch().toStdString(), 0.5f, -2400, 2400, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco1Shape().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco1Sync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVco2Waveform().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco2Octave().toStdString(), 0.0f, -1, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco2Pitch().toStdString(), 0.5f, -2400, 2400, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco2Shape().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco2Sync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVco3Waveform().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco3Octave().toStdString(), 0.0f, -1, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco3Pitch().toStdString(), 0.5f, -2400, 2400, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco3Shape().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVco3Sync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeyMultiMode().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete }); // Low default
    addParameter(Parameter { Constants::NahdXml::xmlKeyMultiShape().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMultiLevel().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMultiKeyTrack().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyMixLevel1().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMixLevel2().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMixLevel3().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfResonance().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyKeyTrack().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpAttack().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpDecay().toStdString(), 0.34f, 0, 10000, 3400, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpRelease().toStdString(), 0.48f, 0, 10000, 4800, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpVelocitySensitivity().toStdString(), 0.5f, 0, 10000, 10000, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyModAttack().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModDecay().toStdString(), 0.34f, 0, 10000, 3400, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModSustain().toStdString(), 0.0f, 0, 10000, 0, 100 }); // AD by default
    addParameter(Parameter { Constants::NahdXml::xmlKeyModIntensity().toStdString(), 0.5f, -10000, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModTarget().toStdString(), 3.0f, 0, 3, 3, 1, Parameter::Type::Discrete }); // Cutoff default

    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoWaveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete }); // Tri default
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete }); // Normal default
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoRate().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoIntensity().toStdString(), 0.5f, -10000, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoTarget().toStdString(), 0.0f, 0, 5, 0, 1, Parameter::Type::Discrete }); // Pitch default

    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Waveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Mode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Rate().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Intensity().toStdString(), 0.5f, -10000, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Target().toStdString(), 0.0f, 0, 5, 0, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMode().toStdString(), 0.0f, 0, 5, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceDepth().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPortamento().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPitchBendRange().toStdString(), 2.0f, 0, 24, 2, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyOscillatorDrift().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCrossModDepth().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayType().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayTime().toStdString(), 0.5f, 0, 10000, 500 }); // 0..10 seconds in ms
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedback().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayDepth().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayMix().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelaySync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString(), 0.25f, 0, 10000, 2500, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString(), 0.0f, 0, 10000, 0, 100 });

    // Prime-ratio drift rates so each voice drifts independently without coherent beating
    static constexpr std::array<double, MaxVoices> driftRates = { 0.10, 0.17, 0.23, 0.29, 0.37, 0.43 };
    for (size_t i = 0; i < m_voices.size(); i++) {
        m_voices[i].lpf.setMode(CascadedSvf::Mode::LowPass);
        m_voices[i].hpf.setMode(CascadedSvf::Mode::HighPass);
        m_voices[i].driftRate = driftRates[i];
        m_voices[i].driftPhase = static_cast<double>(i) / MaxVoices;
    }

    setManualPan(panInternal());
    setManualVolume(volumeInternal());
    setManualGain(gainInternal());
    m_manualLpfCutoff = m_lpfCutoff;
    m_manualLpfResonance = m_lpfResonance;
    m_manualHpfCutoff = m_hpfCutoff;

    SynthDevice::syncParameters();
}

SynthDevice::~SynthDevice() = default;

std::string SynthDevice::name() const
{
    return m_name;
}

std::string SynthDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string SynthDevice::typeName() const
{
    return Constants::synthDeviceName().toStdString();
}

std::string SynthDevice::typeIdString()
{
    return "26f5a47e-4786-11f1-92b0-0b3f3bef9f74";
}

std::string SynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> SynthDevice::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        { static_cast<uint8_t>(Controller::ModulationWheelMSB), "LFO Int" },
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SoundController2), "Resonance" },
        { static_cast<uint8_t>(Controller::SoundController5), "LPF" },
        { static_cast<uint8_t>(Controller::GeneralPurpose6), "HPF" }
    };
}

void SynthDevice::processAudio(AudioContext & context)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    prepareForProcessing(context);

    const uint8_t oversampleFactor = clampOversampleFactor(context.oversampleFactor);
    m_oversampleFactor = oversampleFactor;
    const uint32_t oversampledRate = context.sampleRate * oversampleFactor;

    const double portamentoTime = ParameterMapper::mapExponential(m_portamento, 0.01, 2.0);
    const double portamentoCoeff = m_portamento > 0 ? 1.0 - std::pow(0.001, 1.0 / (portamentoTime * oversampledRate)) : 1.0;
    const double pbOffset = (static_cast<double>(m_pitchBend) - 8192.0) / 8192.0 * m_pitchBendRange;
    const double pbRatio = std::exp2(pbOffset / 12.0);

    for (size_t i = 0; i < m_voices.size(); i++) {
        auto & voice = m_voices.at(i);
        if (voice.active) {
            renderVoice(voice, context, oversampleFactor, oversampledRate, portamentoCoeff, pbRatio, i);
        }
    }

    applyGlobalEffects(context);
}

void SynthDevice::prepareForProcessing(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    m_delay.setSampleRate(static_cast<double>(context.sampleRate));

    const size_t requiredSize = static_cast<size_t>(context.frameCount) * clampOversampleFactor(context.oversampleFactor) * 2;
    if (m_oversampledBuffer.size() < requiredSize) {
        m_oversampledBuffer.resize(requiredSize);
    }
    std::fill(m_oversampledBuffer.begin(), m_oversampledBuffer.begin() + requiredSize, 0.0f);
}

bool SynthDevice::isStacked(VoiceMode mode)
{
    return mode == VoiceMode::Unison || mode == VoiceMode::Supersaw || mode == VoiceMode::Drift;
}

int SynthDevice::voicesPerNote() const
{
    if (isStacked(m_voiceMode)) {
        return MaxVoices;
    }
    return m_voiceMode == VoiceMode::Dual ? 2 : 1;
}

namespace {

//! Relative detune of the JP-8000's seven saws, the spacing that makes a supersaw a supersaw.
//!
//! The point is that they are *not* evenly spaced. Even spacing gives every adjacent pair the same
//! beat rate and every wider pair an exact multiple of it, so the beating lines up into one periodic
//! comb — the buzz that makes plain unison harsh in a dense mix. These offsets never line up.
//! Six voices, so the outermost of the seven is dropped; the centre one is kept, because the whole
//! arrangement hangs off having a voice exactly at pitch.
constexpr std::array<double, SynthDevice::MaxVoices> supersawOffsets { -0.11002313, -0.06288439, -0.01952356, 0.0, 0.01991221, 0.06216538 };

//! Widest detune of the outermost voice, in semitones, at full depth.
constexpr double supersawMaxDetuneSemitones = 0.5;

//! Widest wander of a Drift voice, in cents, at full depth. Deliberately narrower than the Supersaw
//! spread: the movement is what thickens the sound here, not the interval.
constexpr double driftModeMaxCents = 25.0;

//! Level of the centre voice, from Szabo's analysis of the JP-8000. It starts at unity and gives way
//! as the detune opens up.
double supersawCentreGain(double depth)
{
    return -0.55366 * depth + 0.99785;
}

//! Level of every other voice, likewise. Near silent at zero detune, so a closed supersaw collapses
//! to a single clean saw rather than six voices in unison.
double supersawSideGain(double depth)
{
    return -0.73764 * depth * depth + 1.2841 * depth + 0.044372;
}

} // namespace

double SynthDevice::voiceDetuneSemitones(size_t index) const
{
    switch (m_voiceMode) {
    case VoiceMode::Unison:
        return (static_cast<double>(index) - (MaxVoices - 1) / 2.0) * std::pow(m_voiceDepth, 1.5) * 0.2;
    case VoiceMode::Supersaw:
        // Normalized so the outermost voice lands on the stated spread; what matters is the spacing.
        return supersawOffsets.at(index) / std::abs(supersawOffsets.front()) * supersawMaxDetuneSemitones * std::pow(m_voiceDepth, 1.5);
    case VoiceMode::Drift:
        // Nothing fixed: the wander applied per sample in generateVoiceSample is the whole detune.
        return 0.0;
    case VoiceMode::Dual: {
        constexpr double dualDetuneScale = 0.1;
        const double detuneSign = (index % 2 == 0) ? -1.0 : 1.0;
        return detuneSign * std::pow(m_voiceDepth, 1.5) * dualDetuneScale;
    }
    case VoiceMode::Poly:
    case VoiceMode::Mono:
    default:
        return 0.0;
    }
}

float SynthDevice::voiceLevel(size_t index) const
{
    if (m_voiceMode != VoiceMode::Supersaw) {
        return 1.0f;
    }
    const auto depth = static_cast<double>(m_voiceDepth);
    return static_cast<float>(supersawOffsets.at(index) == 0.0 ? supersawCentreGain(depth) : supersawSideGain(depth));
}

float SynthDevice::voiceStackNormalization() const
{
    // Equal-power: detuned voices are mutually uncorrelated for most of their beat cycle, so their
    // power sums and the compensation is the RMS of the level weights. Weighting by the actual
    // levels rather than the raw voice count matters for Supersaw, where a closed-up stack is one
    // voice and a wide one is six; a flat 1/sqrt(N) would leave it 8 dB down when closed.
    double power = 0.0;
    for (int i = 0; i < voicesPerNote(); i++) {
        const double weight = static_cast<double>(voiceLevel(static_cast<size_t>(i)));
        power += weight * weight;
    }
    return power > 0.0 ? static_cast<float>(1.0 / std::sqrt(power)) : 1.0f;
}

float SynthDevice::voiceSpreadPan(size_t slot) const
{
    // Voice-alternating distribution inspired by Behringer DeepMind: consecutive slots swap sides and
    // step in towards the centre, so any prefix of the sequence is still balanced left to right.
    const float side = (slot % 2 == 0) ? -1.0f : 1.0f;
    const float depth = 1.0f - static_cast<float>(slot / 2) * (2.0f / static_cast<float>(MaxVoices));
    return 0.5f + (side * depth * m_panSpread * 0.5f);
}

double SynthDevice::voiceDampingHz(size_t index) const
{
    if (m_voiceMode != VoiceMode::Supersaw && m_voiceMode != VoiceMode::Drift) {
        return 0.0;
    }

    // How far this voice sits from the centre of the stack, 0..1. Supersaw has a real spread to
    // measure; Drift has none, so the voice index stands in for it.
    const double distance = m_voiceMode == VoiceMode::Supersaw
      ? std::abs(supersawOffsets.at(index)) / std::abs(supersawOffsets.front())
      : static_cast<double>(index) / (MaxVoices - 1);

    // Full brightness at the centre, progressively darker outwards, and only as far as the depth
    // knob opens the stack up. The roughness lives in the top octaves, so this is where taking the
    // edge off costs the least character.
    constexpr double brightestHz = 20000.0;
    constexpr double darkestHz = 4000.0;
    const double amount = distance * static_cast<double>(m_voiceDepth);
    return brightestHz + (darkestHz - brightestHz) * amount;
}

void SynthDevice::renderVoice(Voice & voice, AudioContext & context, uint8_t oversampleFactor, uint32_t oversampledRate, double portamentoCoeff, double pbRatio, size_t index)
{
    updateVoiceParameters(voice, oversampledRate, index);

    // The 1/MaxVoices base is the headroom for a full chord: every voice sounding at once reaches
    // full scale. The stack normalization then keeps one note at the same level whatever the voice
    // mode, so switching modes changes the character and not the gain.
    const float gain = (1.0f / static_cast<float>(MaxVoices)) * voiceStackNormalization() * linearGainInternal() * voice.velocity * voiceLevel(index);

    const double dampingHz = voiceDampingHz(index);
    const bool damped = dampingHz > 0.0 && dampingHz < OnePoleFilter::maxCorner(oversampledRate);
    if (damped) {
        voice.unisonDamp.calculate(dampingHz, oversampledRate);
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        for (uint8_t os = 0; os < oversampleFactor; os++) {
            voice.glideFrequency += (voice.frequency - voice.glideFrequency) * portamentoCoeff;

            const ModulationValues mods = calculateModulation(voice);
            float voiceSample = generateVoiceSample(voice, mods, oversampledRate, pbRatio);
            if (damped) {
                voice.unisonDamp.process(static_cast<double>(voiceSample));
                voiceSample = static_cast<float>(voice.unisonDamp.lowPass());
            }
            const float finalHighRateSample = voiceSample * gain;

            const float voicePan = std::clamp(panInternal() + voice.pan - 0.5f + static_cast<float>(mods.panMod) * 0.5f, 0.0f, 1.0f);
            const double panAngle = static_cast<double>(voicePan) * std::numbers::pi * 0.5;
            const float panL = static_cast<float>(std::cos(panAngle));
            const float panR = static_cast<float>(std::sin(panAngle));

            m_oversampledBuffer[(i * oversampleFactor + os) * 2] += finalHighRateSample * panL;
            m_oversampledBuffer[(i * oversampleFactor + os) * 2 + 1] += finalHighRateSample * panR;
        }
    }

    if (voice.ampEg.isSilent()) {
        voice.active = false;
    }
}

void SynthDevice::updateVoiceParameters(Voice & voice, uint32_t oversampledRate, size_t index)
{
    voice.vco1.setSampleRate(oversampledRate);
    voice.vco2.setSampleRate(oversampledRate);
    voice.vco3.setSampleRate(oversampledRate);
    voice.multi.setSampleRate(oversampledRate);
    voice.multi.setOversampleFactor(m_oversampleFactor);
    voice.lpf.setSampleRate(oversampledRate);
    voice.hpf.setSampleRate(oversampledRate);
    voice.ampEg.setSampleRate(oversampledRate);
    voice.modEg.setSampleRate(oversampledRate);
    voice.lfo.setSampleRate(oversampledRate);
    voice.lfo2.setSampleRate(oversampledRate);

    voice.lpf.setResonance(m_lpfResonance);
    voice.vco1.setWaveform(m_vco1Waveform);
    voice.vco2.setWaveform(m_vco2Waveform);
    voice.vco3.setWaveform(m_vco3Waveform);
    voice.vco1.setShape(m_vco1Shape);
    voice.vco2.setShape(m_vco2Shape);
    voice.vco3.setShape(m_vco3Shape);
    voice.multi.setType(m_multiType);
    voice.multi.setShape(m_multiShape);
    voice.multi.setKeyTrack(m_multiKeyTrack);
    voice.multi.setNote(voice.note);

    if (isStacked(m_voiceMode)) {
        voice.frequency = midiNoteToFreq(voice.note) * std::pow(2.0, voiceDetuneSemitones(index) / 12.0);
    } else if (m_voiceMode == VoiceMode::Dual) {
        voice.frequency = midiNoteToFreq(voice.note) * std::pow(2.0, voiceDetuneSemitones(index) / 12.0);
        const float side = (index % 2 == 0) ? -1.0f : 1.0f;
        voice.pan = 0.5f + (side * m_panSpread * 0.5f);
        return;
    }

    // Mono always sounds the same voice, so its position comes from the slot the note was given
    // rather than the voice index, which would pin every note to the same side of the field.
    voice.pan = voiceSpreadPan(m_voiceMode == VoiceMode::Mono ? m_monoPanSlot.value_or(0) : index);
}

void SynthDevice::applyGlobalEffects(AudioContext & context)
{
    const uint8_t oversampleFactor = clampOversampleFactor(context.oversampleFactor);
    std::array<float, 4> highL {};
    std::array<float, 4> highR {};
    for (uint32_t i = 0; i < context.frameCount; i++) {
        for (uint8_t os = 0; os < oversampleFactor; os++) {
            highL[os] = m_oversampledBuffer[(i * oversampleFactor + os) * 2];
            highR[os] = m_oversampledBuffer[(i * oversampleFactor + os) * 2 + 1];
        }

        double l = static_cast<double>(m_downsamplerL.process(highL.data(), oversampleFactor));
        double r = static_cast<double>(m_downsamplerR.process(highR.data(), oversampleFactor));

        m_delay.process(l, r);

        context.buffer[i * 2] += l;
        context.buffer[i * 2 + 1] += r;
    }
}

bool SynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

void SynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void SynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void SynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            m_lpfCutoff = m_manualLpfCutoff;
            m_lpfResonance = m_manualLpfResonance;
            m_hpfCutoff = m_manualHpfCutoff;
            if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
                p->get().setValue(m_lpfCutoff);
            }
            if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfResonance().toStdString()); p) {
                p->get().setValue(m_lpfResonance);
            }
            if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
                p->get().setValue(m_hpfCutoff);
            }
            if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString()); p) {
                m_lfoInt = ParameterMapper::mapCubicCentered((p->get().value() - 0.5f) * 2.0f, -1.0, 1.0);
            }
            updatePanParameter(manualPanInternal(), false);
            updateVolumeParameter(manualVolumeInternal(), false);
            updateGainParameter(manualGainInternal(), false);
            changed = true;
        } else if (controller == static_cast<uint8_t>(Controller::BankSelectMSB)) {
            m_currentBank = std::clamp(static_cast<int>(value), 0, 1); // 0: Factory, 1: User
        } else {
            const float val = static_cast<float>(value) / 127.0f;
            if (controller == static_cast<uint8_t>(Controller::ModulationWheelMSB)) { // LFO intensity (temporary, not saved to param)
                m_lfoInt = val;
                changed = true;
            } else if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController2)) { // Resonance
                m_lpfResonance = val;
                if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfResonance().toStdString()); p) {
                    p->get().setValue(val);
                    syncParameters();
                    changed = true;
                }
            } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) { // Cutoff
                m_lpfCutoff = val;
                if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
                    p->get().setValue(val);
                    syncParameters();
                    changed = true;
                }
            } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) { // HPF Cutoff
                m_hpfCutoff = val;
                if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
                    p->get().setValue(val);
                    syncParameters();
                    changed = true;
                }
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void SynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.release();
        }
    }
}

void SynthDevice::setBpm(float bpm)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_delay.setBpm(bpm);
}

void SynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void SynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.reset();
    }
    m_delay.reset();
    m_downsamplerL.reset();
    m_downsamplerR.reset();
    m_polyNextVoice = 0;
    m_dualNextPair = 0;
    m_monoPanSlot.reset();
    m_nextTriggerId = 1;
}

double SynthDevice::voiceGlideFrequency(size_t index) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return index < m_voices.size() ? m_voices.at(index).glideFrequency : 0.0;
}

void SynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    const double freq = midiNoteToFreq(note);
    const float vel = static_cast<float>(velocity) / 127.0f;
    const float finalVel = std::clamp((1.0f - m_ampVelocitySensitivity) + m_ampVelocitySensitivity * vel, 0.0f, 1.0f);

    if (m_voiceMode == VoiceMode::Mono) {
        // Mono keeps the sounding voice alive on purpose, so the release-first pass below would undo
        // the legato it exists to produce.
        handleMonoNoteOn(note, freq, finalVel);
        return;
    }

    // First release any existing voices for this note to avoid multiple voices for same note in poly
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.release();
        }
    }

    if (m_voiceMode == VoiceMode::Poly) {
        std::optional<size_t> bestVoice;

        // 1. Try to find a voice already playing or releasing this note (Affinity)
        for (size_t i = 0; i < MaxVoices; i++) {
            if (m_voices.at(i).active && m_voices.at(i).note == note) {
                bestVoice = i;
                break;
            }
        }

        // 2. Try to find an idle voice (Round-Robin)
        if (!bestVoice) {
            for (size_t i = 0; i < MaxVoices; i++) {
                const size_t voiceIndex = (m_polyNextVoice + i) % MaxVoices;
                if (!m_voices.at(voiceIndex).active) {
                    bestVoice = voiceIndex;
                    m_polyNextVoice = (voiceIndex + 1) % MaxVoices;
                    break;
                }
            }
        }

        // 3. Steal a voice
        if (!bestVoice) {
            // Prefer voices in Release state (quietest first)
            float lowestAmp = 2.0f;
            for (size_t i = 0; i < MaxVoices; i++) {
                if (m_voices.at(i).ampEg.state() == AdsrEnvelope::State::Release) {
                    const float currentAmp = static_cast<float>(m_voices.at(i).ampEg.value());
                    if (currentAmp < lowestAmp) {
                        lowestAmp = currentAmp;
                        bestVoice = i;
                    }
                }
            }

            // If no voice in Release state, steal the oldest voice
            if (!bestVoice) {
                uint64_t oldestTriggerId { std::numeric_limits<uint64_t>::max() };
                for (size_t i = 0; i < MaxVoices; i++) {
                    if (m_voices.at(i).triggerId < oldestTriggerId) {
                        oldestTriggerId = m_voices.at(i).triggerId;
                        bestVoice = i;
                    }
                }
            }
        }

        if (bestVoice) {
            // Reset glide if portamento is off
            if (m_portamento <= 0.001f) {
                m_voices.at(bestVoice.value()).glideFrequency = freq;
            }

            const float pan = voiceSpreadPan(bestVoice.value());

            if (m_vco1Sync) {
                m_voices.at(bestVoice.value()).trigger(note, freq, pan, finalVel, true, m_nextTriggerId++);
            } else {
                m_voices.at(bestVoice.value()).triggerRandomized(note, freq, pan, finalVel, m_phaseDist(m_rng), m_nextTriggerId++);
            }
        }
    } else if (isStacked(m_voiceMode)) {
        const uint64_t tid { m_nextTriggerId++ };
        for (size_t i = 0; i < MaxVoices; i++) {
            const double voiceFreq = freq * std::pow(2.0, voiceDetuneSemitones(i) / 12.0);

            if (m_portamento <= 0.001f) {
                m_voices.at(i).glideFrequency = voiceFreq;
            }

            const float pan = voiceSpreadPan(i);

            if (m_vco1Sync) {
                m_voices.at(i).trigger(note, voiceFreq, pan, finalVel, true, tid);
            } else {
                m_voices.at(i).triggerRandomized(note, voiceFreq, pan, finalVel, m_phaseDist(m_rng), tid);
            }
        }
    } else {
        // Dual: pair voices (0,1), (2,3), (4,5) — each pair plays one note with two detuned sub-voices
        constexpr size_t numPairs = MaxVoices / 2;
        std::optional<size_t> bestPair;

        // 1. Affinity: reuse a pair already playing this note
        for (size_t p = 0; p < numPairs; p++) {
            if (m_voices.at(p * 2).active && m_voices.at(p * 2).note == note) {
                bestPair = p;
                break;
            }
        }

        // 2. Round-robin free pair (both voices must be idle)
        if (!bestPair) {
            for (size_t i = 0; i < numPairs; i++) {
                const size_t p = (m_dualNextPair + i) % numPairs;
                if (!m_voices.at(p * 2).active && !m_voices.at(p * 2 + 1).active) {
                    bestPair = p;
                    m_dualNextPair = (p + 1) % numPairs;
                    break;
                }
            }
        }

        // 3. Steal oldest pair
        if (!bestPair) {
            uint64_t oldestId = std::numeric_limits<uint64_t>::max();
            for (size_t p = 0; p < numPairs; p++) {
                if (m_voices.at(p * 2).triggerId < oldestId) {
                    oldestId = m_voices.at(p * 2).triggerId;
                    bestPair = p;
                }
            }
        }

        if (bestPair) {
            const size_t v0 = bestPair.value() * 2;
            const size_t v1 = v0 + 1;
            const uint64_t tid = m_nextTriggerId++;

            constexpr double dualDetuneScale = 0.1;
            const double detuneAmount = std::pow(m_voiceDepth, 1.5) * dualDetuneScale;
            const double freq0 = freq * std::pow(2.0, -detuneAmount / 12.0);
            const double freq1 = freq * std::pow(2.0, detuneAmount / 12.0);

            if (m_portamento <= 0.001f) {
                m_voices.at(v0).glideFrequency = freq0;
                m_voices.at(v1).glideFrequency = freq1;
            }

            const float pan0 = 0.5f - m_panSpread * 0.5f;
            const float pan1 = 0.5f + m_panSpread * 0.5f;

            if (m_vco1Sync) {
                m_voices.at(v0).trigger(note, freq0, pan0, finalVel, true, tid);
                m_voices.at(v1).trigger(note, freq1, pan1, finalVel, true, tid);
            } else {
                m_voices.at(v0).triggerRandomized(note, freq0, pan0, finalVel, m_phaseDist(m_rng), tid);
                m_voices.at(v1).triggerRandomized(note, freq1, pan1, finalVel, m_phaseDist(m_rng), tid);
            }
        }
    }
}

void SynthDevice::handleMonoNoteOn(uint8_t note, double frequency, float velocity)
{
    auto & voice = m_voices.front();

    // Legato: the previous note has not been let go, so this is a move within one gesture. Only the
    // pitch target changes — the envelopes keep running and the oscillators keep their phase, which
    // is what turns the portamento into a glide rather than two notes with a slide between them.
    if (voice.active && voice.ampEg.state() != AdsrEnvelope::State::Release) {
        voice.note = note;
        voice.frequency = frequency;
        voice.velocity = velocity;
        if (m_portamento <= 0.001f) {
            voice.glideFrequency = frequency;
        }
        return;
    }

    // A note that starts from silence takes the next pan slot, so a mono line still travels across
    // the field the way successive poly voices would. Glided notes stay where the gesture began.
    m_monoPanSlot = m_monoPanSlot ? (m_monoPanSlot.value() + 1) % MaxVoices : 0;
    const float pan = voiceSpreadPan(m_monoPanSlot.value());

    if (m_portamento <= 0.001f) {
        voice.glideFrequency = frequency;
    }

    if (m_vco1Sync) {
        voice.trigger(note, frequency, pan, velocity, true, m_nextTriggerId++);
    } else {
        voice.triggerRandomized(note, frequency, pan, velocity, m_phaseDist(m_rng), m_nextTriggerId++);
    }
}

void SynthDevice::handleNoteOff(uint8_t note)
{
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.release();
        }
    }
}

double SynthDevice::midiNoteToFreq(uint8_t note) const
{
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

SynthDevice::ModulationValues SynthDevice::calculateModulation(Voice & voice) const
{
    ModulationValues mods;
    mods.ampEnvelope = voice.ampEg.nextSample();
    const double modEnv = voice.modEg.nextSample() * m_modInt;
    const double lfoVal = voice.lfo.nextSample() * m_lfoInt;
    const double lfo2Val = voice.lfo2.nextSample() * m_lfo2Int;

    mods.cutoffMod = (m_modTarget == ModTarget::Cutoff) ? modEnv : 0.0;
    if (m_lfoTarget == LfoTarget::Cutoff) {
        mods.cutoffMod += lfoVal;
    }
    if (m_lfo2Target == LfoTarget::Cutoff) {
        mods.cutoffMod += lfo2Val;
    }

    mods.shapeMod = (m_lfoTarget == LfoTarget::Shape) ? lfoVal : 0.0;
    if (m_lfo2Target == LfoTarget::Shape) {
        mods.shapeMod += lfo2Val;
    }

    mods.vco1PitchMod = (m_modTarget == ModTarget::Pitch1) ? modEnv : 0.0;
    mods.vco2PitchMod = (m_modTarget == ModTarget::Pitch2) ? modEnv : 0.0;
    mods.vco3PitchMod = (m_modTarget == ModTarget::Pitch3) ? modEnv : 0.0;

    if (m_lfoTarget == LfoTarget::Pitch) {
        mods.vco1PitchMod += lfoVal;
        mods.vco2PitchMod += lfoVal;
        mods.vco3PitchMod += lfoVal;
    }
    if (m_lfo2Target == LfoTarget::Pitch) {
        mods.vco1PitchMod += lfo2Val;
        mods.vco2PitchMod += lfo2Val;
        mods.vco3PitchMod += lfo2Val;
    }

    if (m_lfoTarget == LfoTarget::Volume) {
        mods.volumeMod = lfoVal;
    }
    if (m_lfo2Target == LfoTarget::Volume) {
        mods.volumeMod += lfo2Val;
    }
    if (m_lfoTarget == LfoTarget::Resonance) {
        mods.resonanceMod = lfoVal;
    }
    if (m_lfo2Target == LfoTarget::Resonance) {
        mods.resonanceMod += lfo2Val;
    }
    if (m_lfoTarget == LfoTarget::Pan) {
        mods.panMod = lfoVal;
    }
    if (m_lfo2Target == LfoTarget::Pan) {
        mods.panMod += lfo2Val;
    }

    return mods;
}

float SynthDevice::generateVoiceSample(Voice & voice, const ModulationValues & mods, double oversampledRate, double pbRatio)
{
    double vco1Freq = voice.glideFrequency * m_vco1BasePitchRatio * pbRatio;
    double vco2Freq = voice.glideFrequency * m_vco2BasePitchRatio * pbRatio;
    double vco3Freq = voice.glideFrequency * m_vco3BasePitchRatio * pbRatio;

    if (mods.vco1PitchMod != 0.0) {
        vco1Freq *= std::exp2(mods.vco1PitchMod);
    }
    if (mods.vco2PitchMod != 0.0) {
        vco2Freq *= std::exp2(mods.vco2PitchMod);
    }
    if (mods.vco3PitchMod != 0.0) {
        vco3Freq *= std::exp2(mods.vco3PitchMod);
    }

    // Drift voice mode has no fixed detune, so the wander is the detune: the depth knob feeds the
    // same per-voice oscillator drift the Drift parameter uses, on top of whatever that is set to.
    // Each voice has its own rate, and the rates are mutually irrational enough that no two voices
    // ever settle into a steady beat, which is what keeps the stack from combing.
    const double driftCents = static_cast<double>(m_oscillatorDrift) * 5.0
      + (m_voiceMode == VoiceMode::Drift ? static_cast<double>(m_voiceDepth) * driftModeMaxCents : 0.0);
    if (driftCents > 0.0) {
        voice.driftPhase = std::fmod(voice.driftPhase + voice.driftRate / oversampledRate, 1.0);
        const double driftRatio = std::exp2(driftCents / 1200.0 * std::sin(voice.driftPhase * (2.0 * M_PI)));
        vco1Freq *= driftRatio;
        vco2Freq *= driftRatio;
        vco3Freq *= driftRatio;
    }

    double vco1Val = 0.0;
    double oldPhase1 = voice.vco1.phase();
    if (m_mixVco1 >= 0.001f || m_crossModDepth >= 0.001f) {
        voice.vco1.setFrequency(vco1Freq);
        voice.vco1.setShape(std::clamp(m_vco1Shape + mods.shapeMod, 0.0, 1.0));
        oldPhase1 = voice.vco1.phase();
        vco1Val = voice.vco1.nextSample();
    }

    if (m_crossModDepth >= 0.001f) {
        // Audio-rate FM: VCO1's instantaneous output modulates VCO2's frequency,
        // mirroring the Minilogue XD's one-way Cross Mod Depth control.
        vco2Freq *= std::exp2(vco1Val * m_crossModDepth * 4.0);
    }

    double vco2Val = 0.0;
    double oldPhase2 = voice.vco2.phase();
    if (m_mixVco2 >= 0.001f) {
        voice.vco2.setFrequency(vco2Freq);
        voice.vco2.setShape(std::clamp(m_vco2Shape + mods.shapeMod, 0.0, 1.0));

        if (m_vco2Sync && voice.vco1.phase() < oldPhase1) {
            // Calculate fractional phase for VCO2 to maintain sync accuracy
            const double phaseStep1 = vco1Freq / oversampledRate;
            const double phaseStep2 = vco2Freq / oversampledRate;
            if (phaseStep1 > 0.0) {
                const double fraction = voice.vco1.phase() / phaseStep1;
                voice.vco2.sync(fraction * phaseStep2);
            } else {
                voice.vco2.sync(0.0);
            }
        }
        oldPhase2 = voice.vco2.phase();
        vco2Val = voice.vco2.nextSample();
    }

    double vco3Val = 0.0;
    if (m_mixVco3 >= 0.001f) {
        voice.vco3.setFrequency(vco3Freq);
        voice.vco3.setShape(std::clamp(m_vco3Shape + mods.shapeMod, 0.0, 1.0));

        // VCO3 Hard Sync to VCO2
        if (m_vco3Sync && voice.vco2.phase() < oldPhase2) {
            const double phaseStep2 = vco2Freq / oversampledRate;
            const double phaseStep3 = vco3Freq / oversampledRate;
            if (phaseStep2 > 0.0) {
                const double fraction = voice.vco2.phase() / phaseStep2;
                voice.vco3.sync(fraction * phaseStep3);
            } else {
                voice.vco3.sync(0.0);
            }
        }
        vco3Val = voice.vco3.nextSample();
    }

    double multiVal = 0.0;
    if (m_multiLevel >= 0.001f) {
        multiVal = voice.multi.nextSample();
    }

    const double mix = (vco1Val * m_mixVco1) + (vco2Val * m_mixVco2) + (vco3Val * m_mixVco3) + (multiVal * m_multiLevel);
    const double mixHeadroom = mix * 0.4; // Slightly more headroom for 3rd VCO + Multi engine

    // Filter
    const double cutoffMod = mods.cutoffMod + (voice.note - 60.0) / 127.0 * m_filterKeyTrack;

    voice.lpf.setCutoff(std::clamp(m_lpfCutoff + cutoffMod, 0.0, 1.0));
    voice.lpf.setResonance(std::clamp(m_lpfResonance + static_cast<float>(mods.resonanceMod), 0.0f, 1.0f));
    voice.hpf.setCutoff(m_hpfCutoff);

    const float filtered = voice.hpf.process(voice.lpf.process(static_cast<float>(mixHeadroom)));
    const float ampMod = static_cast<float>(std::max(0.0, 1.0 + mods.volumeMod));
    return filtered * static_cast<float>(mods.ampEnvelope) * ampMod;
}

void SynthDevice::syncParameters()
{
    Device::syncParameters();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco1Waveform().toStdString()); p)
        m_vco1Waveform = static_cast<PolyBlepOscillator::Waveform>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco1Octave().toStdString()); p)
        m_vco1Octave = p->get().xmlValue();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco1Pitch().toStdString()); p)
        m_vco1Pitch = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco1Shape().toStdString()); p)
        m_vco1Shape = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco1Sync().toStdString()); p)
        m_vco1Sync = p->get().value() > 0.5f;

    const double vco1PitchOffset = ParameterMapper::mapCubicCentered(m_vco1Pitch * 2.0 - 1.0, -2400, 2400);
    m_vco1BasePitchRatio = std::pow(2.0, (m_vco1Octave * 12.0 + vco1PitchOffset / 100.0) / 12.0);

    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco2Waveform().toStdString()); p)
        m_vco2Waveform = static_cast<PolyBlepOscillator::Waveform>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco2Octave().toStdString()); p)
        m_vco2Octave = p->get().xmlValue();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco2Pitch().toStdString()); p)
        m_vco2Pitch = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco2Shape().toStdString()); p)
        m_vco2Shape = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco2Sync().toStdString()); p)
        m_vco2Sync = p->get().value() > 0.5f;

    const double vco2PitchOffset = ParameterMapper::mapCubicCentered(m_vco2Pitch * 2.0 - 1.0, -2400, 2400);
    m_vco2BasePitchRatio = std::pow(2.0, (m_vco2Octave * 12.0 + vco2PitchOffset / 100.0) / 12.0);

    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco3Waveform().toStdString()); p)
        m_vco3Waveform = static_cast<PolyBlepOscillator::Waveform>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco3Octave().toStdString()); p)
        m_vco3Octave = p->get().xmlValue();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco3Pitch().toStdString()); p)
        m_vco3Pitch = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco3Shape().toStdString()); p)
        m_vco3Shape = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVco3Sync().toStdString()); p)
        m_vco3Sync = p->get().value() > 0.5f;

    const double vco3PitchOffset = ParameterMapper::mapCubicCentered(m_vco3Pitch * 2.0 - 1.0, -2400, 2400);
    m_vco3BasePitchRatio = std::pow(2.0, (m_vco3Octave * 12.0 + vco3PitchOffset / 100.0) / 12.0);

    if (const auto p = parameter(Constants::NahdXml::xmlKeyMultiMode().toStdString()); p)
        m_multiType = static_cast<MultiEngine::Type>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMultiShape().toStdString()); p)
        m_multiShape = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMultiLevel().toStdString()); p)
        m_multiLevel = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMultiKeyTrack().toStdString()); p)
        m_multiKeyTrack = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyMixLevel1().toStdString()); p)
        m_mixVco1 = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMixLevel2().toStdString()); p)
        m_mixVco2 = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMixLevel3().toStdString()); p)
        m_mixVco3 = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p)
        m_lpfCutoff = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfResonance().toStdString()); p)
        m_lpfResonance = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
        m_hpfCutoff = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p)
        m_filterKeyTrack = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyAmpAttack().toStdString()); p)
        m_ampAttack = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAmpDecay().toStdString()); p)
        m_ampDecay = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAmpSustain().toStdString()); p)
        m_ampSustain = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAmpRelease().toStdString()); p)
        m_ampRelease = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAmpVelocitySensitivity().toStdString()); p)
        m_ampVelocitySensitivity = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyModAttack().toStdString()); p)
        m_modAttack = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyModDecay().toStdString()); p)
        m_modDecay = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyModSustain().toStdString()); p)
        m_modSustain = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyModIntensity().toStdString()); p)
        m_modInt = ParameterMapper::mapCubicCentered((p->get().value() - 0.5f) * 2.0f, -1.0, 1.0);
    if (const auto p = parameter(Constants::NahdXml::xmlKeyModTarget().toStdString()); p)
        m_modTarget = static_cast<ModTarget>(p->get().xmlValue());

    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoWaveform().toStdString()); p)
        m_lfoWaveform = static_cast<Lfo::Waveform>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString()); p)
        m_lfoMode = static_cast<Lfo::Mode>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString()); p)
        m_lfoRate = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString()); p)
        m_lfoInt = ParameterMapper::mapCubicCentered((p->get().value() - 0.5f) * 2.0f, -1.0, 1.0);
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoTarget().toStdString()); p)
        m_lfoTarget = static_cast<LfoTarget>(p->get().xmlValue());

    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfo2Waveform().toStdString()); p)
        m_lfo2Waveform = static_cast<Lfo::Waveform>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfo2Mode().toStdString()); p)
        m_lfo2Mode = static_cast<Lfo::Mode>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfo2Rate().toStdString()); p)
        m_lfo2Rate = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfo2Intensity().toStdString()); p)
        m_lfo2Int = ParameterMapper::mapCubicCentered((p->get().value() - 0.5f) * 2.0f, -1.0, 1.0);
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfo2Target().toStdString()); p)
        m_lfo2Target = static_cast<LfoTarget>(p->get().xmlValue());

    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString()); p)
        m_voiceMode = static_cast<VoiceMode>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceDepth().toStdString()); p)
        m_voiceDepth = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); p)
        m_portamento = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p)
        m_panSpread = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPitchBendRange().toStdString()); p)
        m_pitchBendRange = static_cast<int>(p->get().xmlValue());

    if (const auto p = parameter(Constants::NahdXml::xmlKeyOscillatorDrift().toStdString()); p)
        m_oscillatorDrift = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyCrossModDepth().toStdString()); p)
        m_crossModDepth = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p)
        m_delayType = static_cast<Delay::Type>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p)
        m_delayTime = p->get().value() * 10.0f;
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); p)
        m_delayFeedback = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); p)
        m_delayDepth = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); p)
        m_delayMix = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); p)
        m_delaySync = p->get().value() > 0.5f;
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p)
        m_delaySyncDivision = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p)
        m_delay.setFeedbackLpf(p->get().value());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p)
        m_delay.setFeedbackHpf(p->get().value());

    m_delay.setType(m_delayType);
    m_delay.setTime(m_delayTime);
    m_delay.setFeedback(m_delayFeedback);
    m_delay.setDepth(m_delayDepth);
    m_delay.setMix(m_delayMix);
    m_delay.setSync(m_delaySync);
    m_delay.setSyncDivision(m_delaySyncDivision);

    for (auto && voice : m_voices) {
        voice.vco1.setWaveform(m_vco1Waveform);
        voice.vco2.setWaveform(m_vco2Waveform);
        voice.vco3.setWaveform(m_vco3Waveform);
        voice.vco1.setShape(m_vco1Shape);
        voice.vco2.setShape(m_vco2Shape);
        voice.vco3.setShape(m_vco3Shape);

        voice.lfo.setWaveform(m_lfoWaveform);
        voice.lfo.setMode(m_lfoMode);

        double freq = 0.0;
        if (m_lfoMode == Lfo::Mode::BPM) {
            freq = (m_delay.bpm() / 60.0f) * (0.25f / std::max(0.0001f, m_lfoRate));
        } else {
            freq = static_cast<float>(ParameterMapper::mapLfoFrequency(m_lfoRate, 0.05, 20.0));
        }
        voice.lfo.setFrequency(freq);

        voice.lfo2.setWaveform(m_lfo2Waveform);
        voice.lfo2.setMode(m_lfo2Mode);

        double freq2 = 0.0;
        if (m_lfo2Mode == Lfo::Mode::BPM) {
            freq2 = (m_delay.bpm() / 60.0f) * (0.25f / std::max(0.0001f, m_lfo2Rate));
        } else {
            freq2 = static_cast<float>(ParameterMapper::mapLfoFrequency(m_lfo2Rate, 0.05, 20.0));
        }
        voice.lfo2.setFrequency(freq2);

        voice.ampEg.setAttackTime(ParameterMapper::mapExponential(m_ampAttack, 0.000001, 20.0));
        voice.ampEg.setDecayTime(ParameterMapper::mapExponential(m_ampDecay, 0.01, 60.0));
        voice.ampEg.setSustainLevel(m_ampSustain);
        voice.ampEg.setReleaseTime(ParameterMapper::mapExponential(m_ampRelease, 0.001, 60.0));
        voice.modEg.setAttackTime(ParameterMapper::mapExponential(m_modAttack, 0.000001, 20.0));
        voice.modEg.setDecayTime(ParameterMapper::mapExponential(m_modDecay, 0.01, 60.0));
        // Zero leaves it AD: the sweep returns to where it started. Anything above holds the
        // modulation there for as long as the note is held, which is the only way to sweep to a
        // new place and stay.
        voice.modEg.setSustainLevel(m_modSustain);
        voice.modEg.setReleaseTime(ParameterMapper::mapExponential(m_modDecay, 0.01, 60.0));
    }
}

void SynthDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void SynthDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                break;
            }

            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == Constants::NahdXml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();

        // Update manual fallback values for MIDI CC reset
        setManualPan(panInternal());
        setManualVolume(volumeInternal());
        setManualGain(gainInternal());
        m_manualLpfCutoff = m_lpfCutoff;
        m_manualLpfResonance = m_lpfResonance;
        m_manualHpfCutoff = m_hpfCutoff;
    }
    emit dataChanged();
}

void SynthDevice::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_pitchBend = value;
}

void SynthDevice::processMidiProgramChange(uint8_t program, uint8_t)
{
    loadPreset(m_currentBank, program);
}

void SynthDevice::loadPreset(int bank, int index)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (bank == 0) {
            const auto & presets = SynthPresets::presets();
            if (index < 0 || index >= static_cast<int>(presets.size())) {
                return;
            }
            reset();
            for (auto && [name, val] : presets[index].parameters) {
                if (const auto p = parameter(name); p) {
                    p->get().setValue(val);
                }
            }
        } else if (bank == 1) {
            if (m_userPresets.find(index) == m_userPresets.end()) {
                return;
            }
            const auto & preset = m_userPresets.at(index);
            reset();
            for (auto && [name, val] : preset.parameters) {
                if (const auto p = parameter(name); p) {
                    p->get().setValue(val);
                }
            }
        } else {
            return;
        }

        syncParameters();

        // Update manual fallback values for MIDI CC reset to match the new preset
        setManualPan(panInternal());
        setManualVolume(volumeInternal());
        setManualGain(gainInternal());
        m_manualLpfCutoff = m_lpfCutoff;
        m_manualLpfResonance = m_lpfResonance;
        m_manualHpfCutoff = m_hpfCutoff;
    }

    emit dataChanged();
}

void SynthDevice::setUserPresets(const UserPresets & presets)
{
    {
        const std::lock_guard<std::recursive_mutex> lock(mutex());
        m_userPresets = presets;
    }
}

// Accessors (VCO1)
PolyBlepOscillator::Waveform SynthDevice::vco1Waveform() const
{
    return m_vco1Waveform;
}

void SynthDevice::setVco1Waveform(PolyBlepOscillator::Waveform wave)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVco1Waveform().toStdString(), static_cast<int>(wave));
}

int SynthDevice::vco1Octave() const
{
    return m_vco1Octave;
}

void SynthDevice::setVco1Octave(int octave)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVco1Octave().toStdString(), octave);
}

float SynthDevice::vco1Pitch() const
{
    return m_vco1Pitch;
}

void SynthDevice::setVco1Pitch(float pitch)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVco1Pitch().toStdString(), pitch);
}

float SynthDevice::vco1Shape() const
{
    return m_vco1Shape;
}

void SynthDevice::setVco1Shape(float shape)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVco1Shape().toStdString(), shape);
}

bool SynthDevice::vco1Sync() const
{
    return m_vco1Sync;
}

void SynthDevice::setVco1Sync(bool sync)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVco1Sync().toStdString(), sync ? 1.0f : 0.0f);
}

// Accessors (VCO2)
PolyBlepOscillator::Waveform SynthDevice::vco2Waveform() const
{
    return m_vco2Waveform;
}

void SynthDevice::setVco2Waveform(PolyBlepOscillator::Waveform wave)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVco2Waveform().toStdString(), static_cast<int>(wave));
}

int SynthDevice::vco2Octave() const
{
    return m_vco2Octave;
}

void SynthDevice::setVco2Octave(int octave)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVco2Octave().toStdString(), octave);
}

float SynthDevice::vco2Pitch() const
{
    return m_vco2Pitch;
}

void SynthDevice::setVco2Pitch(float pitch)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVco2Pitch().toStdString(), pitch);
}

float SynthDevice::vco2Shape() const
{
    return m_vco2Shape;
}

void SynthDevice::setVco2Shape(float shape)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVco2Shape().toStdString(), shape);
}

bool SynthDevice::vco2Sync() const
{
    return m_vco2Sync;
}

void SynthDevice::setVco2Sync(bool sync)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVco2Sync().toStdString(), sync ? 1.0f : 0.0f);
}

// Multi Engine
MultiEngine::Type SynthDevice::multiType() const
{
    return m_multiType;
}

void SynthDevice::setMultiType(MultiEngine::Type type)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyMultiMode().toStdString(), static_cast<int>(type));
}

float SynthDevice::multiShape() const
{
    return m_multiShape;
}

void SynthDevice::setMultiShape(float shape)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyMultiShape().toStdString(), shape);
}

float SynthDevice::multiLevel() const
{
    return m_multiLevel;
}

void SynthDevice::setMultiLevel(float level)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyMultiLevel().toStdString(), level);
}

float SynthDevice::multiKeyTrack() const
{
    return m_multiKeyTrack;
}

void SynthDevice::setMultiKeyTrack(float keyTrack)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyMultiKeyTrack().toStdString(), keyTrack);
}

// Mixer
float SynthDevice::mixVco1() const
{
    return m_mixVco1;
}

void SynthDevice::setMixVco1(float level)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyMixLevel1().toStdString(), level);
}

float SynthDevice::mixVco2() const
{
    return m_mixVco2;
}

void SynthDevice::setMixVco2(float level)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyMixLevel2().toStdString(), level);
}

// Filter
float SynthDevice::lpfCutoff() const
{
    return m_lpfCutoff;
}

void SynthDevice::setLpfCutoff(float cutoff)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
            p->get().setValue(cutoff);
            m_manualLpfCutoff = p->get().value();
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::lpfResonance() const
{
    return m_lpfResonance;
}

void SynthDevice::setLpfResonance(float resonance)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfResonance().toStdString()); p) {
            p->get().setValue(resonance);
            m_manualLpfResonance = p->get().value();
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::hpfCutoff() const
{
    return m_hpfCutoff;
}

void SynthDevice::setHpfCutoff(float cutoff)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
            p->get().setValue(cutoff);
            m_manualHpfCutoff = p->get().value();
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::filterKeyTrack() const
{
    return m_filterKeyTrack;
}

void SynthDevice::setFilterKeyTrack(float track)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyKeyTrack().toStdString(), track);
}

// Amp EG
float SynthDevice::ampAttack() const
{
    return m_ampAttack;
}

void SynthDevice::setAmpAttack(float a)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyAmpAttack().toStdString(), a);
}

float SynthDevice::ampDecay() const
{
    return m_ampDecay;
}

void SynthDevice::setAmpDecay(float d)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyAmpDecay().toStdString(), d);
}

float SynthDevice::ampSustain() const
{
    return m_ampSustain;
}

void SynthDevice::setAmpSustain(float s)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyAmpSustain().toStdString(), s);
}

float SynthDevice::ampRelease() const
{
    return m_ampRelease;
}

void SynthDevice::setAmpRelease(float r)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyAmpRelease().toStdString(), r);
}

float SynthDevice::ampVelocitySensitivity() const
{
    return m_ampVelocitySensitivity;
}

void SynthDevice::setAmpVelocitySensitivity(float sensitivity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyAmpVelocitySensitivity().toStdString(), sensitivity);
}

// Mod EG
float SynthDevice::modAttack() const
{
    return m_modAttack;
}

void SynthDevice::setModAttack(float a)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyModAttack().toStdString(), a);
}

float SynthDevice::modDecay() const
{
    return m_modDecay;
}

void SynthDevice::setModDecay(float d)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyModDecay().toStdString(), d);
}

float SynthDevice::modSustain() const
{
    return m_modSustain;
}

void SynthDevice::setModSustain(float sustain)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyModSustain().toStdString(), sustain);
}

float SynthDevice::modInt() const
{
    return m_modInt;
}

void SynthDevice::setModInt(float intensity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyModIntensity().toStdString(), intensity);
}

SynthDevice::ModTarget SynthDevice::modTarget() const
{
    return m_modTarget;
}

void SynthDevice::setModTarget(ModTarget target)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyModTarget().toStdString(), static_cast<int>(target));
}

// Lfo
Lfo::Waveform SynthDevice::lfoWaveform() const
{
    return m_lfoWaveform;
}

void SynthDevice::setLfoWaveform(Lfo::Waveform wave)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyLfoWaveform().toStdString(), static_cast<int>(wave));
}

Lfo::Mode SynthDevice::lfoMode() const
{
    return m_lfoMode;
}

void SynthDevice::setLfoMode(Lfo::Mode mode)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyLfoMode().toStdString(), static_cast<int>(mode));
}

float SynthDevice::lfoRate() const
{
    return m_lfoRate;
}

void SynthDevice::setLfoRate(float rate)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLfoRate().toStdString(), rate);
}

float SynthDevice::lfoInt() const
{
    return m_lfoInt;
}

void SynthDevice::setLfoInt(float intensity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLfoIntensity().toStdString(), intensity);
}

SynthDevice::LfoTarget SynthDevice::lfoTarget() const
{
    return m_lfoTarget;
}

void SynthDevice::setLfoTarget(LfoTarget target)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyLfoTarget().toStdString(), static_cast<int>(target));
}

// Lfo 2
Lfo::Waveform SynthDevice::lfo2Waveform() const
{
    return m_lfo2Waveform;
}

void SynthDevice::setLfo2Waveform(Lfo::Waveform wave)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyLfo2Waveform().toStdString(), static_cast<int>(wave));
}

Lfo::Mode SynthDevice::lfo2Mode() const
{
    return m_lfo2Mode;
}

void SynthDevice::setLfo2Mode(Lfo::Mode mode)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyLfo2Mode().toStdString(), static_cast<int>(mode));
}

float SynthDevice::lfo2Rate() const
{
    return m_lfo2Rate;
}

void SynthDevice::setLfo2Rate(float rate)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLfo2Rate().toStdString(), rate);
}

float SynthDevice::lfo2Int() const
{
    return m_lfo2Int;
}

void SynthDevice::setLfo2Int(float intensity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLfo2Intensity().toStdString(), intensity);
}

SynthDevice::LfoTarget SynthDevice::lfo2Target() const
{
    return m_lfo2Target;
}

void SynthDevice::setLfo2Target(LfoTarget target)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyLfo2Target().toStdString(), static_cast<int>(target));
}

// Voice / Global
SynthDevice::VoiceMode SynthDevice::voiceMode() const
{
    return m_voiceMode;
}

void SynthDevice::setVoiceMode(VoiceMode mode)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVoiceMode().toStdString(), static_cast<int>(mode));
}

float SynthDevice::voiceDepth() const
{
    return m_voiceDepth;
}

void SynthDevice::setVoiceDepth(float depth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceDepth().toStdString(), depth);
}

float SynthDevice::portamento() const
{
    return m_portamento;
}

void SynthDevice::setPortamento(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPortamento().toStdString(), val);
}

float SynthDevice::panSpread() const
{
    return m_panSpread;
}

void SynthDevice::setPanSpread(float spread)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPanSpread().toStdString(), spread);
}

int SynthDevice::pitchBendRange() const
{
    return m_pitchBendRange;
}

void SynthDevice::setPitchBendRange(int range)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPitchBendRange().toStdString(), static_cast<float>(range));
}

float SynthDevice::currentPitchBendOffset() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return (static_cast<float>(m_pitchBend) - 8192.0f) / 8192.0f * m_pitchBendRange;
}

void SynthDevice::setPan(float val)
{
    Device::setPan(val);
}

void SynthDevice::setVolume(float vol)
{
    Device::setVolume(vol);
}

float SynthDevice::gain() const
{
    return Device::gain();
}

void SynthDevice::setGain(float val)
{
    Device::setGain(val);
}

// Delay Accessors
Delay::Type SynthDevice::delayType() const
{
    return m_delayType;
}

void SynthDevice::setDelayType(Delay::Type type)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyDelayType().toStdString(), static_cast<int>(type));
}

float SynthDevice::delayTime() const
{
    return m_delayTime;
}

void SynthDevice::setDelayTime(float time)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDelayTime().toStdString(), time);
}

float SynthDevice::delayFeedback() const
{
    return m_delayFeedback;
}

void SynthDevice::setDelayFeedback(float fb)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDelayFeedback().toStdString(), fb);
}

float SynthDevice::delayDepth() const
{
    return m_delayDepth;
}

void SynthDevice::setDelayDepth(float depth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDelayDepth().toStdString(), depth);
}

float SynthDevice::delayMix() const
{
    return m_delayMix;
}

void SynthDevice::setDelayMix(float mix)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDelayMix().toStdString(), mix);
}

bool SynthDevice::delaySync() const
{
    return m_delaySync;
}

void SynthDevice::setDelaySync(bool sync)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDelaySync().toStdString(), sync ? 1.0f : 0.0f);
}

float SynthDevice::delaySyncDivision() const
{
    return m_delaySyncDivision;
}

void SynthDevice::setDelaySyncDivision(float division)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString(), division);
}

float SynthDevice::delayFeedbackLpf() const
{
    return m_delay.feedbackLpf();
}

void SynthDevice::setFeedbackLpf(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString(), cutoff);
}

float SynthDevice::delayFeedbackHpf() const
{
    return m_delay.feedbackHpf();
}

void SynthDevice::setFeedbackHpf(float cutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString(), cutoff);
}

// Accessors (VCO3)
PolyBlepOscillator::Waveform SynthDevice::vco3Waveform() const
{
    return m_vco3Waveform;
}

void SynthDevice::setVco3Waveform(PolyBlepOscillator::Waveform wave)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVco3Waveform().toStdString(), static_cast<int>(wave));
}

int SynthDevice::vco3Octave() const
{
    return m_vco3Octave;
}

void SynthDevice::setVco3Octave(int octave)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVco3Octave().toStdString(), octave);
}

float SynthDevice::vco3Pitch() const
{
    return m_vco3Pitch;
}

void SynthDevice::setVco3Pitch(float pitch)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVco3Pitch().toStdString(), pitch);
}

float SynthDevice::vco3Shape() const
{
    return m_vco3Shape;
}

void SynthDevice::setVco3Shape(float shape)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVco3Shape().toStdString(), shape);
}

bool SynthDevice::vco3Sync() const
{
    return m_vco3Sync;
}

void SynthDevice::setVco3Sync(bool sync)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVco3Sync().toStdString(), sync ? 1.0f : 0.0f);
}

float SynthDevice::mixVco3() const
{
    return m_mixVco3;
}

void SynthDevice::setMixVco3(float level)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyMixLevel3().toStdString(), level);
}

float SynthDevice::oscillatorDrift() const
{
    return m_oscillatorDrift;
}

void SynthDevice::setOscillatorDrift(float drift)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyOscillatorDrift().toStdString(), drift);
}

float SynthDevice::crossModDepth() const
{
    return m_crossModDepth;
}

void SynthDevice::setCrossModDepth(float depth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyCrossModDepth().toStdString(), depth);
}

} // namespace noteahead
