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

#include "noise_fx_device.hpp"

#include "device_parameter_display.hpp"
#include "noise_fx_presets.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <type_traits>

namespace noteahead {

using namespace Constants;

namespace {

//! Puts the noise, which comes out at the RMS of full-scale white noise, near the level the other
//! devices play at rather than at the top of the meter.
constexpr double NoiseGain = 0.25;
//! A sine at this gain has the RMS of the noise at NoiseGain, so the blend keeps the level.
constexpr double ToneGain = 0.2;
//! Scales the noise to unit RMS for the ring modulator, so that ring modulation keeps the tone's
//! level: the noise comes out at the RMS of uniform white noise, 1 / sqrt(3).
const double RingNoiseScale = std::sqrt(3.0);

constexpr double MinCutoffHz = 20.0;
constexpr double MaxCutoffHz = 20000.0;
//! Keeps a corner or a tone where the sample rate can still carry it.
constexpr double MaxFrequencyRatio = 0.45;
constexpr double MaxResonance = 0.98;
//! Saturation at the bottom and the top of the drive knob. The bottom keeps the filter linear.
constexpr double MinSaturation = 0.05;
constexpr double MaxSaturation = 20.0;

constexpr double MinHoldRateHz = 50.0;
constexpr double MaxHoldRateHz = 50000.0;
constexpr double CrackleKnee = 1.0;
constexpr double MaxCrackleDensity = 2000.0;

constexpr double PitchRangeSemitones = 24.0;
//! A full sweep or LFO depth moves the pitch this far: a siren or a laser needs more than the knob.
constexpr double PitchModulationSemitones = 48.0;
constexpr double MinToneHz = 1.0;

constexpr double MinLfoHz = 0.05;
constexpr double MaxLfoHz = 50.0;

constexpr double GateMinWidth = 0.05;
constexpr double GateMaxWidth = 0.95;
constexpr double GateMinSmoothingSeconds = 0.0005;
constexpr double GateMaxSmoothingSeconds = 0.05;

constexpr double MinAttackSeconds = 0.001;
constexpr double MaxAttackSeconds = 10.0;
constexpr double MinDecaySeconds = 0.005;
constexpr double MaxDecaySeconds = 20.0;
constexpr double MinSweepSeconds = 0.01;
constexpr double MaxSweepSeconds = 60.0;

constexpr double KeyTrackCentreNote = 60.0;
constexpr double TuningReferenceNote = 69.0;
constexpr double TuningReferenceHz = 440.0;

//! Riser out of the box: Sweep A opens the filter over four bars.
constexpr int DefaultSyncLength = 6;
constexpr float DefaultSweepACurve = 0.75f;
constexpr float DefaultSweepACutoffDepth = 0.9f;
//! An eighth note: the gate's resting division, for when its depth is turned up.
constexpr int DefaultGateRate = 3;

constexpr uint32_t NoiseSeedBase = 0x401;
constexpr uint32_t SideNoiseSeedBase = 0x5EED;
constexpr uint32_t CrackleSeedBase = 0xC4AC;

double bipolar(float value)
{
    return (static_cast<double>(value) - 0.5) * 2.0;
}

//! A factor that follows @p value01 by @p depth: positive rises with it, negative falls.
double fade(double depth, double value01)
{
    return depth >= 0.0 ? 1.0 - depth * (1.0 - value01) : 1.0 + depth * value01;
}

//! The entry of @p list that @p position, 0..1, falls on.
template<typename T>
size_t indexOf(double position, const std::vector<T> & list)
{
    const auto last = static_cast<double>(list.size() - 1);
    return static_cast<size_t>(std::lround(std::clamp(position, 0.0, 1.0) * last));
}

} // namespace

const std::vector<double> & NoiseFxDevice::syncLengthBeats()
{
    static const std::vector<double> beats = { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0 };
    return beats;
}

const std::vector<double> & NoiseFxDevice::gateDivisionBeats()
{
    static const std::vector<double> beats = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625 };
    return beats;
}

const std::vector<double> & NoiseFxDevice::lfoSyncBars()
{
    static const std::vector<double> bars = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125 };
    return bars;
}

NoiseFxDevice::Voice::Voice(uint32_t noiseSeed, uint32_t sideNoiseSeed, uint32_t crackleSeed)
  : noise { noiseSeed }
  , sideNoise { sideNoiseSeed }
  , crackle { crackleSeed }
{
}

void NoiseFxDevice::Voice::reset()
{
    noise.reset();
    sideNoise.reset();
    crackle.reset();
    tone.reset();
    filterL.reset();
    filterR.reset();
    amp.reset();
    for (auto && sweep : sweeps) {
        sweep = {};
    }
    lfo.reset();
    lfoGlideFrom = 0.0;
    lfoGlideTo.reset();
    gatePhase = 0.0;
    gateLevel = 1.0;
    active = false;
    releaseAtSweepEnd = false;
}

