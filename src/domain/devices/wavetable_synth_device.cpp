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

#include "wavetable_synth_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Voice @p index mapped into the supersaw table, which is one entry shorter than the voice pool.
//! Only a voice left over from another mode can land outside it, and it is on its way out anyway.
size_t supersawIndex(size_t index)
{
    return std::min(index, Utils::Dsp::supersawOffsets.size() - 1);
}

} // namespace

void WavetableSynthDevice::Voice::reset()
{
    active = false;
    osc1.sync(0.0);
    osc2.sync(0.0);
    osc1.snapPosition();
    osc2.snapPosition();
    lpf.reset();
    hpf.reset();
    ampEg.reset();
    modEg.reset();
    lfo.reset();
    lfo2.reset();
    frequency = 0.0;
    glideFrequency = 0.0;
    pan = 0.5f;
    driftPhase = 0.0;
    damping.reset();
}

void WavetableSynthDevice::Voice::trigger(uint8_t n, double freq, float p, float vel, uint64_t tid, double startPhase)
{
    note = n;
    triggerId = tid;
    velocity = vel;
    frequency = freq;
    if (glideFrequency == 0.0) {
        glideFrequency = freq;
    }
    pan = p;

    // Only sync oscillator phase on a fresh (idle) voice to avoid a pop from a
    // hard phase jump while the voice is still producing audio (retrigger/steal).
    // The morph position starts where the modulation says for the same reason: a
    // silent voice has nothing to glide from, a sounding one does.
    if (!active) {
        osc1.sync(startPhase);
        osc2.sync(startPhase);
        osc1.snapPosition();
        osc2.snapPosition();
    }

    active = true;
    ampEg.trigger();
    modEg.trigger();
    lfo.reset();
    lfo2.reset();
}

void WavetableSynthDevice::Voice::release()
{
    ampEg.release();
    modEg.release();
}

namespace {

//! Wavetables cost tens of milliseconds to build and several megabytes to hold, so they are built
//! on first use and then shared by every instance of the device for the rest of the session.
Wavetable::WavetableCS sharedWavetable(int index)
{
    static std::mutex mutex;
    static std::map<int, Wavetable::WavetableCS> cache;

    const std::lock_guard<std::mutex> lock { mutex };
    if (const auto iter = cache.find(index); iter != cache.end()) {
        return iter->second;
    }
    return cache.emplace(index, Wavetable::createSet(static_cast<size_t>(index))).first->second;
}

//! Turns an intensity knob position into the modulation depth it stands for. The taper is the one
//! the knob reads out with, and the one the Synth applies: fine near the centre, so that a small
//! reading really is a small amount of modulation.
double intensityToDepth(float intensity)
{
    return ParameterMapper::mapCubicCentered(static_cast<double>(intensity) * 2.0 - 1.0, -1.0, 1.0);
}

} // namespace

void WavetableSynthDevice::prepareWavetable(int index)
{
    if (index >= 0 && index < static_cast<int>(Wavetable::setNames().size())) {
        sharedWavetable(index);
    }
}

