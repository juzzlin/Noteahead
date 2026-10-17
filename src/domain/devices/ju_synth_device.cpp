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

#include "ju_synth_device.hpp"

#include "device_parameter_display.hpp"
#include "ju_synth_presets.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

//! LFO rate range, Hz, in equal ratios. The Juno-60's runs from a slow sweep to a fast trill.
constexpr double LfoMinHz { 0.1 };
constexpr double LfoMaxHz { 20.0 };
//! Longest LFO delay, seconds. Half of it is spent silent and the other half fading in.
constexpr double LfoMaxDelaySeconds { 3.0 };
//! DCO vibrato at the top of its control, semitones either way. The control is squared, so the
//! bottom half of its travel is where vibrato lives and the top half where it becomes a siren.
constexpr double MaxPitchLfoSemitones { 12.0 };
//! Narrowest pulse the PWM reaches: the Juno's goes from square nearly to nothing.
constexpr double MaxPulseNarrowing { 0.45 };

//! Envelope times, seconds, in equal ratios: about the Juno-60's ranges.
constexpr double AttackMinSeconds { 0.001 };
constexpr double AttackMaxSeconds { 3.0 };
constexpr double DecayMinSeconds { 0.002 };
constexpr double DecayMaxSeconds { 12.0 };
constexpr double ReleaseMinSeconds { 0.002 };
constexpr double ReleaseMaxSeconds { 12.0 };
//! The Juno's envelope is an RC circuit: segments that start fast and ease in.
constexpr double EnvelopeCurve { 0.5 };

//! The gate mode's edges: just enough slope that a note does not click on and off.
constexpr double GateAttackSeconds { 0.002 };
constexpr double GateReleaseSeconds { 0.004 };

//! The filter's corner over its knob, 20 Hz to 20 kHz in equal octaves, the curve the Synth's runs on.
constexpr double FilterLowestHz { 20.0 };
constexpr double FilterHighestHz { 20000.0 };
//! How far the filter LFO moves the cutoff at the top of its control, in knob travel either way.
constexpr double MaxFilterLfoSweep { 0.5 };

//! Level into the ladder. Its OTAs fold at unity, and the feedback has to have room under that
//! ceiling or the fold flattens the very peak the resonance is making: at half scale the voice alone
//! took a 90% resonance down to almost nothing. A fifth leaves the peak free to sing and still lets
//! a self-oscillating filter, which grows until it folds, settle at a steady level.
constexpr double FilterInputLevel { 0.1 };
//! Taken back off after the ladder, so the voice comes out at the level it always had.
constexpr double FilterMakeupGain { 0.5 / FilterInputLevel };

//! How long the output stages keep running after the last voice has gone quiet.
constexpr double TailSeconds { 0.1 };

constexpr double PortamentoMinSeconds { 0.01 };
constexpr double PortamentoMaxSeconds { 2.0 };

double octaveRatio(int range)
{
    // 16', 8', 4'
    return std::exp2(static_cast<double>(range) - 1.0);
}

} // namespace

void JuSynthDevice::Voice::reset()
{
    saw.reset();
    pulse.reset();
    sub.reset();
    vcf.reset();
    env.reset();
    gate.reset();
    active = false;
    gated = false;
    glideFrequency = 0.0;
}