NoiseFxDevice::NoiseFxDevice(std::string name)
  : m_name { std::move(name) }
{
    for (uint32_t i = 0; i < VoiceCount; i++) {
        m_voices.emplace_back(NoiseSeedBase + i, SideNoiseSeedBase + i, CrackleSeedBase + i);
    }

    const auto continuous = [this](const QString & key, float value) {
        addParameter(Parameter { key.toStdString(), value, 0, 10000, static_cast<int>(std::lround(value * 10000.0f)), 100 });
    };
    continuous(NahdXml::xmlKeyColor(), m_colour);
    continuous(NahdXml::xmlKeySampleHoldRate(), m_sampleHoldRate);
    continuous(NahdXml::xmlKeyCrackle(), m_crackle);
    continuous(NahdXml::xmlKeyNoiseLevel(), m_noiseLevel);
    continuous(NahdXml::xmlKeyWidth(), m_width);
    addParameter(Parameter { NahdXml::xmlKeyWaveform().toStdString(), static_cast<float>(m_toneWaveform), 0, 3, static_cast<int>(m_toneWaveform), 1, Parameter::Type::Discrete });
    continuous(NahdXml::xmlKeyPitch(), m_pitch);
    continuous(NahdXml::xmlKeyBlend(), m_blend);
    continuous(NahdXml::xmlKeyRingMod(), m_ringMod);
    addParameter(Parameter { NahdXml::xmlKeyFilterType().toStdString(), 0.0f, 0, FilterTypeCount - 1, 0, 1, Parameter::Type::Discrete });
    continuous(NahdXml::xmlKeyCutoff(), m_cutoff);
    continuous(NahdXml::xmlKeyResonance(), m_resonance);
    continuous(NahdXml::xmlKeyDrive(), m_drive);
    continuous(NahdXml::xmlKeyKeyTrack(), m_keyTrack);
    continuous(NahdXml::xmlKeyAmpAttack(), m_ampAttack);
    continuous(NahdXml::xmlKeyAmpDecay(), m_ampDecay);
    continuous(NahdXml::xmlKeyAmpSustain(), m_ampSustain);
    continuous(NahdXml::xmlKeyAmpRelease(), m_ampRelease);
    continuous(NahdXml::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
    addParameter(Parameter { NahdXml::xmlKeyPlayToEnd().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });

    for (size_t sweep = 0; sweep < SweepCount; sweep++) {
        addSweepParameters(sweep);
    }
    addLfoParameters();
    addGateParameters();

    NoiseFxDevice::syncParameters();

    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeyColor(), QT_TRANSLATE_NOOP("ParameterDisplay", "Colour"));
    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeySampleHoldRate(), QT_TRANSLATE_NOOP("ParameterDisplay", "S&H Rate"));
    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeyCrackle(), QT_TRANSLATE_NOOP("ParameterDisplay", "Crackle"));
    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeyNoiseLevel(), QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Level"));
    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeyWidth(), QT_TRANSLATE_NOOP("ParameterDisplay", "Width"));
    DeviceParameterDisplay::bipolar(*this, NahdXml::xmlKeyPitch(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch"));
    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeyBlend(), QT_TRANSLATE_NOOP("ParameterDisplay", "Blend"));
    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeyRingMod(), QT_TRANSLATE_NOOP("ParameterDisplay", "Ring Mod"));
    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeyCutoff(), QT_TRANSLATE_NOOP("ParameterDisplay", "Cutoff"));
    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeyResonance(), QT_TRANSLATE_NOOP("ParameterDisplay", "Resonance"));
    DeviceParameterDisplay::percent(*this, NahdXml::xmlKeyDrive(), QT_TRANSLATE_NOOP("ParameterDisplay", "Drive"));
}

NoiseFxDevice::~NoiseFxDevice() = default;

void NoiseFxDevice::addRouteParameters(const std::string & targetKeyBase, const std::string & depthKeyBase, Destination defaultTarget, float defaultDepth)
{
    for (size_t destination = 0; destination < DestinationCount; destination++) {
        const auto suffix = std::to_string(destination + 1);
        const auto target = destination == 0 ? defaultTarget : Destination::None;
        const float depth = destination == 0 ? defaultDepth : 0.5f;
        addParameter(Parameter { targetKeyBase + suffix, static_cast<float>(target), 0, DestinationTypeCount - 1, static_cast<int>(target), 1, Parameter::Type::Discrete });
        addParameter(Parameter { depthKeyBase + suffix, depth, 0, 10000, static_cast<int>(std::lround(depth * 10000.0f)), 100 });
    }
}

void NoiseFxDevice::addSweepParameters(size_t sweep)
{
    const auto continuous = [this](const std::string & key, float value) {
        addParameter(Parameter { key, value, 0, 10000, static_cast<int>(std::lround(value * 10000.0f)), 100 });
    };
    const auto discrete = [this](const std::string & key, int value, int min, int max, Parameter::Type type) {
        addParameter(Parameter { key, static_cast<float>(value), min, max, value, 1, type });
    };

    const bool isSweepA = sweep == 0;
    continuous(sweepKey(sweep, NahdXml::xmlKeyStart()), 0.0f);
    continuous(sweepKey(sweep, NahdXml::xmlKeyEnd()), 1.0f);
    continuous(sweepKey(sweep, NahdXml::xmlKeyCurve()), isSweepA ? DefaultSweepACurve : 0.5f);
    continuous(sweepKey(sweep, NahdXml::xmlKeyLength()), 0.5f);
    discrete(sweepKey(sweep, NahdXml::xmlKeyMode()), static_cast<int>(SweepEnvelope::Mode::Hold), 0, 2, Parameter::Type::Discrete);
    discrete(sweepKey(sweep, NahdXml::xmlKeySync()), 1, 0, 1, Parameter::Type::Boolean);
    discrete(sweepKey(sweep, NahdXml::xmlKeySyncLength()), DefaultSyncLength, 0, static_cast<int>(syncLengthBeats().size()) - 1, Parameter::Type::Discrete);
    addRouteParameters(sweepKey(sweep, NahdXml::xmlKeyTarget()), sweepKey(sweep, NahdXml::xmlKeyDepth()), isSweepA ? Destination::Cutoff : Destination::None, isSweepA ? DefaultSweepACutoffDepth : 0.5f);
}