WavetableSynthDevice::WavetableSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    m_voices.resize(MaxVoices);

    // Mutually irrational drift rates, so no two Drift voices ever settle into a steady beat.
    for (size_t i = 0; i < m_voices.size(); i++) {
        m_voices.at(i).driftRate = 0.11 + 0.037 * static_cast<double>(i) * std::numbers::phi;
    }

    // Initialize Parameters
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc1Pos().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthOsc1Pos" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc1Octave().toStdString(), 0.0f, -2, 2, 0, 1, Parameter::Type::Discrete, { "wavetableSynthOsc1Octave" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc1Pitch().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthOsc1Pitch" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc1Level().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous, { "wavetableSynthOsc1Level" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc2Pos().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthOsc2Pos" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc2Octave().toStdString(), 0.0f, -2, 2, 0, 1, Parameter::Type::Discrete, { "wavetableSynthOsc2Octave" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc2Pitch().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthOsc2Pitch" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc2Level().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthOsc2Level" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyNoiseLevel().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthNoiseLevel" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous, { "wavetableSynthLpfCutoff" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfResonance().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthLpfResonance" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthHpfCutoff" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpAttack().toStdString(), 0.1f, 0, 10000, 1000, 100, Parameter::Type::Continuous, { "wavetableSynthAmpAttack" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpDecay().toStdString(), 0.2f, 0, 10000, 2000, 100, Parameter::Type::Continuous, { "wavetableSynthAmpDecay" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous, { "wavetableSynthAmpSustain" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpRelease().toStdString(), 0.2f, 0, 10000, 2000, 100, Parameter::Type::Continuous, { "wavetableSynthAmpRelease" } });
    // Zero is the straight-line envelope this synth has always had, so a project saved before the
    // knob existed loads with exactly the shape it was written with.
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpCurve().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyModAttack().toStdString(), 0.1f, 0, 10000, 1000, 100, Parameter::Type::Continuous, { "wavetableSynthModAttack" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModDecay().toStdString(), 0.2f, 0, 10000, 2000, 100, Parameter::Type::Continuous, { "wavetableSynthModDecay" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModSustain().toStdString(), 0.0f, 0, 10000, 0, 100 }); // AD by default
    addParameter(Parameter { Constants::NahdXml::xmlKeyModIntensity().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthModIntensity" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModTarget().toStdString(), 0.0f, 0, 4, 0, 1, Parameter::Type::Discrete, { "wavetableSynthModTarget" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModCurve().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoWaveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete, { "wavetableSynthLfoWaveform" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete, { "wavetableSynthLfoMode" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoRate().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthLfoRate" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoIntensity().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthLfoIntensity" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoTarget().toStdString(), 0.0f, 0, 6, 0, 1, Parameter::Type::Discrete, { "wavetableSynthLfoTarget" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Waveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete, { "wavetableSynthLfo2Waveform" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Mode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete, { "wavetableSynthLfo2Mode" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Rate().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthLfo2Rate" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Intensity().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthLfo2Intensity" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Target().toStdString(), 0.0f, 0, 6, 0, 1, Parameter::Type::Discrete, { "wavetableSynthLfo2Target" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMode().toStdString(), 0.0f, 0, 5, 0, 1, Parameter::Type::Discrete, { "wavetableSynthVoiceMode" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceDepth().toStdString(), 0.1f, 0, 10000, 1000, 100, Parameter::Type::Continuous, { "wavetableSynthVoiceDepth" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthPanSpread" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPortamento().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthPortamento" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPitchBendRange().toStdString(), 2.0f, 0, 24, 2, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyWavetableIndex().toStdString(), 0.0f, 0, static_cast<int>(Wavetable::setNames().size()) - 1, 0, 1, Parameter::Type::Discrete, { "wavetableSynthWavetableIndex" } });

    for (auto && voice : m_voices) {
        voice.lpf.setMode(CascadedSvf::Mode::LowPass);
        voice.hpf.setMode(CascadedSvf::Mode::HighPass);
    }

    WavetableSynthDevice::syncParameters();
}

WavetableSynthDevice::~WavetableSynthDevice() = default;

std::string WavetableSynthDevice::name() const
{
    return m_name;
}

std::string WavetableSynthDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string WavetableSynthDevice::typeName() const
{
    return Constants::wavetableSynthDeviceName().toStdString();
}

std::string WavetableSynthDevice::typeIdString()
{
    return "9d4f6a1b-3c2e-4d5f-8a9b-0c1d2e3f4a5b";
}

std::string WavetableSynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> WavetableSynthDevice::availableMidiCcControllers() const
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

void WavetableSynthDevice::processAudio(AudioContext & context)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    prepareForProcessing(context);

    const uint8_t oversampleFactor = clampOversampleFactor(context.oversampleFactor);
    m_noiseOversampleGain = noiseGainForOversampling(oversampleFactor);
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

        context.buffer[i * 2] += static_cast<double>(m_downsamplerL.process(highL.data(), oversampleFactor));
        context.buffer[i * 2 + 1] += static_cast<double>(m_downsamplerR.process(highR.data(), oversampleFactor));
    }
}

void WavetableSynthDevice::prepareForProcessing(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const size_t requiredSize = static_cast<size_t>(context.frameCount) * clampOversampleFactor(context.oversampleFactor) * 2;
    if (m_oversampledBuffer.size() < requiredSize) {
        m_oversampledBuffer.resize(requiredSize);
    }
    std::fill(m_oversampledBuffer.begin(), m_oversampledBuffer.begin() + requiredSize, 0.0f);
}

bool WavetableSynthDevice::isStacked(VoiceMode mode)
{
    return mode == VoiceMode::Unison || mode == VoiceMode::Supersaw || mode == VoiceMode::Drift;
}

int WavetableSynthDevice::voicesPerNote() const
{
    switch (m_voiceMode) {
    case VoiceMode::Supersaw:
        // A supersaw is seven saws, and the arrangement only works with the centre one at pitch, so
        // it takes seven of the eight voices rather than stretching the table to fit the last one.
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

double WavetableSynthDevice::voiceDetuneSemitones(size_t index) const
{
    switch (m_voiceMode) {
    case VoiceMode::Unison:
        return (static_cast<double>(index) - (MaxVoices - 1) / 2.0) * std::pow(m_voiceDepth, 1.2) * (0.15 + 0.05 * (index % 2));
    case VoiceMode::Supersaw:
        // Normalized so the outermost voice lands on the stated spread; what matters is the spacing.
        return Utils::Dsp::supersawOffsets.at(supersawIndex(index)) / std::abs(Utils::Dsp::supersawOffsets.front())
          * Utils::Dsp::voiceSpreadMaxSemitones * std::pow(m_voiceDepth, 1.5);
    case VoiceMode::Drift:
        // Nothing fixed: the wander applied per sample in generateVoiceSample is the whole detune.
        return 0.0;
    case VoiceMode::Dual: {
        // The pair sits at the two ends of the same spread a unison stack opens up, which is what
        // makes dual read as unison with two voices rather than as a poly patch with half the notes.
        const double detuneSign = (index % 2 == 0) ? -1.0 : 1.0;
        return detuneSign * std::pow(m_voiceDepth, 1.5) * Utils::Dsp::voiceSpreadMaxSemitones;
    }
    case VoiceMode::Poly:
    case VoiceMode::Mono:
    default:
        return 0.0;
    }
}

float WavetableSynthDevice::voiceLevel(size_t index) const
{
    if (m_voiceMode != VoiceMode::Supersaw) {
        return 1.0f;
    }
    const auto depth = static_cast<double>(m_voiceDepth);
    return static_cast<float>(Utils::Dsp::supersawOffsets.at(supersawIndex(index)) == 0.0
                                ? Utils::Dsp::supersawCentreGain(depth)
                                : Utils::Dsp::supersawSideGain(depth));
}

float WavetableSynthDevice::voiceStackNormalization() const
{
    // Equal-power: detuned voices are mutually uncorrelated for most of their beat cycle, so their
    // power sums and the compensation is the RMS of the level weights. Weighting by the actual
    // levels rather than the raw voice count matters for Supersaw, where a closed-up stack is one
    // voice and a wide one is seven; a flat 1/sqrt(N) would leave it well down when closed.
    double power = 0.0;
    for (int i = 0; i < voicesPerNote(); i++) {
        const double weight = static_cast<double>(voiceLevel(static_cast<size_t>(i)));
        power += weight * weight;
    }
    return power > 0.0 ? static_cast<float>(1.0 / std::sqrt(power)) : 1.0f;
}

float WavetableSynthDevice::voiceSpreadPan(size_t slot) const
{
    // Voice-alternating distribution inspired by Behringer DeepMind: consecutive slots swap sides and
    // step in towards the centre, so any prefix of the sequence is still balanced left to right.
    const float side = (slot % 2 == 0) ? -1.0f : 1.0f;
    const float depth = 1.0f - static_cast<float>(slot / 2) * (2.0f / static_cast<float>(MaxVoices));
    return 0.5f + (side * depth * m_panSpread * 0.5f);
}

double WavetableSynthDevice::voiceDampingHz(size_t index) const
{
    if (m_voiceMode != VoiceMode::Supersaw && m_voiceMode != VoiceMode::Drift) {
        return 0.0;
    }

    // How far this voice sits from the centre of the stack, 0..1. Supersaw has a real spread to
    // measure; Drift has none, so the voice index stands in for it.
    const double distance = m_voiceMode == VoiceMode::Supersaw
      ? std::abs(Utils::Dsp::supersawOffsets.at(supersawIndex(index))) / std::abs(Utils::Dsp::supersawOffsets.front())
      : static_cast<double>(index) / (MaxVoices - 1);

    // Full brightness at the centre, progressively darker outwards, and only as far as the depth
    // knob opens the stack up. The roughness lives in the top octaves, so this is where taking the
    // edge off costs the least character.
    constexpr double brightestHz = 20000.0;
    constexpr double darkestHz = 4000.0;
    const double amount = distance * static_cast<double>(m_voiceDepth);
    return brightestHz + (darkestHz - brightestHz) * amount;
}

void WavetableSynthDevice::renderVoice(Voice & voice, AudioContext & context, uint8_t oversampleFactor, uint32_t oversampledRate, double portamentoCoeff, double pbRatio, size_t index)
{
    updateVoiceParameters(voice, oversampledRate, index);

    // The 1/MaxVoices base is the headroom for a full chord: every voice sounding at once reaches
    // full scale. The stack normalization then keeps one note at the same level whatever the voice
    // mode, so switching modes changes the character and not the gain.
    const float gain = (1.0f / static_cast<float>(MaxVoices)) * voiceStackNormalization() * linearGainInternal() * voice.velocity * voiceLevel(index);

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

void WavetableSynthDevice::updateVoiceParameters(Voice & voice, uint32_t oversampledRate, size_t index)
{
    // The detune has to follow the depth knob while a note sounds, not only at the note on.
    if (isStacked(m_voiceMode) || m_voiceMode == VoiceMode::Dual) {
        voice.frequency = midiNoteToFreq(voice.note) * std::pow(2.0, voiceDetuneSemitones(index) / 12.0);
    }

    voice.osc1.setSampleRate(oversampledRate);
    voice.osc2.setSampleRate(oversampledRate);
    voice.lpf.setSampleRate(oversampledRate);
    voice.hpf.setSampleRate(oversampledRate);
    voice.ampEg.setSampleRate(oversampledRate);
    voice.modEg.setSampleRate(oversampledRate);
    voice.lfo.setSampleRate(oversampledRate);
    voice.lfo2.setSampleRate(oversampledRate);

    voice.lpf.setResonance(m_lpfResonance);
    voice.hpf.setResonance(0.0f);
}

bool WavetableSynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

void WavetableSynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void WavetableSynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void WavetableSynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    const float val = static_cast<float>(value) / 127.0f;
    bool changed = false;

    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
            // The LFO intensity rides on the modulation wheel and never reaches a parameter, so it
            // has to be put back from the one it belongs to by hand.
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

void WavetableSynthDevice::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_pitchBend = value;
}

void WavetableSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.release();
        }
    }
}

void WavetableSynthDevice::setBpm(float bpm)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_bpm = bpm;
}

void WavetableSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void WavetableSynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_rng.seed(RngSeed);
    for (auto && voice : m_voices) {
        voice.reset();
    }
    m_polyNextVoice = 0;
    m_dualNextPair = 0;
    m_monoPanSlot.reset();
    m_nextTriggerId = 1;
}

void WavetableSynthDevice::releaseVoicesAbove(size_t count)
{
    for (size_t i = count; i < MaxVoices; i++) {
        if (m_voices.at(i).active) {
            m_voices.at(i).release();
        }
    }
}

void WavetableSynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    const double freq = midiNoteToFreq(note);
    const float finalVel = static_cast<float>(velocity) / 127.0f;

    if (m_voiceMode == VoiceMode::Mono) {
        // Mono keeps the sounding voice alive on purpose, so it cannot go through the stacked path.
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
        // Pair voices (0,1), (2,3), ... — each pair plays one note with two detuned sub-voices
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

        // 3. Steal the oldest pair
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

    // Polyphonic mode
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

    // Poly stays centred: the pan spread is a property of a stack, and widening poly here would
    // move every note of every project saved before the other voice modes existed.
    m_voices.at(voiceIndex).trigger(note, freq, 0.5f, finalVel, m_nextTriggerId++);
    m_polyNextVoice = (voiceIndex + 1) % MaxVoices;
}