JuSynthDevice::JuSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    addParameter(Parameter { C::xmlKeyLfoRate().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { C::xmlKeyLfoDelay().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { C::xmlKeyRange().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyPitchLfoDepth().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyPulseWidth().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyPwmSource().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeySawEnabled().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyPulseEnabled().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeySubLevel().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyNoiseLevel().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { C::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { C::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { C::xmlKeyLpfResonance().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyEnvPolarity().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyEnvMod().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyFilterLfoDepth().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyKeyTrack().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { C::xmlKeyVcaMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyVelocitySensitivity().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { C::xmlKeyAttack().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyDecay().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { C::xmlKeySustain().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { C::xmlKeyRelease().toStdString(), 0.3f, 0, 10000, 3000, 100 });

    addParameter(Parameter { C::xmlKeyChorusMode().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });

    addParameter(Parameter { C::xmlKeyPortamento().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyPitchBendRange().toStdString(), 2.0f, 0, 24, 2, 1, Parameter::Type::Discrete });

    for (size_t i = 0; i < m_voices.size(); i++) {
        m_voices[i].noiseState = static_cast<uint32_t>(0x9E3779B9u * (i + 1));
    }
    for (auto && voice : m_voices) {
        voice.saw.setWaveform(PolyBlepOscillator::Waveform::Saw);
        voice.pulse.setWaveform(PolyBlepOscillator::Waveform::Square);
        voice.sub.setWaveform(PolyBlepOscillator::Waveform::Square);
        voice.env.setCurve(EnvelopeCurve);
        voice.gate.setAttackTime(GateAttackSeconds);
        voice.gate.setDecayTime(0.0);
        voice.gate.setSustainLevel(1.0);
        voice.gate.setReleaseTime(GateReleaseSeconds);
    }
    m_lfo.setWaveform(Lfo::Waveform::Triangle);
    m_hpf.setMode(CascadedSvf::Mode::HighPass);
    m_hpf.setOrder(2);

    JuSynthDevice::syncParameters();

    // What a parameter automation may drive, and how it reads out.
    using Display = ParameterDisplay;
    DeviceParameterDisplay::filters(*this);
    setParameterDisplay(C::xmlKeyLfoRate().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "Hz", .min = LfoMinHz, .max = LfoMaxHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "LFO Rate") });
    DeviceParameterDisplay::percent(*this, C::xmlKeyPitchLfoDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "DCO LFO"));
    DeviceParameterDisplay::percent(*this, C::xmlKeyPulseWidth(), QT_TRANSLATE_NOOP("ParameterDisplay", "PWM"));
    DeviceParameterDisplay::percent(*this, C::xmlKeySubLevel(), QT_TRANSLATE_NOOP("ParameterDisplay", "Sub Level"));
    DeviceParameterDisplay::percent(*this, C::xmlKeyNoiseLevel(), QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Level"));
    DeviceParameterDisplay::percent(*this, C::xmlKeyEnvMod(), QT_TRANSLATE_NOOP("ParameterDisplay", "Env Mod"));
    DeviceParameterDisplay::percent(*this, C::xmlKeyFilterLfoDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "VCF LFO"));
}

JuSynthDevice::~JuSynthDevice() = default;

std::string JuSynthDevice::name() const
{
    return m_name;
}

std::string JuSynthDevice::category() const
{
    return C::xmlValueSynths().toStdString();
}

std::string JuSynthDevice::typeName() const
{
    return Constants::juSynthDeviceName().toStdString();
}

std::string JuSynthDevice::typeIdString()
{
    return "ad85029e-0a1c-43c4-9969-653516e0e1f8";
}

std::string JuSynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> JuSynthDevice::deviceMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SoundController5), "LPF" },
        { static_cast<uint8_t>(Controller::GeneralPurpose6), "HPF" }
    };
}

void JuSynthDevice::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void JuSynthDevice::processDeviceMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void JuSynthDevice::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
        } else {
            const float val = static_cast<float>(value) / 127.0f;
            const auto automate = [&](const QString & key) {
                if (auto p = parameter(key.toStdString()); p) {
                    p->get().setAutomationValue(val);
                    syncParameters();
                    changed = true;
                }
            };

            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) {
                automate(C::xmlKeyLpfCutoff());
            } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) {
                automate(C::xmlKeyHpfCutoff());
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void JuSynthDevice::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_pitchBend = value;
}

void JuSynthDevice::processMidiProgramChange(uint8_t program, uint8_t)
{
    // Transport traffic: a program change in a song may move the whole panel, but the patch the
    // user saved has to still be there when playback stops.
    applyPreset(program, false);
}

void JuSynthDevice::loadPreset(int index)
{
    applyPreset(index, true);
}

void JuSynthDevice::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        const auto & presets = JuSynthPresets::presets();
        if (index < 0 || index >= static_cast<int>(presets.size())) {
            return;
        }

        // A preset is the whole panel, so everything it does not name goes back to its default
        // first -- on the same layer the preset itself is about to be written to.
        if (authored) {
            reset();
        } else {
            for (auto && [name, p] : parameters()) {
                p.setAutomationValue(p.defaultValue());
            }
        }

        for (auto && [name, value] : presets[static_cast<size_t>(index)].parameters) {
            if (const auto p = parameter(name); p) {
                if (authored) {
                    p->get().setValue(value);
                } else {
                    p->get().setAutomationValue(value);
                }
            }
        }

        syncParameters();
    }

    if (authored) {
        emit dataChanged();
    } else {
        emit parametersChanged();
    }
}

void JuSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.env.release();
            voice.gate.release();
        }
        voice.gated = false;
    }
}

void JuSynthDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    const uint8_t oversampleFactor = clampOversampleFactor(context.oversampleFactor);
    const uint32_t oversampledRate = context.sampleRate * oversampleFactor;
    const size_t frames = context.frameCount;

    // The LFO runs whether or not anything is playing, as the Juno's does, so that a note lands on
    // wherever it has got to rather than always on the same phase.
    m_lfo.setSampleRate(context.sampleRate);
    m_lfo.setFrequency(ParameterMapper::mapExponential(m_lfoRate, LfoMinHz, LfoMaxHz));
    m_lfoBlock.resize(frames);
    const double delaySeconds = static_cast<double>(m_lfoDelay) * LfoMaxDelaySeconds;
    for (size_t i = 0; i < frames; i++) {
        double level = 1.0;
        if (delaySeconds > 0.0) {
            const double hold = delaySeconds * 0.5;
            level = std::clamp((m_lfoDelayElapsed - hold) / (delaySeconds - hold), 0.0, 1.0);
        }
        m_lfoDelayElapsed += 1.0 / context.sampleRate;
        m_lfoBlock[i] = m_lfo.nextSample() * level;
    }

    // Silent once the last voice and the few milliseconds of the chorus and filters after it are
    // done, rather than running six idle voices' worth of output stages forever.
    if (std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; })) {
        m_tailFrames = static_cast<uint32_t>(TailSeconds * context.sampleRate);
    } else if (m_tailFrames == 0) {
        return;
    } else {
        m_tailFrames -= std::min<uint32_t>(m_tailFrames, static_cast<uint32_t>(frames));
    }

    m_oversampledBuffer.assign(frames * oversampleFactor, 0.0f);

    const double portamentoTime = ParameterMapper::mapExponential(m_portamento, PortamentoMinSeconds, PortamentoMaxSeconds);
    const double portamentoCoeff = m_portamento > 0.0f ? 1.0 - std::pow(0.001, 1.0 / (portamentoTime * oversampledRate)) : 1.0;
    const double pitchBendRatio = std::exp2((static_cast<double>(m_pitchBend) - 8192.0) / 8192.0 * m_pitchBendRange / 12.0);

    for (auto && voice : m_voices) {
        if (voice.active) {
            renderVoice(voice, frames, oversampleFactor, oversampledRate, portamentoCoeff, pitchBendRatio);
        }
    }

    // The high pass and the chorus are on the sum of the voices, as on the Juno, at the output rate.
    m_hpf.setSampleRate(context.sampleRate);
    m_hpf.setCutoff(static_cast<double>(m_hpfCutoff));
    m_dcBlocker.setSampleRate(context.sampleRate);
    m_chorus.setSampleRate(static_cast<double>(context.sampleRate));
    m_chorus.setMode(m_chorusMode);

    const double panAngle = static_cast<double>(panInternal()) * std::numbers::pi * 0.5;
    // Constant power, centre at unity: a mono synth panned to the middle comes out as loud as it went in.
    const double panL = std::cos(panAngle) * std::numbers::sqrt2;
    const double panR = std::sin(panAngle) * std::numbers::sqrt2;

    for (size_t i = 0; i < frames; i++) {
        const double mono = m_hpf.process(m_dcBlocker.process(static_cast<double>(m_decimator.process(&m_oversampledBuffer[i * oversampleFactor], oversampleFactor))));
        double left = 0.0;
        double right = 0.0;
        m_chorus.process(mono, left, right);
        context.buffer[i * 2] += left * panL;
        context.buffer[i * 2 + 1] += right * panR;
    }
}