void NoiseFxDevice::addLfoParameters()
{
    addParameter(Parameter { lfoKey(NahdXml::xmlKeyWaveform()), static_cast<float>(Lfo::Waveform::Sine), 0, SmoothRandomWaveform, static_cast<int>(Lfo::Waveform::Sine), 1, Parameter::Type::Discrete });
    addParameter(Parameter { lfoKey(NahdXml::xmlKeySync()), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { lfoKey(NahdXml::xmlKeyRate()), 0.5f, 0, 10000, 5000, 100 });
    addRouteParameters(lfoKey(NahdXml::xmlKeyTarget()), lfoKey(NahdXml::xmlKeyDepth()), Destination::None, 0.5f);
}

void NoiseFxDevice::addGateParameters()
{
    const auto rateCount = static_cast<int>(gateDivisionBeats().size());
    addParameter(Parameter { gateKey(NahdXml::xmlKeyRate()), static_cast<float>(DefaultGateRate), 0, rateCount - 1, DefaultGateRate, 1, Parameter::Type::Discrete });
    addParameter(Parameter { gateKey(NahdXml::xmlKeyDepth()), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { gateKey(NahdXml::xmlKeyWidth()), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { gateKey(NahdXml::xmlKeySmoothing()), 0.3f, 0, 10000, 3000, 100 });
}

std::string NoiseFxDevice::sweepKey(size_t sweep, const QString & key)
{
    return "sweep" + std::to_string(sweep + 1) + "_" + key.toStdString();
}

std::string NoiseFxDevice::sweepDestinationKey(size_t sweep, const QString & key, size_t destination)
{
    return sweepKey(sweep, key) + std::to_string(destination + 1);
}

std::string NoiseFxDevice::lfoKey(const QString & key)
{
    return "lfo_" + key.toStdString();
}

std::string NoiseFxDevice::lfoDestinationKey(const QString & key, size_t destination)
{
    return lfoKey(key) + std::to_string(destination + 1);
}

std::string NoiseFxDevice::gateKey(const QString & key)
{
    return "gate_" + key.toStdString();
}

std::string NoiseFxDevice::name() const
{
    return m_name;
}

std::string NoiseFxDevice::category() const
{
    return NahdXml::xmlValueSynths().toStdString();
}

std::string NoiseFxDevice::typeName() const
{
    return noiseFxDeviceName().toStdString();
}

std::string NoiseFxDevice::typeIdString()
{
    return "6e0b3f2a-9d47-4c1e-b85a-2f7c91d4e0a6";
}

std::string NoiseFxDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> NoiseFxDevice::deviceMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" }
    };
}

void NoiseFxDevice::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    auto & voice = m_voices.at(allocateVoice());
    voice.filterL.setSampleRate(sampleRate());
    voice.filterR.setSampleRate(sampleRate());
    voice.amp.setSampleRate(sampleRate());
    configureAmp(voice.amp);
    for (size_t sweep = 0; sweep < SweepCount; sweep++) {
        voice.sweeps.at(sweep).setSampleRate(sampleRate());
        configureSweep(voice.sweeps.at(sweep), m_sweeps.at(sweep));
        voice.sweeps.at(sweep).trigger();
    }
    voice.lfo.trigger();
    voice.lfoGlideTo.reset();
    voice.gatePhase = 0.0;
    voice.gateLevel = 1.0;
    voice.amp.trigger();

    const double velocity01 = static_cast<double>(velocity) / 127.0;
    voice.velocityGain = 1.0 - static_cast<double>(m_velocitySensitivity) * (1.0 - velocity01);
    voice.note = note;
    voice.active = true;
    voice.releaseAtSweepEnd = m_playToEnd && m_sweeps.front().mode == SweepEnvelope::Mode::Hold;
}

void NoiseFxDevice::processDeviceMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note && !voice.releaseAtSweepEnd) {
            releaseVoice(voice);
        }
    }
}

void NoiseFxDevice::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
        } else if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
            changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
        } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
            changed |= updatePanParameter(static_cast<float>(value) / 127.0f, false);
        }
    }
    if (changed) {
        emit parametersChanged();
    }
}

void NoiseFxDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active) {
            releaseVoice(voice);
        }
    }
}

void NoiseFxDevice::processMidiProgramChange(uint8_t program, uint8_t)
{
    applyPreset(program, false);
}

void NoiseFxDevice::loadPreset(int index)
{
    applyPreset(index, true);
}