void WavetableSynthDevice::handleMonoNoteOn(uint8_t note, double frequency, float velocity)
{
    releaseVoicesAbove(1);

    auto & voice = m_voices.front();

    // A note that starts from silence takes the next pan slot, so a mono line still travels across
    // the field the way successive poly voices would. A note arriving over a sounding one belongs to
    // the same gesture and stays where that gesture began, which also keeps the pan from jumping
    // under a tone that never falls silent between the two.
    if (!voice.active || voice.ampEg.state() == AdsrEnvelope::State::Release) {
        m_monoPanSlot = m_monoPanSlot ? (m_monoPanSlot.value() + 1) % MaxVoices : 0;
    }

    // Every note gets its own attack — the envelopes retrigger even mid-glide, which is what keeps a
    // mono line articulate. Only the pitch is continuous: the glide frequency is left where the last
    // note put it, so the oscillators travel to the new note rather than jumping.
    if (m_portamento <= 0.001f) {
        voice.glideFrequency = frequency;
    }

    voice.trigger(note, frequency, voiceSpreadPan(m_monoPanSlot.value_or(0)), velocity, m_nextTriggerId++, m_phaseDist(m_rng));
}

void WavetableSynthDevice::handleNoteOff(uint8_t note)
{
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.release();
        }
    }
}

