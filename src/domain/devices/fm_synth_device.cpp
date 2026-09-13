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

#include "fm_synth_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "fm_synth_presets.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace noteahead {

namespace {

//! Widest detune an operator's knob reaches, either way. Wider than this stops reading as detune
//! and starts reading as a wrong ratio, which the ratio setting is there for.
constexpr double MaxDetuneCents { 100.0 };

//! Note the key scaling is measured from, and how much level it takes off above it per octave at
//! the top of the knob. Above middle C an operator's sidebands climb towards Nyquist along with the
//! note, so a modulator held at a fixed level gets harsher the higher it is played; this is the
//! standard cure, and it is why a DX patch stays playable across the keyboard.
constexpr double KeyScaleBreakNote { 60.0 };
constexpr double KeyScaleOctaveAttenuation { 0.5 };

size_t supersawIndex(size_t index)
{
    return std::min(index, Utils::Dsp::supersawOffsets.size() - 1);
}

double intensityToDepth(float intensity)
{
    return ParameterMapper::mapCubicCentered(static_cast<double>(intensity) * 2.0 - 1.0, -1.0, 1.0);
}

} // namespace

const std::array<FmSynthDevice::Algorithm, FmSynthDevice::AlgorithmCount> & FmSynthDevice::algorithms()
{
    // Bit 0 is operator 1. modulators[i] names who modulates operator i, carriers names who is
    // heard. Everything in modulators[i] is above i, which is what lets a voice be rendered in one
    // pass from operator 4 down to operator 1 with no delayed values anywhere except the feedback.
    //
    // The eight are the useful four-operator topologies rather than a transcription of any one
    // machine's chart: one carrier four ways, two carriers two ways, three carriers, and additive.
    static const std::array<Algorithm, AlgorithmCount> table { {
      { { 0b0010, 0b0100, 0b1000, 0b0000 }, 0b0001 }, // Serial:          4>3>2>1
      { { 0b0010, 0b1100, 0b0000, 0b0000 }, 0b0001 }, // Stacked Mods:    (3+4)>2>1
      { { 0b0110, 0b0000, 0b1000, 0b0000 }, 0b0001 }, // Branch:          4>3, (2+3)>1
      { { 0b1110, 0b0000, 0b0000, 0b0000 }, 0b0001 }, // Triple Mod:      (2+3+4)>1
      { { 0b0010, 0b0000, 0b1000, 0b0000 }, 0b0101 }, // Twin Stacks:     4>3, 2>1
      { { 0b0010, 0b1000, 0b0000, 0b0000 }, 0b0101 }, // Chain + Carrier: 4>2>1, 3 alone
      { { 0b1000, 0b1000, 0b1000, 0b0000 }, 0b0111 }, // Shared Mod:      4>1, 4>2, 4>3
      { { 0b0000, 0b0000, 0b0000, 0b0000 }, 0b1111 }, // Additive:        four sines
    } };
    return table;
}

std::vector<std::string> FmSynthDevice::algorithmNames()
{
    return { "Serial", "Stacked Mods", "Branch", "Triple Mod", "Twin Stacks", "Chain + Carrier", "Shared Mod", "Additive" };
}

double FmSynthDevice::ratioForSetting(int setting)
{
    // Setting 0 is the half, and every setting above it is its own whole number. A modulator an
    // octave below the carrier is what most electric pianos are built on, so it earns the one
    // fractional entry on the grid.
    return setting <= 0 ? 0.5 : static_cast<double>(setting);
}

void FmSynthDevice::Voice::reset()
{
    active = false;
    for (auto && op : operators) {
        op.reset();
    }
    lpf.reset();
    hpf.reset();
    ampEg.reset();
    modEg.reset();
    lfo.reset();
    lfo2.reset();
    dcBlocker.reset();
    frequency = 0.0;
    glideFrequency = 0.0;
    pan = 0.5f;
    driftPhase = 0.0;
    damping.reset();
}

void FmSynthDevice::Voice::trigger(uint8_t n, double freq, float p, float vel, uint64_t tid, double startPhase)
{
    note = n;
    triggerId = tid;
    velocity = vel;
    frequency = freq;
    if (glideFrequency == 0.0) {
        glideFrequency = freq;
    }
    pan = p;

    // Only a silent voice may have its phases and envelopes snapped back to the start. Doing it to
    // one that is still sounding is a step in the waveform, which is a click. FM is worse off for
    // it than most: the operators are phase locked to each other, so a jump in one of them moves
    // the whole timbre and not just the level.
    if (!active) {
        for (auto && op : operators) {
            op.sync(startPhase);
            op.reset();
            op.sync(startPhase);
        }
        ampEg.reset();
        modEg.reset();
        dcBlocker.reset();
    }

    active = true;
    ampEg.trigger();
    modEg.trigger();
    for (auto && op : operators) {
        op.trigger();
    }
    lfo.reset();
    lfo2.reset();
}

void FmSynthDevice::Voice::release()
{
    ampEg.release();
    modEg.release();
}