void NoiseFxDevice::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        const auto & presets = NoiseFxPresets::presets();
        if (index < 0 || index >= static_cast<int>(presets.size())) {
            return;
        }

        // A preset is the whole panel, so everything it does not name goes back to its default
        // first, on the layer the preset is about to be written to.
        if (authored) {
            reset();
        } else {
            for (auto && [name, p] : parameters()) {
                p.setAutomationValue(p.defaultValue());
            }
        }

        for (auto && [name, value] : presets.at(static_cast<size_t>(index)).parameters) {
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

void NoiseFxDevice::releaseVoice(Voice & voice)
{
    voice.amp.release();
    voice.releaseAtSweepEnd = false;
}

size_t NoiseFxDevice::allocateVoice()
{
    for (size_t i = 0; i < m_voices.size(); i++) {
        if (!m_voices.at(i).active) {
            return i;
        }
    }
    const size_t stolen = m_nextVoiceToSteal;
    m_nextVoiceToSteal = (m_nextVoiceToSteal + 1) % m_voices.size();
    return stolen;
}

void NoiseFxDevice::configureSweep(SweepEnvelope & envelope, const SweepSettings & settings) const
{
    envelope.setStartValue(static_cast<double>(settings.start));
    envelope.setEndValue(static_cast<double>(settings.end));
    envelope.setCurve(bipolar(settings.curve));
    envelope.setMode(settings.mode);
    if (settings.sync) {
        envelope.setLengthBeats(syncLengthBeats().at(static_cast<size_t>(settings.syncLength)));
    } else {
        envelope.setLengthSeconds(ParameterMapper::mapExponential(static_cast<double>(settings.length), MinSweepSeconds, MaxSweepSeconds));
    }
}

void NoiseFxDevice::configureAmp(AdsrEnvelope & amp) const
{
    amp.setAttackTime(ParameterMapper::mapExponential(static_cast<double>(m_ampAttack), MinAttackSeconds, MaxAttackSeconds));
    amp.setDecayTime(ParameterMapper::mapExponential(static_cast<double>(m_ampDecay), MinDecaySeconds, MaxDecaySeconds));
    amp.setSustainLevel(static_cast<double>(m_ampSustain));
    amp.setReleaseTime(ParameterMapper::mapExponential(static_cast<double>(m_ampRelease), MinDecaySeconds, MaxDecaySeconds));
}

void NoiseFxDevice::applyRoute(const Route & route, double value, double value01, bool isLfo, Modulation & modulation)
{
    for (size_t destination = 0; destination < DestinationCount; destination++) {
        const double depth = bipolar(route.depths.at(destination));
        const double offset = depth * value;
        switch (route.targets.at(destination)) {
        case Destination::None:
            break;
        case Destination::Cutoff:
            modulation.cutoff += offset;
            break;
        case Destination::Resonance:
            modulation.resonance += offset;
            break;
        case Destination::Colour:
            modulation.colour += offset;
            break;
        case Destination::SampleHoldRate:
            modulation.holdRate += offset;
            break;
        case Destination::Level:
            modulation.level *= fade(depth, value01);
            break;
        case Destination::Pan:
            modulation.pan += offset * 0.5;
            break;
        case Destination::Pitch:
            modulation.pitch += offset;
            break;
        case Destination::Blend:
            modulation.blend += offset;
            break;
        case Destination::LfoRate:
            if (!isLfo) {
                modulation.lfoRate += offset;
            }
            break;
        case Destination::LfoDepth:
            if (!isLfo) {
                modulation.lfoDepth *= fade(depth, value01);
            }
            break;
        case Destination::GateRate:
            modulation.gateRate += offset;
            break;
        }
    }
}

double NoiseFxDevice::lfoFrequency(double rate, double bpm) const
{
    if (m_lfo.sync) {
        const double bars = lfoSyncBars().at(indexOf(rate, lfoSyncBars()));
        return bpm / 60.0 * 0.25 / bars;
    }
    return ParameterMapper::mapExponential(rate, MinLfoHz, MaxLfoHz);
}

double NoiseFxDevice::nextLfoValue(Voice & voice) const
{
    const double phase = voice.lfo.phase();
    const double value = voice.lfo.nextSample();
    if (m_lfo.waveform != SmoothRandomWaveform) {
        return value;
    }

    // Random draws its next value as the cycle wraps, which is where the glide has just arrived at
    // the last one, so the curve has no step anywhere.
    if (!voice.lfoGlideTo) {
        voice.lfoGlideFrom = value;
        voice.lfoGlideTo = value;
    } else if (value != *voice.lfoGlideTo) {
        voice.lfoGlideFrom = *voice.lfoGlideTo;
        voice.lfoGlideTo = value;
    }
    const double glide = (1.0 - std::cos(std::numbers::pi * phase)) * 0.5;
    return voice.lfoGlideFrom + (*voice.lfoGlideTo - voice.lfoGlideFrom) * glide;
}

void NoiseFxDevice::renderVoice(Voice & voice, double sampleRate, double bpm, double & left, double & right)
{
    Modulation modulation;
    for (size_t sweep = 0; sweep < SweepCount; sweep++) {
        const double value = voice.sweeps.at(sweep).nextSample();
        applyRoute(m_sweeps.at(sweep).route, value, value, false, modulation);
    }

    // The LFO comes after the sweeps so that they can move its rate and depth.
    voice.lfo.setFrequency(lfoFrequency(static_cast<double>(m_lfo.rate) + modulation.lfoRate, bpm));
    const double lfoValue = nextLfoValue(voice) * std::max(0.0, modulation.lfoDepth);
    applyRoute(m_lfo.route, lfoValue, (lfoValue + 1.0) * 0.5, true, modulation);

    // Noise
    const double colour = static_cast<double>(m_colour) + modulation.colour;
    voice.noise.setColour(colour);
    voice.sideNoise.setColour(colour);
    const double holdRate = std::clamp(static_cast<double>(m_sampleHoldRate) + modulation.holdRate, 0.0, 1.0);
    const auto holdRateHz = holdRate < 1.0 ? std::optional { ParameterMapper::mapExponential(holdRate, MinHoldRateHz, MaxHoldRateHz) } : std::nullopt;
    voice.noise.setHoldRate(holdRateHz);
    voice.sideNoise.setHoldRate(holdRateHz);
    const double mainNoise = voice.noise.nextSample();
    const double sideNoise = voice.sideNoise.nextSample();
    // Mid and side of two uncorrelated noises: each channel keeps unit level, and their correlation
    // runs from one at no width down to none at full width.
    const double angle = static_cast<double>(m_width) * std::numbers::pi * 0.25;
    const double noiseL = std::cos(angle) * mainNoise + std::sin(angle) * sideNoise;
    const double noiseR = std::cos(angle) * mainNoise - std::sin(angle) * sideNoise;

    // Tone
    const double semitones = bipolar(m_pitch) * PitchRangeSemitones + modulation.pitch * PitchModulationSemitones;
    const double toneHz = TuningReferenceHz * std::exp2((static_cast<double>(voice.note) - TuningReferenceNote + semitones) / 12.0);
    voice.tone.setFrequency(std::clamp(toneHz, MinToneHz, sampleRate * MaxFrequencyRatio));
    const double ringMod = static_cast<double>(m_ringMod);
    const double dryTone = voice.tone.nextSample();
    const double tone = dryTone * (1.0 - ringMod + ringMod * mainNoise * RingNoiseScale) * ToneGain;

    const double blend = std::clamp(static_cast<double>(m_blend) + modulation.blend, 0.0, 1.0);
    const double crackle = voice.crackle.nextSample();
    const double noiseLevel = static_cast<double>(m_noiseLevel) * static_cast<double>(m_noiseLevel);
    const double noiseGain = NoiseGain * noiseLevel * (1.0 - blend);
    const double inputL = noiseL * noiseGain + tone * blend + crackle;
    const double inputR = noiseR * noiseGain + tone * blend + crackle;

    // Filter
    const double keyTrackOctaves = (static_cast<double>(voice.note) - KeyTrackCentreNote) / 12.0 * static_cast<double>(m_keyTrack);
    const double cutoffHz = ParameterMapper::mapExponential(static_cast<double>(m_cutoff) + modulation.cutoff, MinCutoffHz, MaxCutoffHz) * std::exp2(keyTrackOctaves);
    const double clampedCutoffHz = std::clamp(cutoffHz, MinCutoffHz, sampleRate * MaxFrequencyRatio);
    const double resonance = std::clamp(static_cast<double>(m_resonance) + modulation.resonance, 0.0, 1.0) * MaxResonance;
    const auto filter = [this, clampedCutoffHz, resonance](SaturatingSvf & svf, double input) {
        svf.setCutoff(clampedCutoffHz);
        svf.setResonance(resonance);
        const double lowPass = svf.process(input);
        switch (m_filterType) {
        case FilterType::LowPass:
            return lowPass;
        case FilterType::BandPass:
            return svf.bandPass();
        case FilterType::HighPass:
            return svf.highPass();
        case FilterType::Notch:
            return lowPass + svf.highPass();
        }
        return lowPass;
    };
    const double filteredL = filter(voice.filterL, inputL);
    const double filteredR = filter(voice.filterR, inputR);

    // Gate
    const auto & divisions = gateDivisionBeats();
    const double gatePosition = static_cast<double>(m_gate.rate) / static_cast<double>(divisions.size() - 1) + modulation.gateRate;
    const double gateBeats = divisions.at(indexOf(gatePosition, divisions));
    voice.gatePhase += bpm / 60.0 / sampleRate / gateBeats;
    voice.gatePhase -= std::floor(voice.gatePhase);
    const double gateWidth = GateMinWidth + (GateMaxWidth - GateMinWidth) * static_cast<double>(m_gate.width);
    const double gateTarget = voice.gatePhase < gateWidth ? 1.0 : 0.0;
    voice.gateLevel = gateTarget + (voice.gateLevel - gateTarget) * m_gateSmoothingCoefficient;
    const double gate = 1.0 - static_cast<double>(m_gate.depth) * (1.0 - voice.gateLevel);

    // Amp
    if (voice.releaseAtSweepEnd && !voice.sweeps.front().isRunning()) {
        releaseVoice(voice);
    }
    const double amp = voice.amp.nextSample();
    if (voice.amp.isSilent() || !voice.amp.isActive()) {
        voice.active = false;
    }
    const double gain = amp * voice.velocityGain * std::max(0.0, modulation.level) * gate;

    left = filteredL * gain;
    right = filteredR * gain;
    m_voicePanner.setPan(std::clamp(0.5 + modulation.pan, 0.0, 1.0));
    m_voicePanner.process(left, right);
}

void NoiseFxDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    const double sampleRate = static_cast<double>(context.sampleRate);
    const double saturation = ParameterMapper::mapExponential(static_cast<double>(m_drive), MinSaturation, MaxSaturation);
    const double crackleDensity = ParameterMapper::mapExponentialFromZero(static_cast<double>(m_crackle), CrackleKnee, MaxCrackleDensity);
    const double gateSmoothingSeconds = ParameterMapper::mapExponential(static_cast<double>(m_gate.smoothing), GateMinSmoothingSeconds, GateMaxSmoothingSeconds);
    m_gateSmoothingCoefficient = std::exp(-1.0 / (gateSmoothingSeconds * sampleRate));
    for (auto && voice : m_voices) {
        voice.noise.setSampleRate(sampleRate);
        voice.sideNoise.setSampleRate(sampleRate);
        voice.crackle.setSampleRate(sampleRate);
        voice.crackle.setDensity(crackleDensity);
        voice.tone.setSampleRate(sampleRate);
        voice.tone.setWaveform(m_toneWaveform);
        for (auto * filter : { &voice.filterL, &voice.filterR }) {
            filter->setSampleRate(sampleRate);
            filter->setSaturation(saturation);
        }
        voice.amp.setSampleRate(sampleRate);
        configureAmp(voice.amp);
        voice.lfo.setSampleRate(sampleRate);
        voice.lfo.setWaveform(m_lfo.waveform == SmoothRandomWaveform ? Lfo::Waveform::Random : static_cast<Lfo::Waveform>(m_lfo.waveform));
        // The tempo is picked up on every block, so a synced sweep follows a tempo change in the
        // middle of it and still lands on the beat.
        for (size_t sweep = 0; sweep < SweepCount; sweep++) {
            auto & envelope = voice.sweeps.at(sweep);
            envelope.setSampleRate(sampleRate);
            envelope.setBpm(context.bpm);
            configureSweep(envelope, m_sweeps.at(sweep));
        }
    }
    m_panner.setPan(static_cast<double>(panInternal()));
    const double gain = linearGainInternal();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        double outL = 0.0;
        double outR = 0.0;
        for (auto && voice : m_voices) {
            if (!voice.active) {
                continue;
            }
            double voiceL = 0.0;
            double voiceR = 0.0;
            renderVoice(voice, sampleRate, context.bpm, voiceL, voiceR);
            outL += voiceL;
            outR += voiceR;
        }
        m_panner.process(outL, outR);
        context.buffer[i * 2] += outL * gain;
        context.buffer[i * 2 + 1] += outR * gain;
    }
}