double WavetableSynthDevice::midiNoteToFreq(uint8_t note) const
{
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

WavetableSynthDevice::ModulationValues WavetableSynthDevice::calculateModulation(Voice & voice) const
{
    ModulationValues mods = ModulationValues {};
    mods.ampEnvelope = voice.ampEg.nextSample();
    mods.modEnvelope = voice.modEg.nextSample() * m_modDepth;
    mods.lfoValue = voice.lfo.nextSample() * m_lfoDepth;
    mods.lfo2Value = voice.lfo2.nextSample() * m_lfo2Depth;

    if (m_modTarget == ModTarget::Cutoff) {
        mods.cutoffMod = mods.modEnvelope;
    } else if (m_modTarget == ModTarget::Osc1Pos) {
        mods.osc1PosMod = mods.modEnvelope;
    } else if (m_modTarget == ModTarget::Osc2Pos) {
        mods.osc2PosMod = mods.modEnvelope;
    }

    if (m_lfoTarget == LfoTarget::Cutoff) {
        mods.cutoffMod += mods.lfoValue;
    } else if (m_lfoTarget == LfoTarget::Osc1Pos) {
        mods.osc1PosMod += mods.lfoValue;
    } else if (m_lfoTarget == LfoTarget::Osc2Pos) {
        mods.osc2PosMod += mods.lfoValue;
    } else if (m_lfoTarget == LfoTarget::Volume) {
        mods.volumeMod += mods.lfoValue;
    } else if (m_lfoTarget == LfoTarget::Resonance) {
        mods.resonanceMod += mods.lfoValue;
    } else if (m_lfoTarget == LfoTarget::Pan) {
        mods.panMod += mods.lfoValue;
    }

    if (m_lfo2Target == LfoTarget::Cutoff) {
        mods.cutoffMod += mods.lfo2Value;
    } else if (m_lfo2Target == LfoTarget::Osc1Pos) {
        mods.osc1PosMod += mods.lfo2Value;
    } else if (m_lfo2Target == LfoTarget::Osc2Pos) {
        mods.osc2PosMod += mods.lfo2Value;
    } else if (m_lfo2Target == LfoTarget::Volume) {
        mods.volumeMod += mods.lfo2Value;
    } else if (m_lfo2Target == LfoTarget::Resonance) {
        mods.resonanceMod += mods.lfo2Value;
    } else if (m_lfo2Target == LfoTarget::Pan) {
        mods.panMod += mods.lfo2Value;
    }

    return mods;
}

float WavetableSynthDevice::generateVoiceSample(Voice & voice, const ModulationValues & mods, double oversampledRate, double pbRatio)
{
    double freq = voice.glideFrequency * pbRatio;

    // Drift voice mode has no fixed detune, so the wander is the detune. Each voice has its own
    // rate, and the rates are mutually irrational enough that no two voices ever settle into a
    // steady beat, which is what keeps the stack from combing.
    if (m_voiceMode == VoiceMode::Drift && m_voiceDepth > 0.0f) {
        const double driftCents = static_cast<double>(m_voiceDepth) * Utils::Dsp::driftModeMaxCents;
        voice.driftPhase = std::fmod(voice.driftPhase + voice.driftRate / oversampledRate, 1.0);
        freq *= std::exp2(driftCents / 1200.0 * std::sin(voice.driftPhase * 2.0 * std::numbers::pi));
    }

    double osc1Freq = freq * m_osc1BasePitchRatio;
    double osc2Freq = freq * m_osc2BasePitchRatio;

    double osc1PitchMod = 0.0;
    double osc2PitchMod = 0.0;

    if (m_modTarget == ModTarget::Pitch1) {
        osc1PitchMod += mods.modEnvelope;
    } else if (m_modTarget == ModTarget::Pitch2) {
        osc2PitchMod += mods.modEnvelope;
    }

    if (m_lfoTarget == LfoTarget::Pitch) {
        osc1PitchMod += mods.lfoValue;
        osc2PitchMod += mods.lfoValue;
    }

    if (m_lfo2Target == LfoTarget::Pitch) {
        osc1PitchMod += mods.lfo2Value;
        osc2PitchMod += mods.lfo2Value;
    }

    osc1Freq *= std::exp2(osc1PitchMod);
    osc2Freq *= std::exp2(osc2PitchMod);

    voice.osc1.setFrequency(osc1Freq);
    voice.osc1.setPosition(std::clamp(m_osc1Pos + mods.osc1PosMod, 0.0, 1.0));
    const float osc1Val = static_cast<float>(voice.osc1.nextSample()) * m_osc1Level;

    voice.osc2.setFrequency(osc2Freq);
    voice.osc2.setPosition(std::clamp(m_osc2Pos + mods.osc2PosMod, 0.0, 1.0));
    const float osc2Val = static_cast<float>(voice.osc2.nextSample()) * m_osc2Level;

    const float noise = m_noiseDist(m_rng) * m_noiseLevel * m_noiseOversampleGain;

    const float mix = osc1Val + osc2Val + noise;

    voice.lpf.setCutoff(std::clamp(m_lpfCutoff + static_cast<float>(mods.cutoffMod), 0.0f, 1.0f));
    voice.lpf.setResonance(std::clamp(m_lpfResonance + static_cast<float>(mods.resonanceMod), 0.0f, 1.0f));
    voice.hpf.setCutoff(m_hpfCutoff);

    const float ampMod = static_cast<float>(std::max(0.0, 1.0 + mods.volumeMod));
    return voice.hpf.process(voice.lpf.process(mix)) * static_cast<float>(mods.ampEnvelope) * ampMod;
}

void WavetableSynthDevice::syncParameters()
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

    updateParam(Constants::NahdXml::xmlKeyOsc1Pos(), m_osc1Pos);
    updateDiscreteParam(Constants::NahdXml::xmlKeyOsc1Octave(), m_osc1Octave);
    updateParam(Constants::NahdXml::xmlKeyOsc1Pitch(), m_osc1Pitch);
    updateParam(Constants::NahdXml::xmlKeyOsc1Level(), m_osc1Level);

    updateParam(Constants::NahdXml::xmlKeyOsc2Pos(), m_osc2Pos);
    updateDiscreteParam(Constants::NahdXml::xmlKeyOsc2Octave(), m_osc2Octave);
    updateParam(Constants::NahdXml::xmlKeyOsc2Pitch(), m_osc2Pitch);
    updateParam(Constants::NahdXml::xmlKeyOsc2Level(), m_osc2Level);

    updateParam(Constants::NahdXml::xmlKeyNoiseLevel(), m_noiseLevel);

    updateParam(Constants::NahdXml::xmlKeyLpfCutoff(), m_lpfCutoff);
    updateParam(Constants::NahdXml::xmlKeyLpfResonance(), m_lpfResonance);
    updateParam(Constants::NahdXml::xmlKeyHpfCutoff(), m_hpfCutoff);

    updateParam(Constants::NahdXml::xmlKeyAmpAttack(), m_ampAttack);
    updateParam(Constants::NahdXml::xmlKeyAmpDecay(), m_ampDecay);
    updateParam(Constants::NahdXml::xmlKeyAmpSustain(), m_ampSustain);
    updateParam(Constants::NahdXml::xmlKeyAmpRelease(), m_ampRelease);
    updateParam(Constants::NahdXml::xmlKeyAmpCurve(), m_ampCurve);

    updateParam(Constants::NahdXml::xmlKeyModAttack(), m_modAttack);
    updateParam(Constants::NahdXml::xmlKeyModDecay(), m_modDecay);
    updateParam(Constants::NahdXml::xmlKeyModSustain(), m_modSustain);
    updateParam(Constants::NahdXml::xmlKeyModIntensity(), m_modInt);
    m_modDepth = intensityToDepth(m_modInt);
    updateDiscreteParam(Constants::NahdXml::xmlKeyModTarget(), m_modTarget);
    updateParam(Constants::NahdXml::xmlKeyModCurve(), m_modCurve);

    updateDiscreteParam(Constants::NahdXml::xmlKeyLfoWaveform(), m_lfoWaveform);
    updateDiscreteParam(Constants::NahdXml::xmlKeyLfoMode(), m_lfoMode);
    updateParam(Constants::NahdXml::xmlKeyLfoRate(), m_lfoRate);
    updateParam(Constants::NahdXml::xmlKeyLfoIntensity(), m_lfoInt);
    m_lfoDepth = intensityToDepth(m_lfoInt);
    updateDiscreteParam(Constants::NahdXml::xmlKeyLfoTarget(), m_lfoTarget);

    updateDiscreteParam(Constants::NahdXml::xmlKeyLfo2Waveform(), m_lfo2Waveform);
    updateDiscreteParam(Constants::NahdXml::xmlKeyLfo2Mode(), m_lfo2Mode);
    updateParam(Constants::NahdXml::xmlKeyLfo2Rate(), m_lfo2Rate);
    updateParam(Constants::NahdXml::xmlKeyLfo2Intensity(), m_lfo2Int);
    m_lfo2Depth = intensityToDepth(m_lfo2Int);
    updateDiscreteParam(Constants::NahdXml::xmlKeyLfo2Target(), m_lfo2Target);

    updateDiscreteParam(Constants::NahdXml::xmlKeyVoiceMode(), m_voiceMode);
    updateParam(Constants::NahdXml::xmlKeyVoiceDepth(), m_voiceDepth);
    updateParam(Constants::NahdXml::xmlKeyPanSpread(), m_panSpread);
    updateParam(Constants::NahdXml::xmlKeyPortamento(), m_portamento);
    updateDiscreteParam(Constants::NahdXml::xmlKeyPitchBendRange(), m_pitchBendRange);
    updateDiscreteParam(Constants::NahdXml::xmlKeyWavetableIndex(), m_wavetableIndex);

    // A project saved by a newer build can name a set this one does not have; fall back to the
    // last one rather than refusing to load.
    m_wavetableIndex = std::clamp(m_wavetableIndex, 0, static_cast<int>(Wavetable::setNames().size()) - 1);
    const auto currentWavetable = sharedWavetable(m_wavetableIndex);

    const double osc1PitchOffset = ParameterMapper::mapCubicCentered(m_osc1Pitch * 2.0 - 1.0, -1200, 1200);
    m_osc1BasePitchRatio = std::pow(2.0, (m_osc1Octave * 12.0 + osc1PitchOffset / 100.0) / 12.0);

    const double osc2PitchOffset = ParameterMapper::mapCubicCentered(m_osc2Pitch * 2.0 - 1.0, -1200, 1200);
    m_osc2BasePitchRatio = std::pow(2.0, (m_osc2Octave * 12.0 + osc2PitchOffset / 100.0) / 12.0);

    for (auto && voice : m_voices) {
        voice.osc1.setWavetable(currentWavetable);
        voice.osc2.setWavetable(currentWavetable);

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
        // Zero leaves it AD: the sweep returns to where it started. Anything above holds the
        // modulation there for as long as the note is held.
        voice.modEg.setSustainLevel(m_modSustain);
        voice.modEg.setReleaseTime(ParameterMapper::mapExponential(m_modDecay, 0.01, 10.0));
        voice.modEg.setCurve(m_modCurve);
    }
}