void JuSynthDevice::renderVoice(Voice & voice, size_t frameCount, uint8_t oversampleFactor, uint32_t oversampledRate, double portamentoCoeff, double pitchBendRatio)
{
    voice.saw.setSampleRate(oversampledRate);
    voice.pulse.setSampleRate(oversampledRate);
    voice.sub.setSampleRate(oversampledRate);
    voice.env.setSampleRate(oversampledRate);
    voice.gate.setSampleRate(oversampledRate);
    voice.vcf.setSampleRate(oversampledRate);
    voice.vcf.setResonance(static_cast<double>(m_lpfResonance));

    // The 1/MaxVoices is the headroom for a full chord, as in the Synth
    const double velocityGain = 1.0 - static_cast<double>(m_velocitySensitivity) * (1.0 - static_cast<double>(voice.velocity));
    const double gain = (1.0 / MaxVoices) * static_cast<double>(linearGainInternal()) * velocityGain;

    const double highestHz = std::min(FilterHighestHz, oversampledRate * 0.49);
    const double octavesOfTravel = std::log2(highestHz / FilterLowestHz);
    // Full key follow moves the cutoff an octave per octave played, from middle C
    const double keyTrackOffset = static_cast<double>(m_keyTrack) * (static_cast<double>(voice.note) - 60.0) / 12.0 / std::log2(FilterHighestHz / FilterLowestHz);
    const double envSign = m_envInverted ? -1.0 : 1.0;
    const double pitchLfoSemitones = static_cast<double>(m_pitchLfoDepth * m_pitchLfoDepth) * MaxPitchLfoSemitones;
    const double rangeRatio = octaveRatio(m_range);

    for (size_t i = 0; i < frameCount; i++) {
        const double lfo = m_lfoBlock[i];
        const double vibrato = pitchLfoSemitones > 0.0 ? std::exp2(lfo * pitchLfoSemitones / 12.0) : 1.0;
        const double cutoffBase = static_cast<double>(m_lpfCutoff) + lfo * static_cast<double>(m_filterLfoDepth) * MaxFilterLfoSweep + keyTrackOffset;

        for (uint8_t os = 0; os < oversampleFactor; os++) {
            voice.glideFrequency += (voice.frequency - voice.glideFrequency) * portamentoCoeff;
            const double frequency = voice.glideFrequency * rangeRatio * pitchBendRatio * vibrato;

            const double env = voice.env.nextSample();
            const double gate = voice.gate.nextSample();

            double pwmAmount = static_cast<double>(m_pulseWidth);
            if (m_pwmSource == PwmSource::Lfo) {
                pwmAmount *= 0.5 * (lfo + 1.0);
            } else if (m_pwmSource == PwmSource::Envelope) {
                pwmAmount *= env;
            }

            // All three run whatever is switched in, so that they stay locked: the sub is the saw
            // divided by two, and switching a wave in mid-note must not put it out of phase.
            voice.saw.setFrequency(frequency);
            voice.pulse.setFrequency(frequency);
            voice.pulse.setPulseWidth(0.5 - MaxPulseNarrowing * pwmAmount);
            voice.sub.setFrequency(frequency * 0.5);
            const double saw = voice.saw.nextSample();
            const double pulse = voice.pulse.nextSample();
            const double sub = voice.sub.nextSample();

            double mix = sub * static_cast<double>(m_subLevel);
            if (m_sawEnabled) {
                mix += saw;
            }
            if (m_pulseEnabled) {
                mix += pulse;
            }
            if (m_noiseLevel > 0.0f) {
                mix += nextNoise(voice) * static_cast<double>(m_noiseLevel);
            }

            const double cutoff = std::clamp(cutoffBase + envSign * static_cast<double>(m_envMod) * env, 0.0, 1.0);
            const double cutoffHz = FilterLowestHz * std::exp2(cutoff * octavesOfTravel);
            voice.vcf.setCutoff(cutoffHz);
            const double filtered = voice.vcf.process(mix * FilterInputLevel) * FilterMakeupGain;

            const double amp = m_vcaMode == VcaMode::Gate ? gate : env;
            m_oversampledBuffer[i * oversampleFactor + os] += static_cast<float>(filtered * amp * gain);
        }
    }

    const auto & ampEnvelope = m_vcaMode == VcaMode::Gate ? voice.gate : voice.env;
    if (ampEnvelope.isSilent()) {
        voice.active = false;
    }
}

double JuSynthDevice::nextNoise(Voice & voice) const
{
    // xorshift32: cheap, and the same sequence on every render, so a bounce is repeatable
    auto & x = voice.noiseState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return static_cast<double>(x) / 2147483648.0 - 1.0;
}

bool JuSynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

void JuSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void JuSynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (size_t i = 0; i < m_voices.size(); i++) {
        m_voices[i].reset();
        m_voices[i].noiseState = static_cast<uint32_t>(0x9E3779B9u * (i + 1));
    }
    m_noteCounter = 0;
    m_lfo.reset();
    m_lfoDelayElapsed = 0.0;
    m_decimator.reset();
    m_dcBlocker.reset();
    m_hpf.reset();
    m_chorus.reset();
    m_tailFrames = 0;
}

void JuSynthDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(C::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(C::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(C::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void JuSynthDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == C::xmlKeyDevice()) {
                break;
            }
            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == C::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == C::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == C::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
    }
    emit dataChanged();
}

void JuSynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    // The LFO delay starts over when a key goes down with none held, as on the Juno
    if (std::ranges::none_of(m_voices, [](const auto & voice) { return voice.gated; })) {
        m_lfoDelayElapsed = 0.0;
    }

    auto & voice = m_voices[allocateVoice(note)];
    const double frequency = 440.0 * std::exp2((static_cast<double>(note) - 69.0) / 12.0);
    if (!voice.active) {
        // A fresh voice starts from silence, its oscillators in step and its filter empty
        voice.saw.reset();
        voice.pulse.reset();
        voice.sub.reset();
        voice.vcf.reset();
        voice.env.reset();
        voice.gate.reset();
    }
    if (voice.glideFrequency == 0.0 || m_portamento <= 0.0f) {
        voice.glideFrequency = frequency;
    }
    voice.frequency = frequency;
    voice.note = note;
    voice.velocity = static_cast<float>(velocity) / 127.0f;
    voice.active = true;
    voice.gated = true;
    voice.startedAt = ++m_noteCounter;
    voice.env.trigger();
    voice.gate.trigger();
}

void JuSynthDevice::handleNoteOff(uint8_t note)
{
    for (auto && voice : m_voices) {
        if (voice.gated && voice.note == note) {
            voice.env.release();
            voice.gate.release();
            voice.gated = false;
        }
    }
}

size_t JuSynthDevice::allocateVoice(uint8_t note)
{
    // The same note again takes its own voice back, then a silent voice, then the oldest released
    // one, and only then the oldest of all.
    for (size_t i = 0; i < m_voices.size(); i++) {
        if (m_voices[i].active && m_voices[i].note == note) {
            return i;
        }
    }
    for (size_t i = 0; i < m_voices.size(); i++) {
        if (!m_voices[i].active) {
            return i;
        }
    }
    std::optional<size_t> oldestReleased;
    size_t oldest = 0;
    for (size_t i = 0; i < m_voices.size(); i++) {
        if (!m_voices[i].gated && (!oldestReleased || m_voices[i].startedAt < m_voices[*oldestReleased].startedAt)) {
            oldestReleased = i;
        }
        if (m_voices[i].startedAt < m_voices[oldest].startedAt) {
            oldest = i;
        }
    }
    return oldestReleased.value_or(oldest);
}