bool NoiseFxDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

void NoiseFxDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void NoiseFxDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.reset();
    }
    m_nextVoiceToSteal = 0;
}

void NoiseFxDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void NoiseFxDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == NahdXml::xmlKeyDevice()) {
                break;
            }
            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == NahdXml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == NahdXml::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == NahdXml::xmlKeyParameter()) {
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

namespace {

//! Reads @p key into @p member as the member's type: continuous as is, the rest rounded.
template<typename Container, typename Member>
void readParameter(const Container & container, const std::string & key, Member & member)
{
    if (const auto p = container.parameter(key); p) {
        if constexpr (std::is_same_v<Member, float>) {
            member = p->get().value();
        } else if constexpr (std::is_same_v<Member, bool>) {
            member = p->get().value() > 0.5f;
        } else {
            member = static_cast<Member>(std::lround(p->get().value()));
        }
    }
}

} // namespace

void NoiseFxDevice::syncRoute(const std::string & targetKeyBase, const std::string & depthKeyBase, Route & route)
{
    for (size_t destination = 0; destination < DestinationCount; destination++) {
        const auto suffix = std::to_string(destination + 1);
        readParameter(*this, targetKeyBase + suffix, route.targets.at(destination));
        readParameter(*this, depthKeyBase + suffix, route.depths.at(destination));
    }
}

void NoiseFxDevice::syncParameters()
{
    Device::syncParameters();

    readParameter(*this, NahdXml::xmlKeyColor().toStdString(), m_colour);
    readParameter(*this, NahdXml::xmlKeySampleHoldRate().toStdString(), m_sampleHoldRate);
    readParameter(*this, NahdXml::xmlKeyCrackle().toStdString(), m_crackle);
    readParameter(*this, NahdXml::xmlKeyNoiseLevel().toStdString(), m_noiseLevel);
    readParameter(*this, NahdXml::xmlKeyWidth().toStdString(), m_width);
    readParameter(*this, NahdXml::xmlKeyWaveform().toStdString(), m_toneWaveform);
    readParameter(*this, NahdXml::xmlKeyPitch().toStdString(), m_pitch);
    readParameter(*this, NahdXml::xmlKeyBlend().toStdString(), m_blend);
    readParameter(*this, NahdXml::xmlKeyRingMod().toStdString(), m_ringMod);
    readParameter(*this, NahdXml::xmlKeyFilterType().toStdString(), m_filterType);
    readParameter(*this, NahdXml::xmlKeyCutoff().toStdString(), m_cutoff);
    readParameter(*this, NahdXml::xmlKeyResonance().toStdString(), m_resonance);
    readParameter(*this, NahdXml::xmlKeyDrive().toStdString(), m_drive);
    readParameter(*this, NahdXml::xmlKeyKeyTrack().toStdString(), m_keyTrack);
    readParameter(*this, NahdXml::xmlKeyAmpAttack().toStdString(), m_ampAttack);
    readParameter(*this, NahdXml::xmlKeyAmpDecay().toStdString(), m_ampDecay);
    readParameter(*this, NahdXml::xmlKeyAmpSustain().toStdString(), m_ampSustain);
    readParameter(*this, NahdXml::xmlKeyAmpRelease().toStdString(), m_ampRelease);
    readParameter(*this, NahdXml::xmlKeyVelocitySensitivity().toStdString(), m_velocitySensitivity);
    readParameter(*this, NahdXml::xmlKeyPlayToEnd().toStdString(), m_playToEnd);

    for (size_t sweep = 0; sweep < SweepCount; sweep++) {
        auto & settings = m_sweeps.at(sweep);
        readParameter(*this, sweepKey(sweep, NahdXml::xmlKeyStart()), settings.start);
        readParameter(*this, sweepKey(sweep, NahdXml::xmlKeyEnd()), settings.end);
        readParameter(*this, sweepKey(sweep, NahdXml::xmlKeyCurve()), settings.curve);
        readParameter(*this, sweepKey(sweep, NahdXml::xmlKeyLength()), settings.length);
        readParameter(*this, sweepKey(sweep, NahdXml::xmlKeyMode()), settings.mode);
        readParameter(*this, sweepKey(sweep, NahdXml::xmlKeySync()), settings.sync);
        readParameter(*this, sweepKey(sweep, NahdXml::xmlKeySyncLength()), settings.syncLength);
        syncRoute(sweepKey(sweep, NahdXml::xmlKeyTarget()), sweepKey(sweep, NahdXml::xmlKeyDepth()), settings.route);
    }

    readParameter(*this, lfoKey(NahdXml::xmlKeyWaveform()), m_lfo.waveform);
    readParameter(*this, lfoKey(NahdXml::xmlKeySync()), m_lfo.sync);
    readParameter(*this, lfoKey(NahdXml::xmlKeyRate()), m_lfo.rate);
    syncRoute(lfoKey(NahdXml::xmlKeyTarget()), lfoKey(NahdXml::xmlKeyDepth()), m_lfo.route);

    readParameter(*this, gateKey(NahdXml::xmlKeyRate()), m_gate.rate);
    readParameter(*this, gateKey(NahdXml::xmlKeyDepth()), m_gate.depth);
    readParameter(*this, gateKey(NahdXml::xmlKeyWidth()), m_gate.width);
    readParameter(*this, gateKey(NahdXml::xmlKeySmoothing()), m_gate.smoothing);
}

float NoiseFxDevice::keyedParameterValue(const std::string & key) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (const auto p = parameter(key); p) {
        const auto & parameter = p->get();
        return parameter.isDiscrete() || parameter.isBoolean() ? parameter.value() : shownParameterValue(QString::fromStdString(key), parameter.value());
    }
    return 0.0f;
}