void WavetableSynthDevice::serializeToXml(ProjectWriter & writer) const
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

void WavetableSynthDevice::deserializeFromXml(ProjectReader & reader)
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

// Accessors (Osc 1)
float WavetableSynthDevice::osc1Pos() const
{
    return m_osc1Pos;
}

void WavetableSynthDevice::setOsc1Pos(float pos)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc1Pos().toStdString()); synthParameter) {
        synthParameter->get().setValue(pos);
        syncParameters();
        emit dataChanged();
    }
}

int WavetableSynthDevice::osc1Octave() const
{
    return m_osc1Octave;
}

void WavetableSynthDevice::setOsc1Octave(int octave)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc1Octave().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(octave);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::osc1Pitch() const
{
    return m_osc1Pitch;
}

void WavetableSynthDevice::setOsc1Pitch(float pitch)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc1Pitch().toStdString()); synthParameter) {
        synthParameter->get().setValue(pitch);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::osc1Level() const
{
    return m_osc1Level;
}

void WavetableSynthDevice::setOsc1Level(float level)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc1Level().toStdString()); synthParameter) {
        synthParameter->get().setValue(level);
        syncParameters();
        emit dataChanged();
    }
}

// Accessors (Osc 2)
float WavetableSynthDevice::osc2Pos() const
{
    return m_osc2Pos;
}