FmSynthDevice::FmSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    m_voices.resize(MaxVoices);

    for (size_t i = 0; i < m_voices.size(); i++) {
        m_voices.at(i).driftRate = 0.11 + 0.037 * static_cast<double>(i) * std::numbers::phi;
    }

    using namespace Constants;

    for (size_t i = 0; i < OperatorCount; i++) {
        addParameter(Parameter { NahdXml::xmlKeyOperatorWaveform(i).toStdString(), 0.0f, 0, static_cast<int>(FmOperator::WaveformCount) - 1, 0, 1, Parameter::Type::Discrete });
        addParameter(Parameter { NahdXml::xmlKeyOperatorRatio(i).toStdString(), 1.0f, 0, MaxRatioSetting, 1, 1, Parameter::Type::Discrete });
        addParameter(Parameter { NahdXml::xmlKeyOperatorDetune(i).toStdString(), 0.5f, 0, 10000, 5000, 100 });
        // Only operator 1 is heard by the init patch, and it is a plain sine until something is
        // turned up. An FM synth that opens making a noise nobody chose is no use as a starting
        // point: every one of the eight algorithms has operator 1 as a carrier, so this sounds.
        addParameter(Parameter { NahdXml::xmlKeyOperatorLevel(i).toStdString(), i == 0 ? 1.0f : 0.0f, 0, 10000, i == 0 ? 10000 : 0, 100 });
        addParameter(Parameter { NahdXml::xmlKeyOperatorVelocitySensitivity(i).toStdString(), 0.0f, 0, 10000, 0, 100 });
        addParameter(Parameter { NahdXml::xmlKeyOperatorKeyScale(i).toStdString(), 0.0f, 0, 10000, 0, 100 });
        addParameter(Parameter { NahdXml::xmlKeyOperatorAttack(i).toStdString(), 0.0f, 0, 10000, 0, 100 });
        addParameter(Parameter { NahdXml::xmlKeyOperatorDecay(i).toStdString(), 0.2f, 0, 10000, 2000, 100 });
        addParameter(Parameter { NahdXml::xmlKeyOperatorSustain(i).toStdString(), 1.0f, 0, 10000, 10000, 100 });
    }

    addParameter(Parameter { NahdXml::xmlKeyAlgorithm().toStdString(), 0.0f, 0, static_cast<int>(AlgorithmCount) - 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { NahdXml::xmlKeyFeedback().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyLpfResonance().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { NahdXml::xmlKeyAmpAttack().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { NahdXml::xmlKeyAmpDecay().toStdString(), 0.2f, 0, 10000, 2000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyAmpSustain().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyAmpRelease().toStdString(), 0.2f, 0, 10000, 2000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyAmpCurve().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { NahdXml::xmlKeyAmpVelocitySensitivity().toStdString(), 1.0f, 0, 10000, 10000, 100 });

    addParameter(Parameter { NahdXml::xmlKeyModAttack().toStdString(), 0.1f, 0, 10000, 1000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyModDecay().toStdString(), 0.2f, 0, 10000, 2000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyModSustain().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { NahdXml::xmlKeyModIntensity().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyModTarget().toStdString(), 2.0f, 0, 3, 2, 1, Parameter::Type::Discrete });
    addParameter(Parameter { NahdXml::xmlKeyModCurve().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { NahdXml::xmlKeyLfoWaveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { NahdXml::xmlKeyLfoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { NahdXml::xmlKeyLfoRate().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyLfoIntensity().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyLfoTarget().toStdString(), 0.0f, 0, 5, 0, 1, Parameter::Type::Discrete });

    addParameter(Parameter { NahdXml::xmlKeyLfo2Waveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { NahdXml::xmlKeyLfo2Mode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { NahdXml::xmlKeyLfo2Rate().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyLfo2Intensity().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyLfo2Target().toStdString(), 0.0f, 0, 5, 0, 1, Parameter::Type::Discrete });

    addParameter(Parameter { NahdXml::xmlKeyVoiceMode().toStdString(), 0.0f, 0, 5, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { NahdXml::xmlKeyVoiceDepth().toStdString(), 0.1f, 0, 10000, 1000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyPanSpread().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyPortamento().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { NahdXml::xmlKeyPitchBendRange().toStdString(), 2.0f, 0, 24, 2, 1, Parameter::Type::Discrete });

    addParameter(Parameter { NahdXml::xmlKeyDelayType().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { NahdXml::xmlKeyDelayTime().toStdString(), 0.5f, 0, 10000, 500 }); // 0..10 seconds in ms
    addParameter(Parameter { NahdXml::xmlKeyDelayFeedback().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyDelayDepth().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyDelayMix().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { NahdXml::xmlKeyDelaySync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { NahdXml::xmlKeyDelaySyncDivision().toStdString(), 0.25f, 0, 10000, 2500, 100 });
    addParameter(Parameter { NahdXml::xmlKeyDelayFeedbackLpf().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { NahdXml::xmlKeyDelayFeedbackHpf().toStdString(), 0.0f, 0, 10000, 0, 100 });

    for (auto && voice : m_voices) {
        voice.lpf.setMode(CascadedSvf::Mode::LowPass);
        voice.hpf.setMode(CascadedSvf::Mode::HighPass);
    }

    FmSynthDevice::syncParameters();
}

FmSynthDevice::~FmSynthDevice() = default;

std::string FmSynthDevice::name() const
{
    return m_name;
}

std::string FmSynthDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string FmSynthDevice::typeName() const
{
    return Constants::fmSynthDeviceName().toStdString();
}

std::string FmSynthDevice::typeIdString()
{
    return "2f7c8e13-6b4a-4d90-9e35-71a0c6d84b2f";
}

std::string FmSynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> FmSynthDevice::availableMidiCcControllers() const
{
    return {
        MidiCcController { 1, "LFO Int" },
        MidiCcController { 7, "Volume" },
        MidiCcController { 10, "Pan" },
        MidiCcController { 71, "Resonance" },
        MidiCcController { 74, "Cutoff" },
        MidiCcController { 81, "HPF Cutoff" }
    };
}

bool FmSynthDevice::isStacked(VoiceMode mode)
{
    return mode == VoiceMode::Unison || mode == VoiceMode::Supersaw || mode == VoiceMode::Drift;
}

int FmSynthDevice::voicesPerNote() const
{
    switch (m_voiceMode) {
    case VoiceMode::Supersaw:
        return SupersawVoices;
    case VoiceMode::Unison:
    case VoiceMode::Drift:
        return MaxVoices;
    case VoiceMode::Dual:
        return 2;
    case VoiceMode::Poly:
    case VoiceMode::Mono:
    default:
        return 1;
    }
}

double FmSynthDevice::voiceDetuneSemitones(size_t index) const
{
    switch (m_voiceMode) {
    case VoiceMode::Unison:
        return (static_cast<double>(index) - (MaxVoices - 1) / 2.0) * std::pow(m_voiceDepth, 1.2) * (0.15 + 0.05 * (index % 2));
    case VoiceMode::Supersaw:
        return Utils::Dsp::supersawOffsets.at(supersawIndex(index)) / std::abs(Utils::Dsp::supersawOffsets.front())
          * Utils::Dsp::voiceSpreadMaxSemitones * std::pow(m_voiceDepth, 1.5);
    case VoiceMode::Drift:
        return 0.0;
    case VoiceMode::Dual: {
        const double detuneSign = (index % 2 == 0) ? -1.0 : 1.0;
        return detuneSign * std::pow(m_voiceDepth, 1.5) * Utils::Dsp::voiceSpreadMaxSemitones;
    }
    case VoiceMode::Poly:
    case VoiceMode::Mono:
    default:
        return 0.0;
    }
}

float FmSynthDevice::voiceLevel(size_t index) const
{
    if (m_voiceMode != VoiceMode::Supersaw) {
        return 1.0f;
    }
    const auto depth = static_cast<double>(m_voiceDepth);
    return static_cast<float>(Utils::Dsp::supersawOffsets.at(supersawIndex(index)) == 0.0
                                ? Utils::Dsp::supersawCentreGain(depth)
                                : Utils::Dsp::supersawSideGain(depth));
}

float FmSynthDevice::voiceStackNormalization() const
{
    double power = 0.0;
    for (int i = 0; i < voicesPerNote(); i++) {
        const double weight = static_cast<double>(voiceLevel(static_cast<size_t>(i)));
        power += weight * weight;
    }
    return power > 0.0 ? static_cast<float>(1.0 / std::sqrt(power)) : 1.0f;
}

float FmSynthDevice::voiceSpreadPan(size_t slot) const
{
    const float side = (slot % 2 == 0) ? -1.0f : 1.0f;
    const float depth = 1.0f - static_cast<float>(slot / 2) * (2.0f / static_cast<float>(MaxVoices));
    return 0.5f + (side * depth * m_panSpread * 0.5f);
}

double FmSynthDevice::voiceDampingHz(size_t index) const
{
    if (m_voiceMode != VoiceMode::Supersaw && m_voiceMode != VoiceMode::Drift) {
        return 0.0;
    }

    const double distance = m_voiceMode == VoiceMode::Supersaw
      ? std::abs(Utils::Dsp::supersawOffsets.at(supersawIndex(index))) / std::abs(Utils::Dsp::supersawOffsets.front())
      : static_cast<double>(index) / (MaxVoices - 1);

    constexpr double brightestHz = 20000.0;
    constexpr double darkestHz = 4000.0;
    const double amount = distance * static_cast<double>(m_voiceDepth);
    return brightestHz + (darkestHz - brightestHz) * amount;
}

float FmSynthDevice::scaledOperatorLevel(const Voice & voice, size_t op) const
{
    // The knob is squared on its way in. Linear, almost all of the useful modulation range is
    // squeezed into the bottom of the travel and the knob is unusable above a quarter.
    const double knob = static_cast<double>(m_operatorLevel.at(op));
    double level = knob * knob;

    if (const double sensitivity = static_cast<double>(m_operatorVelocitySensitivity.at(op)); sensitivity > 0.0) {
        level *= 1.0 - sensitivity + sensitivity * static_cast<double>(voice.velocity);
    }

    if (const double keyScale = static_cast<double>(m_operatorKeyScale.at(op)); keyScale > 0.0) {
        const double octavesAbove = std::max(0.0, static_cast<double>(voice.note) - KeyScaleBreakNote) / 12.0;
        level *= std::pow(KeyScaleOctaveAttenuation, keyScale * octavesAbove);
    }

    return static_cast<float>(level);
}

void FmSynthDevice::processAudio(AudioContext & context)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    prepareForProcessing(context);

    const uint8_t oversampleFactor = clampOversampleFactor(context.oversampleFactor);
    const uint32_t oversampledRate = context.sampleRate * oversampleFactor;

    const double portamentoTime = ParameterMapper::mapExponential(m_portamento, 0.01, 2.0);
    const double portamentoCoeff = m_portamento > 0 ? 1.0 - std::pow(0.001, 1.0 / (portamentoTime * oversampledRate)) : 1.0;
    const double pbOffset = (static_cast<double>(m_pitchBend) - 8192.0) / 8192.0 * m_pitchBendRange;
    const double pbRatio = std::exp2(pbOffset / 12.0);

    for (size_t index = 0; index < m_voices.size(); index++) {
        auto & voice = m_voices.at(index);
        voice.lfo.setWaveform(m_lfoWaveform);
        voice.lfo.setMode(m_lfoMode);
        if (m_lfoMode == Lfo::Mode::BPM) {
            voice.lfo.setFrequency(m_bpm, m_lfoRate);
        } else {
            voice.lfo.setFrequency(ParameterMapper::mapLfoFrequency(m_lfoRate, 0.05, 20.0));
        }

        voice.lfo2.setWaveform(m_lfo2Waveform);
        voice.lfo2.setMode(m_lfo2Mode);
        if (m_lfo2Mode == Lfo::Mode::BPM) {
            voice.lfo2.setFrequency(m_bpm, m_lfo2Rate);
        } else {
            voice.lfo2.setFrequency(ParameterMapper::mapLfoFrequency(m_lfo2Rate, 0.05, 20.0));
        }

        if (voice.active) {
            renderVoice(voice, context, oversampleFactor, oversampledRate, portamentoCoeff, pbRatio, index);
        }
    }

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

void FmSynthDevice::prepareForProcessing(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    m_delay.setSampleRate(static_cast<double>(context.sampleRate));
    const size_t requiredSize = static_cast<size_t>(context.frameCount) * clampOversampleFactor(context.oversampleFactor) * 2;
    if (m_oversampledBuffer.size() < requiredSize) {
        m_oversampledBuffer.resize(requiredSize);
    }
    std::fill(m_oversampledBuffer.begin(), m_oversampledBuffer.begin() + requiredSize, 0.0f);
}

void FmSynthDevice::renderVoice(Voice & voice, AudioContext & context, uint8_t oversampleFactor, uint32_t oversampledRate, double portamentoCoeff, double pbRatio, size_t index)
{
    updateVoiceParameters(voice, oversampledRate, index);

    const float gain = (1.0f / static_cast<float>(MaxVoices)) * voiceStackNormalization() * linearGainInternal() * voiceLevel(index);

    const double dampingHz = voiceDampingHz(index);
    const bool damped = dampingHz > 0.0 && dampingHz < OnePoleFilter::maxCorner(oversampledRate);
    if (damped) {
        voice.damping.calculate(dampingHz, oversampledRate);
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        for (uint8_t subSample = 0; subSample < oversampleFactor; subSample++) {
            voice.glideFrequency += (voice.frequency - voice.glideFrequency) * portamentoCoeff;
            const ModulationValues mods = calculateModulation(voice);
            float voiceSample = generateVoiceSample(voice, mods, oversampledRate, pbRatio);
            if (damped) {
                voice.damping.process(static_cast<double>(voiceSample));
                voiceSample = static_cast<float>(voice.damping.lowPass());
            }
            const float sample = voiceSample * gain;

            const float voicePan = std::clamp(panInternal() + voice.pan - 0.5f + static_cast<float>(mods.panMod) * 0.5f, 0.0f, 1.0f);
            const double panAngle = static_cast<double>(voicePan) * std::numbers::pi * 0.5;
            const float panL = static_cast<float>(std::cos(panAngle));
            const float panR = static_cast<float>(std::sin(panAngle));

            m_oversampledBuffer[(i * oversampleFactor + subSample) * 2] += sample * panL;
            m_oversampledBuffer[(i * oversampleFactor + subSample) * 2 + 1] += sample * panR;
        }

        if (voice.ampEg.isSilent()) {
            voice.active = false;
            break;
        }
    }
}

void FmSynthDevice::updateVoiceParameters(Voice & voice, uint32_t oversampledRate, size_t index)
{
    if (isStacked(m_voiceMode) || m_voiceMode == VoiceMode::Dual) {
        voice.frequency = midiNoteToFreq(voice.note) * std::pow(2.0, voiceDetuneSemitones(index) / 12.0);
    }

    for (size_t op = 0; op < OperatorCount; op++) {
        voice.operators.at(op).setSampleRate(oversampledRate);
        // Velocity and key scaling both depend on the note this voice is playing, so an operator's
        // level cannot be settled once for the whole device the way the rest of the patch is.
        voice.operators.at(op).setLevel(scaledOperatorLevel(voice, op));
    }

    voice.lpf.setSampleRate(oversampledRate);
    voice.hpf.setSampleRate(oversampledRate);
    voice.ampEg.setSampleRate(oversampledRate);
    voice.modEg.setSampleRate(oversampledRate);
    voice.lfo.setSampleRate(oversampledRate);
    voice.lfo2.setSampleRate(oversampledRate);
    voice.dcBlocker.setSampleRate(oversampledRate);

    voice.lpf.setResonance(m_lpfResonance);
    voice.hpf.setResonance(0.0f);
}

FmSynthDevice::ModulationValues FmSynthDevice::calculateModulation(Voice & voice) const
{
    ModulationValues mods = ModulationValues {};
    mods.ampEnvelope = voice.ampEg.nextSample();
    mods.modEnvelope = voice.modEg.nextSample() * m_modDepth;
    mods.lfoValue = voice.lfo.nextSample() * m_lfoDepth;
    mods.lfo2Value = voice.lfo2.nextSample() * m_lfo2Depth;

    // The index multiplier starts at unity and is scaled, not offset: modulation of the brightness
    // has to fold back to exactly the patch's own index when the modulator sits at zero.
    double indexScale = 1.0;

    const auto applyTarget = [&](auto target, double amount) {
        using Target = decltype(target);
        if (target == Target::Cutoff) {
            mods.cutoffMod += amount;
        } else if (target == Target::Pitch) {
            mods.pitchMod += amount;
        } else if (target == Target::ModIndex) {
            indexScale *= std::max(0.0, 1.0 + amount);
        }
    };

    applyTarget(m_modTarget, mods.modEnvelope);
    if (m_modTarget == ModTarget::Feedback) {
        mods.feedbackMod += mods.modEnvelope;
    }

    applyTarget(m_lfoTarget, mods.lfoValue);
    applyTarget(m_lfo2Target, mods.lfo2Value);

    for (const auto & [target, amount] : { std::pair { m_lfoTarget, mods.lfoValue }, std::pair { m_lfo2Target, mods.lfo2Value } }) {
        if (target == LfoTarget::Volume) {
            mods.volumeMod += amount;
        } else if (target == LfoTarget::Resonance) {
            mods.resonanceMod += amount;
        } else if (target == LfoTarget::Pan) {
            mods.panMod += amount;
        }
    }

    mods.indexMod = indexScale;

    return mods;
}

float FmSynthDevice::generateVoiceSample(Voice & voice, const ModulationValues & mods, double oversampledRate, double pbRatio)
{
    double freq = voice.glideFrequency * pbRatio;

    if (m_voiceMode == VoiceMode::Drift && m_voiceDepth > 0.0f) {
        const double driftCents = static_cast<double>(m_voiceDepth) * Utils::Dsp::driftModeMaxCents;
        voice.driftPhase = std::fmod(voice.driftPhase + voice.driftRate / oversampledRate, 1.0);
        freq *= std::exp2(driftCents / 1200.0 * std::sin(voice.driftPhase * 2.0 * std::numbers::pi));
    }

    freq *= std::exp2(mods.pitchMod);

    const auto & algorithm = algorithms().at(static_cast<size_t>(m_algorithm));

    const double feedbackDepth = std::max(0.0, static_cast<double>(m_feedback) + mods.feedbackMod) * FmOperator::MaxIndexCycles;
    voice.operators.at(FeedbackOperator).setFeedback(feedbackDepth);

    // Operator 4 down to operator 1. The algorithm table only ever routes an operator into a
    // lower-numbered one, so by the time an operator is asked for its sample, every operator that
    // modulates it has already produced this sample's output -- no delay anywhere but the feedback.
    std::array<double, OperatorCount> outputs {};
    for (size_t reverse = 0; reverse < OperatorCount; reverse++) {
        const size_t op = OperatorCount - 1 - reverse;

        voice.operators.at(op).setFrequency(freq * m_operatorRatioValue.at(op) * m_operatorDetuneRatio.at(op));

        double phaseMod = 0.0;
        if (const uint8_t sources = algorithm.modulators.at(op); sources) {
            for (size_t source = op + 1; source < OperatorCount; source++) {
                if (sources & (1u << source)) {
                    phaseMod += outputs.at(source);
                }
            }
            phaseMod *= FmOperator::MaxIndexCycles * mods.indexMod;
        }

        outputs.at(op) = voice.operators.at(op).nextSample(phaseMod);
    }

    double mix = 0.0;
    int carrierCount = 0;
    for (size_t op = 0; op < OperatorCount; op++) {
        if (algorithm.carriers & (1u << op)) {
            mix += outputs.at(op);
            carrierCount++;
        }
    }
    // Carriers of an FM patch usually sit at whole-number ratios of the same note, so they are
    // correlated rather than independent and their amplitudes add. Dividing by the count rather
    // than by its root is what keeps Additive from being four times as loud as Serial.
    if (carrierCount > 1) {
        mix /= static_cast<double>(carrierCount);
    }

    const auto blocked = static_cast<float>(voice.dcBlocker.process(mix));

    voice.lpf.setCutoff(std::clamp(m_lpfCutoff + static_cast<float>(mods.cutoffMod), 0.0f, 1.0f));
    voice.lpf.setResonance(std::clamp(m_lpfResonance + static_cast<float>(mods.resonanceMod), 0.0f, 1.0f));
    voice.hpf.setCutoff(m_hpfCutoff);

    const float ampMod = static_cast<float>(std::max(0.0, 1.0 + mods.volumeMod));
    const float velocityGain = 1.0f - m_ampVelocitySensitivity + m_ampVelocitySensitivity * voice.velocity;

    return voice.hpf.process(voice.lpf.process(blocked)) * static_cast<float>(mods.ampEnvelope) * ampMod * velocityGain;
}

bool FmSynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

void FmSynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void FmSynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void FmSynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    const float val = static_cast<float>(value) / 127.0f;
    bool changed = false;

    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
            if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString()); p) {
                if (const auto restored = p->get().value(); std::abs(restored - m_lfoInt) > 0.0001f) {
                    m_lfoInt = restored;
                    m_lfoDepth = intensityToDepth(m_lfoInt);
                    changed = true;
                }
            }
        } else if (controller == 1) { // LFO intensity (temporary, not saved to param)
            m_lfoInt = val;
            m_lfoDepth = intensityToDepth(m_lfoInt);
            changed = true;
        } else if (controller == 7) {
            changed = updateVolumeParameter(faderPositionFromMidiCc(value), false);
        } else if (controller == 10) {
            changed = updatePanParameter(val, false);
        } else if (controller == 74) {
            m_lpfCutoff = val;
            if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); synthParameter) {
                synthParameter->get().setAutomationValue(val);
                syncParameters();
                changed = true;
            }
        } else if (controller == 71) {
            m_lpfResonance = val;
            if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfResonance().toStdString()); synthParameter) {
                synthParameter->get().setAutomationValue(val);
                syncParameters();
                changed = true;
            }
        } else if (controller == 81) {
            m_hpfCutoff = val;
            if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); synthParameter) {
                synthParameter->get().setAutomationValue(val);
                syncParameters();
                changed = true;
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void FmSynthDevice::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_pitchBend = value;
}

void FmSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.release();
        }
    }
}

void FmSynthDevice::setBpm(float bpm)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_bpm = bpm;
    m_delay.setBpm(bpm);
}

void FmSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void FmSynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_rng.seed(RngSeed);
    m_delay.reset();
    for (auto && voice : m_voices) {
        voice.reset();
    }
    m_polyNextVoice = 0;
    m_dualNextPair = 0;
    m_monoPanSlot.reset();
    m_nextTriggerId = 1;
}

void FmSynthDevice::releaseVoicesAbove(size_t count)
{
    for (size_t i = count; i < MaxVoices; i++) {
        if (m_voices.at(i).active) {
            m_voices.at(i).release();
        }
    }
}

void FmSynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    const double freq = midiNoteToFreq(note);
    const float finalVel = static_cast<float>(velocity) / 127.0f;

    if (m_voiceMode == VoiceMode::Mono) {
        handleMonoNoteOn(note, freq, finalVel);
        return;
    }

    if (isStacked(m_voiceMode)) {
        const auto stackSize = static_cast<size_t>(voicesPerNote());
        releaseVoicesAbove(stackSize);
        const uint64_t tid = m_nextTriggerId++;
        for (size_t i = 0; i < stackSize; i++) {
            const double voiceFreq = freq * std::pow(2.0, voiceDetuneSemitones(i) / 12.0);
            if (m_portamento <= 0.001f) {
                m_voices.at(i).glideFrequency = voiceFreq;
            }
            m_voices.at(i).trigger(note, voiceFreq, voiceSpreadPan(i), finalVel, tid, m_phaseDist(m_rng));
        }
        return;
    }

    if (m_voiceMode == VoiceMode::Dual) {
        constexpr size_t numPairs = MaxVoices / 2;
        std::optional<size_t> bestPair;

        for (size_t p = 0; p < numPairs; p++) {
            if (m_voices.at(p * 2).active && m_voices.at(p * 2).note == note) {
                bestPair = p;
                break;
            }
        }

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

        if (!bestPair) {
            uint64_t oldestId = std::numeric_limits<uint64_t>::max();
            for (size_t p = 0; p < numPairs; p++) {
                if (m_voices.at(p * 2).triggerId < oldestId) {
                    oldestId = m_voices.at(p * 2).triggerId;
                    bestPair = p;
                }
            }
        }

        const size_t v0 = bestPair.value() * 2;
        const size_t v1 = v0 + 1;
        const uint64_t tid = m_nextTriggerId++;
        const double freq0 = freq * std::pow(2.0, voiceDetuneSemitones(v0) / 12.0);
        const double freq1 = freq * std::pow(2.0, voiceDetuneSemitones(v1) / 12.0);

        if (m_portamento <= 0.001f) {
            m_voices.at(v0).glideFrequency = freq0;
            m_voices.at(v1).glideFrequency = freq1;
        }

        const float pan0 = 0.5f - m_panSpread * 0.5f;
        const float pan1 = 0.5f + m_panSpread * 0.5f;
        m_voices.at(v0).trigger(note, freq0, pan0, finalVel, tid, m_phaseDist(m_rng));
        m_voices.at(v1).trigger(note, freq1, pan1, finalVel, tid, m_phaseDist(m_rng));
        return;
    }

    size_t voiceIndex = m_polyNextVoice;
    bool found = false;

    for (size_t i = 0; i < MaxVoices; i++) {
        const size_t idx = (m_polyNextVoice + i) % MaxVoices;
        if (!m_voices.at(idx).active) {
            voiceIndex = idx;
            found = true;
            break;
        }
    }

    if (!found) {
        uint64_t oldestId = std::numeric_limits<uint64_t>::max();
        for (size_t i = 0; i < MaxVoices; i++) {
            if (m_voices.at(i).triggerId < oldestId) {
                oldestId = m_voices.at(i).triggerId;
                voiceIndex = i;
            }
        }
    }

    if (m_portamento <= 0.001f) {
        m_voices.at(voiceIndex).glideFrequency = freq;
    }

    m_voices.at(voiceIndex).trigger(note, freq, 0.5f, finalVel, m_nextTriggerId++);
    m_polyNextVoice = (voiceIndex + 1) % MaxVoices;
}

void FmSynthDevice::handleMonoNoteOn(uint8_t note, double frequency, float velocity)
{
    releaseVoicesAbove(1);

    auto & voice = m_voices.front();

    if (!voice.active || voice.ampEg.state() == AdsrEnvelope::State::Release) {
        m_monoPanSlot = m_monoPanSlot ? (m_monoPanSlot.value() + 1) % MaxVoices : 0;
    }

    if (m_portamento <= 0.001f) {
        voice.glideFrequency = frequency;
    }

    voice.trigger(note, frequency, voiceSpreadPan(m_monoPanSlot.value_or(0)), velocity, m_nextTriggerId++, m_phaseDist(m_rng));
}

void FmSynthDevice::handleNoteOff(uint8_t note)
{
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.release();
        }
    }
}