void NoiseFxDevice::setKeyedParameterValue(const std::string & key, float value)
{
    if (const auto p = parameter(key); p) {
        if (p->get().isDiscrete() || p->get().isBoolean()) {
            setDiscreteParameterValue(key, static_cast<int>(std::lround(value)));
        } else {
            setContinuousParameterValue(key, value);
        }
    }
}

float NoiseFxDevice::colour() const
{
    return shownParameterValue(NahdXml::xmlKeyColor(), m_colour);
}

void NoiseFxDevice::setColour(float colour)
{
    setContinuousParameterValue(NahdXml::xmlKeyColor().toStdString(), colour);
}

float NoiseFxDevice::sampleHoldRate() const
{
    return shownParameterValue(NahdXml::xmlKeySampleHoldRate(), m_sampleHoldRate);
}

void NoiseFxDevice::setSampleHoldRate(float rate)
{
    setContinuousParameterValue(NahdXml::xmlKeySampleHoldRate().toStdString(), rate);
}

float NoiseFxDevice::crackle() const
{
    return shownParameterValue(NahdXml::xmlKeyCrackle(), m_crackle);
}

void NoiseFxDevice::setCrackle(float crackle)
{
    setContinuousParameterValue(NahdXml::xmlKeyCrackle().toStdString(), crackle);
}