void WavetableSynthDevice::setOsc2Pos(float pos)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc2Pos().toStdString()); synthParameter) {
        synthParameter->get().setValue(pos);
        syncParameters();
        emit dataChanged();
    }
}

int WavetableSynthDevice::osc2Octave() const
{
    return m_osc2Octave;
}

void WavetableSynthDevice::setOsc2Octave(int octave)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc2Octave().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(octave);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::osc2Pitch() const
{
    return m_osc2Pitch;
}

void WavetableSynthDevice::setOsc2Pitch(float pitch)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc2Pitch().toStdString()); synthParameter) {
        synthParameter->get().setValue(pitch);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::osc2Level() const
{
    return m_osc2Level;
}

void WavetableSynthDevice::setOsc2Level(float level)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc2Level().toStdString()); synthParameter) {
        synthParameter->get().setValue(level);
        syncParameters();
        emit dataChanged();
    }
}

// Noise
float WavetableSynthDevice::noiseLevel() const
{
    return m_noiseLevel;
}

void WavetableSynthDevice::setNoiseLevel(float level)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyNoiseLevel().toStdString()); synthParameter) {
        synthParameter->get().setValue(level);
        syncParameters();
        emit dataChanged();
    }
}

// Filter
float WavetableSynthDevice::lpfCutoff() const
{
    return m_lpfCutoff;
}

void WavetableSynthDevice::setLpfCutoff(float cutoff)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); synthParameter) {
        synthParameter->get().setValue(cutoff);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lpfResonance() const
{
    return m_lpfResonance;
}

void WavetableSynthDevice::setLpfResonance(float resonance)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfResonance().toStdString()); synthParameter) {
        synthParameter->get().setValue(resonance);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::hpfCutoff() const
{
    return m_hpfCutoff;
}

void WavetableSynthDevice::setHpfCutoff(float cutoff)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); synthParameter) {
        synthParameter->get().setValue(cutoff);
        syncParameters();
        emit dataChanged();
    }
}

// Amp EG
float WavetableSynthDevice::ampAttack() const
{
    return m_ampAttack;
}

void WavetableSynthDevice::setAmpAttack(float a)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpAttack().toStdString()); synthParameter) {
        synthParameter->get().setValue(a);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::ampDecay() const
{
    return m_ampDecay;
}

void WavetableSynthDevice::setAmpDecay(float d)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpDecay().toStdString()); synthParameter) {
        synthParameter->get().setValue(d);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::ampSustain() const
{
    return m_ampSustain;
}

void WavetableSynthDevice::setAmpSustain(float s)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpSustain().toStdString()); synthParameter) {
        synthParameter->get().setValue(s);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::ampRelease() const
{
    return m_ampRelease;
}

void WavetableSynthDevice::setAmpRelease(float r)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpRelease().toStdString()); synthParameter) {
        synthParameter->get().setValue(r);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::ampCurve() const
{
    return m_ampCurve;
}

void WavetableSynthDevice::setAmpCurve(float curve)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpCurve().toStdString()); synthParameter) {
        synthParameter->get().setValue(curve);
        syncParameters();
        emit dataChanged();
    }
}

// Mod EG
float WavetableSynthDevice::modAttack() const
{
    return m_modAttack;
}

void WavetableSynthDevice::setModAttack(float a)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModAttack().toStdString()); synthParameter) {
        synthParameter->get().setValue(a);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::modDecay() const
{
    return m_modDecay;
}

void WavetableSynthDevice::setModDecay(float d)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModDecay().toStdString()); synthParameter) {
        synthParameter->get().setValue(d);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::modInt() const
{
    return m_modInt;
}

float WavetableSynthDevice::modSustain() const
{
    return m_modSustain;
}

void WavetableSynthDevice::setModSustain(float sustain)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModSustain().toStdString()); synthParameter) {
        synthParameter->get().setValue(sustain);
        syncParameters();
        emit dataChanged();
    }
}