void JuSynthDevice::syncParameters()
{
    Device::syncParameters();
    const auto value = [this](const QString & key, auto & target) {
        if (const auto p = parameter(key.toStdString()); p) {
            target = p->get().value();
        }
    };
    const auto index = [this](const QString & key) {
        const auto p = parameter(key.toStdString());
        return p ? static_cast<int>(p->get().xmlValue()) : 0;
    };
    const auto flag = [this](const QString & key) {
        const auto p = parameter(key.toStdString());
        return p && p->get().value() > 0.5f;
    };

    value(C::xmlKeyLfoRate(), m_lfoRate);
    value(C::xmlKeyLfoDelay(), m_lfoDelay);
    m_range = std::clamp(index(C::xmlKeyRange()), 0, 2);
    value(C::xmlKeyPitchLfoDepth(), m_pitchLfoDepth);
    value(C::xmlKeyPulseWidth(), m_pulseWidth);
    m_pwmSource = static_cast<PwmSource>(std::clamp(index(C::xmlKeyPwmSource()), 0, 2));
    m_sawEnabled = flag(C::xmlKeySawEnabled());
    m_pulseEnabled = flag(C::xmlKeyPulseEnabled());
    value(C::xmlKeySubLevel(), m_subLevel);
    value(C::xmlKeyNoiseLevel(), m_noiseLevel);
    value(C::xmlKeyHpfCutoff(), m_hpfCutoff);
    value(C::xmlKeyLpfCutoff(), m_lpfCutoff);
    value(C::xmlKeyLpfResonance(), m_lpfResonance);
    m_envInverted = flag(C::xmlKeyEnvPolarity());
    value(C::xmlKeyEnvMod(), m_envMod);
    value(C::xmlKeyFilterLfoDepth(), m_filterLfoDepth);
    value(C::xmlKeyKeyTrack(), m_keyTrack);
    m_vcaMode = static_cast<VcaMode>(std::clamp(index(C::xmlKeyVcaMode()), 0, 1));
    value(C::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
    value(C::xmlKeyAttack(), m_attack);
    value(C::xmlKeyDecay(), m_decay);
    value(C::xmlKeySustain(), m_sustain);
    value(C::xmlKeyRelease(), m_release);
    m_chorusMode = static_cast<JunoChorus::Mode>(std::clamp(index(C::xmlKeyChorusMode()), 0, 3));
    value(C::xmlKeyPortamento(), m_portamento);
    m_pitchBendRange = index(C::xmlKeyPitchBendRange());

    for (auto && voice : m_voices) {
        voice.env.setAttackTime(ParameterMapper::mapExponential(m_attack, AttackMinSeconds, AttackMaxSeconds));
        voice.env.setDecayTime(ParameterMapper::mapExponential(m_decay, DecayMinSeconds, DecayMaxSeconds));
        voice.env.setSustainLevel(static_cast<double>(m_sustain));
        voice.env.setReleaseTime(ParameterMapper::mapExponential(m_release, ReleaseMinSeconds, ReleaseMaxSeconds));
    }
}

float JuSynthDevice::lfoRate() const
{
    return shownParameterValue(C::xmlKeyLfoRate(), m_lfoRate);
}

void JuSynthDevice::setLfoRate(float rate)
{
    setContinuousParameterValue(C::xmlKeyLfoRate().toStdString(), rate);
}

float JuSynthDevice::lfoDelay() const
{
    return shownParameterValue(C::xmlKeyLfoDelay(), m_lfoDelay);
}

void JuSynthDevice::setLfoDelay(float delay)
{
    setContinuousParameterValue(C::xmlKeyLfoDelay().toStdString(), delay);
}

int JuSynthDevice::range() const
{
    return m_range;
}

void JuSynthDevice::setRange(int range)
{
    setDiscreteParameterValue(C::xmlKeyRange().toStdString(), range);
}

float JuSynthDevice::pitchLfoDepth() const
{
    return shownParameterValue(C::xmlKeyPitchLfoDepth(), m_pitchLfoDepth);
}

void JuSynthDevice::setPitchLfoDepth(float depth)
{
    setContinuousParameterValue(C::xmlKeyPitchLfoDepth().toStdString(), depth);
}

float JuSynthDevice::pulseWidth() const
{
    return shownParameterValue(C::xmlKeyPulseWidth(), m_pulseWidth);
}

void JuSynthDevice::setPulseWidth(float width)
{
    setContinuousParameterValue(C::xmlKeyPulseWidth().toStdString(), width);
}

JuSynthDevice::PwmSource JuSynthDevice::pwmSource() const
{
    return m_pwmSource;
}

void JuSynthDevice::setPwmSource(PwmSource source)
{
    setDiscreteParameterValue(C::xmlKeyPwmSource().toStdString(), static_cast<int>(source));
}

bool JuSynthDevice::sawEnabled() const
{
    return m_sawEnabled;
}

void JuSynthDevice::setSawEnabled(bool enabled)
{
    setContinuousParameterValue(C::xmlKeySawEnabled().toStdString(), enabled ? 1.0f : 0.0f);
}

bool JuSynthDevice::pulseEnabled() const
{
    return m_pulseEnabled;
}

void JuSynthDevice::setPulseEnabled(bool enabled)
{
    setContinuousParameterValue(C::xmlKeyPulseEnabled().toStdString(), enabled ? 1.0f : 0.0f);
}

float JuSynthDevice::subLevel() const
{
    return shownParameterValue(C::xmlKeySubLevel(), m_subLevel);
}

void JuSynthDevice::setSubLevel(float level)
{
    setContinuousParameterValue(C::xmlKeySubLevel().toStdString(), level);
}

float JuSynthDevice::noiseLevel() const
{
    return shownParameterValue(C::xmlKeyNoiseLevel(), m_noiseLevel);
}

void JuSynthDevice::setNoiseLevel(float level)
{
    setContinuousParameterValue(C::xmlKeyNoiseLevel().toStdString(), level);
}

float JuSynthDevice::hpfCutoff() const
{
    return shownParameterValue(C::xmlKeyHpfCutoff(), m_hpfCutoff);
}

void JuSynthDevice::setHpfCutoff(float cutoff)
{
    setContinuousParameterValue(C::xmlKeyHpfCutoff().toStdString(), cutoff);
}

float JuSynthDevice::lpfCutoff() const
{
    return shownParameterValue(C::xmlKeyLpfCutoff(), m_lpfCutoff);
}

void JuSynthDevice::setLpfCutoff(float cutoff)
{
    setContinuousParameterValue(C::xmlKeyLpfCutoff().toStdString(), cutoff);
}

float JuSynthDevice::lpfResonance() const
{
    return shownParameterValue(C::xmlKeyLpfResonance(), m_lpfResonance);
}

void JuSynthDevice::setLpfResonance(float resonance)
{
    setContinuousParameterValue(C::xmlKeyLpfResonance().toStdString(), resonance);
}

bool JuSynthDevice::envInverted() const
{
    return m_envInverted;
}

void JuSynthDevice::setEnvInverted(bool inverted)
{
    setContinuousParameterValue(C::xmlKeyEnvPolarity().toStdString(), inverted ? 1.0f : 0.0f);
}

float JuSynthDevice::envMod() const
{
    return shownParameterValue(C::xmlKeyEnvMod(), m_envMod);
}

void JuSynthDevice::setEnvMod(float amount)
{
    setContinuousParameterValue(C::xmlKeyEnvMod().toStdString(), amount);
}

float JuSynthDevice::filterLfoDepth() const
{
    return shownParameterValue(C::xmlKeyFilterLfoDepth(), m_filterLfoDepth);
}

void JuSynthDevice::setFilterLfoDepth(float depth)
{
    setContinuousParameterValue(C::xmlKeyFilterLfoDepth().toStdString(), depth);
}

float JuSynthDevice::keyTrack() const
{
    return shownParameterValue(C::xmlKeyKeyTrack(), m_keyTrack);
}

void JuSynthDevice::setKeyTrack(float keyTrack)
{
    setContinuousParameterValue(C::xmlKeyKeyTrack().toStdString(), keyTrack);
}

JuSynthDevice::VcaMode JuSynthDevice::vcaMode() const
{
    return m_vcaMode;
}

void JuSynthDevice::setVcaMode(VcaMode mode)
{
    setDiscreteParameterValue(C::xmlKeyVcaMode().toStdString(), static_cast<int>(mode));
}

float JuSynthDevice::velocitySensitivity() const
{
    return shownParameterValue(C::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
}

void JuSynthDevice::setVelocitySensitivity(float sensitivity)
{
    setContinuousParameterValue(C::xmlKeyVelocitySensitivity().toStdString(), sensitivity);
}

float JuSynthDevice::attack() const
{
    return shownParameterValue(C::xmlKeyAttack(), m_attack);
}

void JuSynthDevice::setAttack(float attack)
{
    setContinuousParameterValue(C::xmlKeyAttack().toStdString(), attack);
}

float JuSynthDevice::decay() const
{
    return shownParameterValue(C::xmlKeyDecay(), m_decay);
}

void JuSynthDevice::setDecay(float decay)
{
    setContinuousParameterValue(C::xmlKeyDecay().toStdString(), decay);
}

float JuSynthDevice::sustain() const
{
    return shownParameterValue(C::xmlKeySustain(), m_sustain);
}

void JuSynthDevice::setSustain(float sustain)
{
    setContinuousParameterValue(C::xmlKeySustain().toStdString(), sustain);
}

float JuSynthDevice::release() const
{
    return shownParameterValue(C::xmlKeyRelease(), m_release);
}

void JuSynthDevice::setRelease(float release)
{
    setContinuousParameterValue(C::xmlKeyRelease().toStdString(), release);
}

JunoChorus::Mode JuSynthDevice::chorusMode() const
{
    return m_chorusMode;
}

void JuSynthDevice::setChorusMode(JunoChorus::Mode mode)
{
    setDiscreteParameterValue(C::xmlKeyChorusMode().toStdString(), static_cast<int>(mode));
}

float JuSynthDevice::portamento() const
{
    return shownParameterValue(C::xmlKeyPortamento(), m_portamento);
}

void JuSynthDevice::setPortamento(float portamento)
{
    setContinuousParameterValue(C::xmlKeyPortamento().toStdString(), portamento);
}

int JuSynthDevice::pitchBendRange() const
{
    return m_pitchBendRange;
}

void JuSynthDevice::setPitchBendRange(int range)
{
    setDiscreteParameterValue(C::xmlKeyPitchBendRange().toStdString(), range);
}

} // namespace noteahead