double FmSynthDevice::midiNoteToFreq(uint8_t note) const
{
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

void FmSynthDevice::loadPreset(int index)
{
    const auto & presets = FmSynthPresets::presets();
    if (index >= 0 && index < static_cast<int>(presets.size())) {
        applyPreset(presets.at(static_cast<size_t>(index)).parameters);
    }
}

void FmSynthDevice::loadRandomPatch(uint32_t seed)
{
    applyPreset(FmSynthPresets::randomPatch(seed).parameters);
}

void FmSynthDevice::applyPreset(const std::map<std::string, float> & values)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        // A preset is the whole panel, so everything it does not name goes back to its default
        // first. Without this, whatever the previous patch had turned up stays turned up.
        reset();

        for (auto && [name, value] : values) {
            if (const auto synthParameter = parameter(name); synthParameter) {
                synthParameter->get().setValue(value);
            }
        }

        syncParameters();
    }

    emit dataChanged();
}

double FmSynthDevice::voiceGlideFrequency(size_t index) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return index < m_voices.size() ? m_voices.at(index).glideFrequency : 0.0;
}

void FmSynthDevice::syncParameters()
{
    Device::syncParameters();

    auto updateParam = [this](const QString & key, float & var) {
        if (const auto synthParameter = parameter(key.toStdString()); synthParameter) {
            var = synthParameter->get().value();
        }
    };

    auto updateDiscreteParam = [this](const QString & key, auto & var) {
        if (const auto synthParameter = parameter(key.toStdString()); synthParameter) {
            var = static_cast<std::decay_t<decltype(var)>>(synthParameter->get().xmlValue());
        }
    };

    using namespace Constants;

    for (size_t i = 0; i < OperatorCount; i++) {
        updateDiscreteParam(NahdXml::xmlKeyOperatorWaveform(i), m_operatorWaveform.at(i));
        updateDiscreteParam(NahdXml::xmlKeyOperatorRatio(i), m_operatorRatio.at(i));
        updateParam(NahdXml::xmlKeyOperatorDetune(i), m_operatorDetune.at(i));
        updateParam(NahdXml::xmlKeyOperatorLevel(i), m_operatorLevel.at(i));
        updateParam(NahdXml::xmlKeyOperatorVelocitySensitivity(i), m_operatorVelocitySensitivity.at(i));
        updateParam(NahdXml::xmlKeyOperatorKeyScale(i), m_operatorKeyScale.at(i));
        updateParam(NahdXml::xmlKeyOperatorAttack(i), m_operatorAttack.at(i));
        updateParam(NahdXml::xmlKeyOperatorDecay(i), m_operatorDecay.at(i));
        updateParam(NahdXml::xmlKeyOperatorSustain(i), m_operatorSustain.at(i));

        // A project saved by a newer build can name a waveform or a ratio this one does not have.
        m_operatorWaveform.at(i) = std::clamp(m_operatorWaveform.at(i), 0, static_cast<int>(FmOperator::WaveformCount) - 1);
        m_operatorRatio.at(i) = std::clamp(m_operatorRatio.at(i), 0, MaxRatioSetting);

        m_operatorRatioValue.at(i) = ratioForSetting(m_operatorRatio.at(i));
        const double detuneCents = ParameterMapper::mapCubicCentered(m_operatorDetune.at(i) * 2.0 - 1.0, -MaxDetuneCents, MaxDetuneCents);
        m_operatorDetuneRatio.at(i) = std::exp2(detuneCents / 1200.0);
    }

    updateDiscreteParam(NahdXml::xmlKeyAlgorithm(), m_algorithm);
    m_algorithm = std::clamp(m_algorithm, 0, static_cast<int>(AlgorithmCount) - 1);
    updateParam(NahdXml::xmlKeyFeedback(), m_feedback);

    updateParam(NahdXml::xmlKeyLpfCutoff(), m_lpfCutoff);
    updateParam(NahdXml::xmlKeyLpfResonance(), m_lpfResonance);
    updateParam(NahdXml::xmlKeyHpfCutoff(), m_hpfCutoff);

    updateParam(NahdXml::xmlKeyAmpAttack(), m_ampAttack);
    updateParam(NahdXml::xmlKeyAmpDecay(), m_ampDecay);
    updateParam(NahdXml::xmlKeyAmpSustain(), m_ampSustain);
    updateParam(NahdXml::xmlKeyAmpRelease(), m_ampRelease);
    updateParam(NahdXml::xmlKeyAmpCurve(), m_ampCurve);
    updateParam(NahdXml::xmlKeyAmpVelocitySensitivity(), m_ampVelocitySensitivity);

    updateParam(NahdXml::xmlKeyModAttack(), m_modAttack);
    updateParam(NahdXml::xmlKeyModDecay(), m_modDecay);
    updateParam(NahdXml::xmlKeyModSustain(), m_modSustain);
    updateParam(NahdXml::xmlKeyModIntensity(), m_modInt);
    m_modDepth = intensityToDepth(m_modInt);
    updateDiscreteParam(NahdXml::xmlKeyModTarget(), m_modTarget);
    updateParam(NahdXml::xmlKeyModCurve(), m_modCurve);

    updateDiscreteParam(NahdXml::xmlKeyLfoWaveform(), m_lfoWaveform);
    updateDiscreteParam(NahdXml::xmlKeyLfoMode(), m_lfoMode);
    updateParam(NahdXml::xmlKeyLfoRate(), m_lfoRate);
    updateParam(NahdXml::xmlKeyLfoIntensity(), m_lfoInt);
    m_lfoDepth = intensityToDepth(m_lfoInt);
    updateDiscreteParam(NahdXml::xmlKeyLfoTarget(), m_lfoTarget);

    updateDiscreteParam(NahdXml::xmlKeyLfo2Waveform(), m_lfo2Waveform);
    updateDiscreteParam(NahdXml::xmlKeyLfo2Mode(), m_lfo2Mode);
    updateParam(NahdXml::xmlKeyLfo2Rate(), m_lfo2Rate);
    updateParam(NahdXml::xmlKeyLfo2Intensity(), m_lfo2Int);
    m_lfo2Depth = intensityToDepth(m_lfo2Int);
    updateDiscreteParam(NahdXml::xmlKeyLfo2Target(), m_lfo2Target);

    updateDiscreteParam(NahdXml::xmlKeyVoiceMode(), m_voiceMode);
    updateParam(NahdXml::xmlKeyVoiceDepth(), m_voiceDepth);
    updateParam(NahdXml::xmlKeyPanSpread(), m_panSpread);
    updateParam(NahdXml::xmlKeyPortamento(), m_portamento);
    updateDiscreteParam(NahdXml::xmlKeyPitchBendRange(), m_pitchBendRange);

    updateDiscreteParam(NahdXml::xmlKeyDelayType(), m_delayType);
    updateParam(NahdXml::xmlKeyDelayTime(), m_delayTime);
    updateParam(NahdXml::xmlKeyDelayFeedback(), m_delayFeedback);
    updateParam(NahdXml::xmlKeyDelayDepth(), m_delayDepth);
    updateParam(NahdXml::xmlKeyDelayMix(), m_delayMix);
    if (const auto p = parameter(NahdXml::xmlKeyDelaySync().toStdString()); p) {
        m_delaySync = p->get().value() > 0.5f;
    }
    updateParam(NahdXml::xmlKeyDelaySyncDivision(), m_delaySyncDivision);
    if (const auto p = parameter(NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p) {
        m_delay.setFeedbackLpf(p->get().value());
    }
    if (const auto p = parameter(NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p) {
        m_delay.setFeedbackHpf(p->get().value());
    }
    m_delay.setType(m_delayType);
    m_delay.setTime(m_delayTime);
    m_delay.setFeedback(m_delayFeedback);
    m_delay.setDepth(m_delayDepth);
    m_delay.setMix(m_delayMix);
    m_delay.setSync(m_delaySync);
    m_delay.setSyncDivision(m_delaySyncDivision);

    for (auto && voice : m_voices) {
        for (size_t i = 0; i < OperatorCount; i++) {
            auto & op = voice.operators.at(i);
            op.setWaveform(static_cast<FmOperator::Waveform>(m_operatorWaveform.at(i)));
            op.envelope().setAttackTime(ParameterMapper::mapExponential(m_operatorAttack.at(i), 0.001, 10.0));
            op.envelope().setDecayTime(ParameterMapper::mapExponential(m_operatorDecay.at(i), 0.01, 10.0));
            op.envelope().setSustainLevel(m_operatorSustain.at(i));
        }

        voice.lfo.setWaveform(m_lfoWaveform);
        voice.lfo.setMode(m_lfoMode);
        if (m_lfoMode == Lfo::Mode::BPM) {
            voice.lfo.setFrequency(m_bpm, m_lfoRate);
        } else {
            voice.lfo.setFrequency(ParameterMapper::mapLfoFrequency(m_lfoRate, 0.05, 20.0));
        }

        voice.lfo2.setWaveform(m_lfo2Waveform);
        voice.lfo2.setMode(m_lfo2Mode);
        if (m_lfo2Mode == Lfo::Mode::BPM) {
            voice.lfo2.setFrequency(m_bpm, m_lfo2Rate);
        } else {
            voice.lfo2.setFrequency(ParameterMapper::mapLfoFrequency(m_lfo2Rate, 0.05, 20.0));
        }

        voice.ampEg.setAttackTime(ParameterMapper::mapExponential(m_ampAttack, 0.001, 10.0));
        voice.ampEg.setDecayTime(ParameterMapper::mapExponential(m_ampDecay, 0.01, 10.0));
        voice.ampEg.setSustainLevel(m_ampSustain);
        voice.ampEg.setReleaseTime(ParameterMapper::mapExponential(m_ampRelease, 0.01, 10.0));
        voice.ampEg.setCurve(m_ampCurve);

        voice.modEg.setAttackTime(ParameterMapper::mapExponential(m_modAttack, 0.001, 10.0));
        voice.modEg.setDecayTime(ParameterMapper::mapExponential(m_modDecay, 0.01, 10.0));
        voice.modEg.setSustainLevel(m_modSustain);
        voice.modEg.setReleaseTime(ParameterMapper::mapExponential(m_modDecay, 0.01, 10.0));
        voice.modEg.setCurve(m_modCurve);
    }
}

void FmSynthDevice::serializeToXml(ProjectWriter & writer) const
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

void FmSynthDevice::deserializeFromXml(ProjectReader & reader)
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
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
    }
    emit dataChanged();
}