void WavetableSynthDevice::setModInt(float intensity)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModIntensity().toStdString()); synthParameter) {
        synthParameter->get().setValue(intensity);
        syncParameters();
        emit dataChanged();
    }
}

WavetableSynthDevice::ModTarget WavetableSynthDevice::modTarget() const
{
    return m_modTarget;
}

void WavetableSynthDevice::setModTarget(ModTarget target)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModTarget().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(target));
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::modCurve() const
{
    return m_modCurve;
}

void WavetableSynthDevice::setModCurve(float curve)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModCurve().toStdString()); synthParameter) {
        synthParameter->get().setValue(curve);
        syncParameters();
        emit dataChanged();
    }
}

// LFO
Lfo::Waveform WavetableSynthDevice::lfoWaveform() const
{
    return m_lfoWaveform;
}

void WavetableSynthDevice::setLfoWaveform(Lfo::Waveform wave)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoWaveform().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(wave));
        syncParameters();
        emit dataChanged();
    }
}

Lfo::Mode WavetableSynthDevice::lfoMode() const
{
    return m_lfoMode;
}

void WavetableSynthDevice::setLfoMode(Lfo::Mode mode)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(mode));
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lfoRate() const
{
    return m_lfoRate;
}

void WavetableSynthDevice::setLfoRate(float rate)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString()); synthParameter) {
        synthParameter->get().setValue(rate);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lfoInt() const
{
    return m_lfoInt;
}

void WavetableSynthDevice::setLfoInt(float intensity)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString()); synthParameter) {
        synthParameter->get().setValue(intensity);
        syncParameters();
        emit dataChanged();
    }
}

WavetableSynthDevice::LfoTarget WavetableSynthDevice::lfoTarget() const
{
    return m_lfoTarget;
}

void WavetableSynthDevice::setLfoTarget(LfoTarget target)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoTarget().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(target));
        syncParameters();
        emit dataChanged();
    }
}

// LFO 2
Lfo::Waveform WavetableSynthDevice::lfo2Waveform() const
{
    return m_lfo2Waveform;
}

void WavetableSynthDevice::setLfo2Waveform(Lfo::Waveform wave)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Waveform().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(wave));
        syncParameters();
        emit dataChanged();
    }
}

Lfo::Mode WavetableSynthDevice::lfo2Mode() const
{
    return m_lfo2Mode;
}

void WavetableSynthDevice::setLfo2Mode(Lfo::Mode mode)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Mode().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(mode));
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lfo2Rate() const
{
    return m_lfo2Rate;
}

void WavetableSynthDevice::setLfo2Rate(float rate)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Rate().toStdString()); synthParameter) {
        synthParameter->get().setValue(rate);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lfo2Int() const
{
    return m_lfo2Int;
}

void WavetableSynthDevice::setLfo2Int(float intensity)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Intensity().toStdString()); synthParameter) {
        synthParameter->get().setValue(intensity);
        syncParameters();
        emit dataChanged();
    }
}

WavetableSynthDevice::LfoTarget WavetableSynthDevice::lfo2Target() const
{
    return m_lfo2Target;
}

void WavetableSynthDevice::setLfo2Target(LfoTarget target)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Target().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(target));
        syncParameters();
        emit dataChanged();
    }
}

// Voice / Global
double WavetableSynthDevice::voiceGlideFrequency(size_t index) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return index < m_voices.size() ? m_voices.at(index).glideFrequency : 0.0;
}

WavetableSynthDevice::VoiceMode WavetableSynthDevice::voiceMode() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_voiceMode;
}

void WavetableSynthDevice::setVoiceMode(VoiceMode mode)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVoiceMode().toStdString(), static_cast<int>(mode));
}

float WavetableSynthDevice::voiceDepth() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_voiceDepth;
}

void WavetableSynthDevice::setVoiceDepth(float depth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceDepth().toStdString(), depth);
}

float WavetableSynthDevice::panSpread() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_panSpread;
}

void WavetableSynthDevice::setPanSpread(float spread)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPanSpread().toStdString(), spread);
}

float WavetableSynthDevice::portamento() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_portamento;
}

void WavetableSynthDevice::setPortamento(float p)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPortamento().toStdString(), p);
}

int WavetableSynthDevice::wavetableIndex() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_wavetableIndex;
}

void WavetableSynthDevice::setWavetableIndex(int index)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyWavetableIndex().toStdString(), index);
}

std::vector<std::string> WavetableSynthDevice::wavetableNames() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return Wavetable::setNames();
}

int WavetableSynthDevice::pitchBendRange() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_pitchBendRange;
}

void WavetableSynthDevice::setPitchBendRange(int range)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyPitchBendRange().toStdString(), range);
}

} // namespace noteahead