float NoiseFxDevice::noiseLevel() const
{
    return shownParameterValue(NahdXml::xmlKeyNoiseLevel(), m_noiseLevel);
}

void NoiseFxDevice::setNoiseLevel(float level)
{
    setContinuousParameterValue(NahdXml::xmlKeyNoiseLevel().toStdString(), level);
}

float NoiseFxDevice::width() const
{
    return shownParameterValue(NahdXml::xmlKeyWidth(), m_width);
}

void NoiseFxDevice::setWidth(float width)
{
    setContinuousParameterValue(NahdXml::xmlKeyWidth().toStdString(), width);
}

PolyBlepOscillator::Waveform NoiseFxDevice::toneWaveform() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_toneWaveform;
}

void NoiseFxDevice::setToneWaveform(PolyBlepOscillator::Waveform waveform)
{
    setDiscreteParameterValue(NahdXml::xmlKeyWaveform().toStdString(), static_cast<int>(waveform));
}

float NoiseFxDevice::pitch() const
{
    return shownParameterValue(NahdXml::xmlKeyPitch(), m_pitch);
}

void NoiseFxDevice::setPitch(float pitch)
{
    setContinuousParameterValue(NahdXml::xmlKeyPitch().toStdString(), pitch);
}

float NoiseFxDevice::blend() const
{
    return shownParameterValue(NahdXml::xmlKeyBlend(), m_blend);
}

void NoiseFxDevice::setBlend(float blend)
{
    setContinuousParameterValue(NahdXml::xmlKeyBlend().toStdString(), blend);
}

float NoiseFxDevice::ringMod() const
{
    return shownParameterValue(NahdXml::xmlKeyRingMod(), m_ringMod);
}

void NoiseFxDevice::setRingMod(float ringMod)
{
    setContinuousParameterValue(NahdXml::xmlKeyRingMod().toStdString(), ringMod);
}

NoiseFxDevice::FilterType NoiseFxDevice::filterType() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_filterType;
}

void NoiseFxDevice::setFilterType(FilterType type)
{
    setDiscreteParameterValue(NahdXml::xmlKeyFilterType().toStdString(), static_cast<int>(type));
}

float NoiseFxDevice::cutoff() const
{
    return shownParameterValue(NahdXml::xmlKeyCutoff(), m_cutoff);
}