// Operator accessors

FmOperator::Waveform FmSynthDevice::operatorWaveform(size_t index) const
{
    return static_cast<FmOperator::Waveform>(m_operatorWaveform.at(index));
}

void FmSynthDevice::setOperatorWaveform(size_t index, FmOperator::Waveform value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOperatorWaveform(index).toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

int FmSynthDevice::operatorRatio(size_t index) const
{
    return m_operatorRatio.at(index);
}

void FmSynthDevice::setOperatorRatio(size_t index, int value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOperatorRatio(index).toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::operatorDetune(size_t index) const
{
    return m_operatorDetune.at(index);
}

void FmSynthDevice::setOperatorDetune(size_t index, float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOperatorDetune(index).toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::operatorLevel(size_t index) const
{
    return m_operatorLevel.at(index);
}

void FmSynthDevice::setOperatorLevel(size_t index, float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOperatorLevel(index).toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::operatorVelocitySensitivity(size_t index) const
{
    return m_operatorVelocitySensitivity.at(index);
}

void FmSynthDevice::setOperatorVelocitySensitivity(size_t index, float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOperatorVelocitySensitivity(index).toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::operatorKeyScale(size_t index) const
{
    return m_operatorKeyScale.at(index);
}

void FmSynthDevice::setOperatorKeyScale(size_t index, float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOperatorKeyScale(index).toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::operatorAttack(size_t index) const
{
    return m_operatorAttack.at(index);
}

void FmSynthDevice::setOperatorAttack(size_t index, float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOperatorAttack(index).toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::operatorDecay(size_t index) const
{
    return m_operatorDecay.at(index);
}

void FmSynthDevice::setOperatorDecay(size_t index, float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOperatorDecay(index).toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::operatorSustain(size_t index) const
{
    return m_operatorSustain.at(index);
}

void FmSynthDevice::setOperatorSustain(size_t index, float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOperatorSustain(index).toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

// Global accessors

int FmSynthDevice::algorithm() const
{
    return m_algorithm;
}

void FmSynthDevice::setAlgorithm(int value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAlgorithm().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::feedback() const
{
    return m_feedback;
}

void FmSynthDevice::setFeedback(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyFeedback().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::lpfCutoff() const
{
    return m_lpfCutoff;
}

void FmSynthDevice::setLpfCutoff(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::lpfResonance() const
{
    return m_lpfResonance;
}

void FmSynthDevice::setLpfResonance(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfResonance().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::hpfCutoff() const
{
    return m_hpfCutoff;
}

void FmSynthDevice::setHpfCutoff(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::ampAttack() const
{
    return m_ampAttack;
}

void FmSynthDevice::setAmpAttack(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpAttack().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::ampDecay() const
{
    return m_ampDecay;
}

void FmSynthDevice::setAmpDecay(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpDecay().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::ampSustain() const
{
    return m_ampSustain;
}

void FmSynthDevice::setAmpSustain(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpSustain().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::ampRelease() const
{
    return m_ampRelease;
}

void FmSynthDevice::setAmpRelease(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpRelease().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::ampCurve() const
{
    return m_ampCurve;
}

void FmSynthDevice::setAmpCurve(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpCurve().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::ampVelocitySensitivity() const
{
    return m_ampVelocitySensitivity;
}

void FmSynthDevice::setAmpVelocitySensitivity(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpVelocitySensitivity().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::modAttack() const
{
    return m_modAttack;
}

void FmSynthDevice::setModAttack(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModAttack().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::modDecay() const
{
    return m_modDecay;
}

void FmSynthDevice::setModDecay(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModDecay().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::modSustain() const
{
    return m_modSustain;
}

void FmSynthDevice::setModSustain(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModSustain().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::modInt() const
{
    return m_modInt;
}

void FmSynthDevice::setModInt(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModIntensity().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

FmSynthDevice::ModTarget FmSynthDevice::modTarget() const
{
    return m_modTarget;
}

void FmSynthDevice::setModTarget(FmSynthDevice::ModTarget value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModTarget().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::modCurve() const
{
    return m_modCurve;
}

void FmSynthDevice::setModCurve(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModCurve().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

Lfo::Waveform FmSynthDevice::lfoWaveform() const
{
    return m_lfoWaveform;
}

void FmSynthDevice::setLfoWaveform(Lfo::Waveform value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoWaveform().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

Lfo::Mode FmSynthDevice::lfoMode() const
{
    return m_lfoMode;
}

void FmSynthDevice::setLfoMode(Lfo::Mode value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::lfoRate() const
{
    return m_lfoRate;
}

void FmSynthDevice::setLfoRate(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::lfoInt() const
{
    return m_lfoInt;
}

void FmSynthDevice::setLfoInt(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

FmSynthDevice::LfoTarget FmSynthDevice::lfoTarget() const
{
    return m_lfoTarget;
}

void FmSynthDevice::setLfoTarget(FmSynthDevice::LfoTarget value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoTarget().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

Lfo::Waveform FmSynthDevice::lfo2Waveform() const
{
    return m_lfo2Waveform;
}

void FmSynthDevice::setLfo2Waveform(Lfo::Waveform value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Waveform().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

Lfo::Mode FmSynthDevice::lfo2Mode() const
{
    return m_lfo2Mode;
}

void FmSynthDevice::setLfo2Mode(Lfo::Mode value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Mode().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::lfo2Rate() const
{
    return m_lfo2Rate;
}

void FmSynthDevice::setLfo2Rate(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Rate().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::lfo2Int() const
{
    return m_lfo2Int;
}

void FmSynthDevice::setLfo2Int(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Intensity().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

FmSynthDevice::LfoTarget FmSynthDevice::lfo2Target() const
{
    return m_lfo2Target;
}

void FmSynthDevice::setLfo2Target(FmSynthDevice::LfoTarget value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Target().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

FmSynthDevice::VoiceMode FmSynthDevice::voiceMode() const
{
    return m_voiceMode;
}

void FmSynthDevice::setVoiceMode(FmSynthDevice::VoiceMode value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::voiceDepth() const
{
    return m_voiceDepth;
}

void FmSynthDevice::setVoiceDepth(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyVoiceDepth().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::panSpread() const
{
    return m_panSpread;
}

void FmSynthDevice::setPanSpread(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::portamento() const
{
    return m_portamento;
}

void FmSynthDevice::setPortamento(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

int FmSynthDevice::pitchBendRange() const
{
    return m_pitchBendRange;
}

void FmSynthDevice::setPitchBendRange(int value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyPitchBendRange().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

Delay::Type FmSynthDevice::delayType() const
{
    return m_delayType;
}

void FmSynthDevice::setDelayType(Delay::Type value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(value));
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::delayTime() const
{
    return m_delayTime;
}

void FmSynthDevice::setDelayTime(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::delayFeedback() const
{
    return m_delayFeedback;
}

void FmSynthDevice::setDelayFeedback(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::delayDepth() const
{
    return m_delayDepth;
}

void FmSynthDevice::setDelayDepth(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::delayMix() const
{
    return m_delayMix;
}

void FmSynthDevice::setDelayMix(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

bool FmSynthDevice::delaySync() const
{
    return m_delaySync;
}

void FmSynthDevice::setDelaySync(bool value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(value ? 1 : 0);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::delaySyncDivision() const
{
    return m_delaySyncDivision;
}

void FmSynthDevice::setDelaySyncDivision(float value)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); synthParameter) {
        synthParameter->get().setValue(value);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::delayFeedbackLpf() const
{
    return m_delay.feedbackLpf();
}

void FmSynthDevice::setDelayFeedbackLpf(float cutoff)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); synthParameter) {
        synthParameter->get().setValue(cutoff);
        syncParameters();
        emit dataChanged();
    }
}

float FmSynthDevice::delayFeedbackHpf() const
{
    return m_delay.feedbackHpf();
}

void FmSynthDevice::setDelayFeedbackHpf(float cutoff)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); synthParameter) {
        synthParameter->get().setValue(cutoff);
        syncParameters();
        emit dataChanged();
    }
}

} // namespace noteahead