void NoiseFxDevice::setCutoff(float cutoff)
{
    setContinuousParameterValue(NahdXml::xmlKeyCutoff().toStdString(), cutoff);
}

float NoiseFxDevice::resonance() const
{
    return shownParameterValue(NahdXml::xmlKeyResonance(), m_resonance);
}

void NoiseFxDevice::setResonance(float resonance)
{
    setContinuousParameterValue(NahdXml::xmlKeyResonance().toStdString(), resonance);
}

float NoiseFxDevice::drive() const
{
    return shownParameterValue(NahdXml::xmlKeyDrive(), m_drive);
}

void NoiseFxDevice::setDrive(float drive)
{
    setContinuousParameterValue(NahdXml::xmlKeyDrive().toStdString(), drive);
}

float NoiseFxDevice::keyTrack() const
{
    return shownParameterValue(NahdXml::xmlKeyKeyTrack(), m_keyTrack);
}

void NoiseFxDevice::setKeyTrack(float keyTrack)
{
    setContinuousParameterValue(NahdXml::xmlKeyKeyTrack().toStdString(), keyTrack);
}

float NoiseFxDevice::ampAttack() const
{
    return shownParameterValue(NahdXml::xmlKeyAmpAttack(), m_ampAttack);
}

void NoiseFxDevice::setAmpAttack(float attack)
{
    setContinuousParameterValue(NahdXml::xmlKeyAmpAttack().toStdString(), attack);
}

float NoiseFxDevice::ampDecay() const
{
    return shownParameterValue(NahdXml::xmlKeyAmpDecay(), m_ampDecay);
}

void NoiseFxDevice::setAmpDecay(float decay)
{
    setContinuousParameterValue(NahdXml::xmlKeyAmpDecay().toStdString(), decay);
}

float NoiseFxDevice::ampSustain() const
{
    return shownParameterValue(NahdXml::xmlKeyAmpSustain(), m_ampSustain);
}

void NoiseFxDevice::setAmpSustain(float sustain)
{
    setContinuousParameterValue(NahdXml::xmlKeyAmpSustain().toStdString(), sustain);
}

float NoiseFxDevice::ampRelease() const
{
    return shownParameterValue(NahdXml::xmlKeyAmpRelease(), m_ampRelease);
}

void NoiseFxDevice::setAmpRelease(float release)
{
    setContinuousParameterValue(NahdXml::xmlKeyAmpRelease().toStdString(), release);
}

float NoiseFxDevice::velocitySensitivity() const
{
    return shownParameterValue(NahdXml::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
}

void NoiseFxDevice::setVelocitySensitivity(float sensitivity)
{
    setContinuousParameterValue(NahdXml::xmlKeyVelocitySensitivity().toStdString(), sensitivity);
}

bool NoiseFxDevice::playToEnd() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_playToEnd;
}

void NoiseFxDevice::setPlayToEnd(bool playToEnd)
{
    setDiscreteParameterValue(NahdXml::xmlKeyPlayToEnd().toStdString(), playToEnd ? 1 : 0);
}

float NoiseFxDevice::sweepParameterValue(size_t sweep, const QString & key) const
{
    return keyedParameterValue(sweepKey(sweep, key));
}

void NoiseFxDevice::setSweepParameterValue(size_t sweep, const QString & key, float value)
{
    setKeyedParameterValue(sweepKey(sweep, key), value);
}

NoiseFxDevice::Destination NoiseFxDevice::sweepTarget(size_t sweep, size_t destination) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sweeps.at(sweep).route.targets.at(destination);
}

void NoiseFxDevice::setSweepTarget(size_t sweep, size_t destination, Destination target)
{
    setDiscreteParameterValue(sweepDestinationKey(sweep, NahdXml::xmlKeyTarget(), destination), static_cast<int>(target));
}

float NoiseFxDevice::sweepDepth(size_t sweep, size_t destination) const
{
    return keyedParameterValue(sweepDestinationKey(sweep, NahdXml::xmlKeyDepth(), destination));
}

void NoiseFxDevice::setSweepDepth(size_t sweep, size_t destination, float depth)
{
    setContinuousParameterValue(sweepDestinationKey(sweep, NahdXml::xmlKeyDepth(), destination), depth);
}

float NoiseFxDevice::lfoParameterValue(const QString & key) const
{
    return keyedParameterValue(lfoKey(key));
}

void NoiseFxDevice::setLfoParameterValue(const QString & key, float value)
{
    setKeyedParameterValue(lfoKey(key), value);
}

NoiseFxDevice::Destination NoiseFxDevice::lfoTarget(size_t destination) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_lfo.route.targets.at(destination);
}

void NoiseFxDevice::setLfoTarget(size_t destination, Destination target)
{
    setDiscreteParameterValue(lfoDestinationKey(NahdXml::xmlKeyTarget(), destination), static_cast<int>(target));
}

float NoiseFxDevice::lfoDepth(size_t destination) const
{
    return keyedParameterValue(lfoDestinationKey(NahdXml::xmlKeyDepth(), destination));
}

void NoiseFxDevice::setLfoDepth(size_t destination, float depth)
{
    setContinuousParameterValue(lfoDestinationKey(NahdXml::xmlKeyDepth(), destination), depth);
}

float NoiseFxDevice::gateParameterValue(const QString & key) const
{
    return keyedParameterValue(gateKey(key));
}

void NoiseFxDevice::setGateParameterValue(const QString & key, float value)
{
    setKeyedParameterValue(gateKey(key), value);
}

} // namespace noteahead
