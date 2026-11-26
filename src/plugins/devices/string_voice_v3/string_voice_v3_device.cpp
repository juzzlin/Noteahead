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

#include "string_voice_v3_device.hpp"

#include "../../../domain/devices/device_parameter_display.hpp"
#include <QtGlobal>

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../../infra/data_service.hpp"
#include "../../../infra/midi/midi_cc_mapping.hpp"
#include "../../../infra/xml/nahd_xml_reader.hpp"
#include "../../../infra/xml/nahd_xml_writer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>

namespace noteahead {

namespace {

//! From a table, because it is asked for every sample of every voice.
double midiNoteToFreq(uint8_t note)
{
    static const auto frequencies = [] {
        std::array<double, 128> table {};
        for (size_t i = 0; i < table.size(); i++) {
            table[i] = 440.0 * std::pow(2.0, (static_cast<double>(i) - 69.0) / 12.0);
        }
        return table;
    }();
    return frequencies[note & 0x7F];
}

//! Level of a Strings section that is switched on. The hardware gives no control over it -- the
//! switches only say where the section sounds and Balance says how loud -- so it is a constant, set
//! to what the V1 8' register carried at its default.
constexpr double StringsSectionGain { 0.8 };

//! The strings source is a saw with its fundamental lifted. Measured against a VC340, harmonics 2
//! upwards sit at a saw's levels while the fundamental stands about 5.8 dB above one, so a sine at
//! the same frequency is added to make up the difference. A saw's fundamental is 2/pi of its peak,
//! and lifting it by 5.8 dB means adding 0.95 of that again. Negative because the oscillator's saw
//! carries its fundamental in antiphase with its sine, so adding the two as they come would cancel
//! the fundamental rather than reinforce it.
constexpr double StringsFundamentalLift { -0.95 * 2.0 / std::numbers::pi };

//! How far apart the Strings' unison copies sit, in cents between neighbours.
//!
//! Chosen from the instrument rather than by ear: the recordings' lowest modulation component sits
//! at about 1.1 Hz, and components this far apart beat at 1.06 Hz at C4. Being a ratio it holds at
//! every pitch, which is what the measurements show too.
constexpr double StringsDetuneCents { 7.0 };

//! Depth and rate of each copy's own tuning wander, which keeps the beating from being a fixed
//! pattern. Slow on purpose -- see the note in processAudio() about where shimmer becomes vibrato.
constexpr double StringsDriftCents { 3.0 };
constexpr double StringsDriftRateHz { 0.13 };

//! The Strings tone network, as measured with the hardware's Tone control at its middle: a gentle
//! first-order high pass and a second-order low pass. The Tone control sweeps the low pass and
//! leaves the high pass alone, so the middle of its travel reproduces the measurement.
constexpr double StringsHpfHz { 300.0 };
constexpr double StringsToneCentreHz { 4800.0 };
//! What the Tone control reaches at either end of its travel.
constexpr double StringsToneMinHz { 700.0 };
constexpr double StringsToneMaxHz { 16000.0 };

//! CascadedSvf takes a cutoff as a 0..1 position on a 20 Hz .. 20 kHz exponential scale.
double toCutoffPosition(double hz)
{
    return std::clamp(std::log2(hz / 20.0) / std::log2(20000.0 / 20.0), 0.0, 1.0);
}

//! High-accuracy exp2(x) for small |x| <= 0.12 (error < 5e-14).
double fastSmallExp2(double x)
{
    if (std::abs(x) > 0.15) {
        return std::exp2(x);
    }
    const double u = x * std::numbers::ln2;
    return 1.0 + u * (1.0 + u * (0.5 + u * ((1.0 / 6.0) + u * ((1.0 / 24.0) + u * ((1.0 / 120.0) + u * (1.0 / 720.0))))));
}

//! High-accuracy sin(2*pi*t) for t in [0, 1) (error < 7e-12).
double fastSineCycles(double t)
{
    if (t > 0.25) {
        if (t <= 0.75) {
            t = 0.5 - t;
        } else {
            t -= 1.0;
        }
    }
    const double x = (2.0 * std::numbers::pi) * t;
    const double x2 = x * x;
    return x * (1.0 + x2 * ((-1.0 / 6.0) + x2 * ((1.0 / 120.0) + x2 * ((-1.0 / 5040.0) + x2 * ((1.0 / 362880.0) + x2 * ((-1.0 / 39916800.0) + x2 * ((1.0 / 6227020800.0) + x2 * (-1.0 / 1307674368000.0))))))));
}

//! Where a voice's PWM, vibrato and drift start. They run on through every note, so a reset has to put
//! them back, or a render would start them wherever the session left them. Offset by voice, so that
//! simultaneous notes neither pulse nor vibrate in lockstep.
template<typename Voice>
void setStartPhases(Voice & v, int i, int maxVoices)
{
    v.pwmPhase = static_cast<double>(i) / static_cast<double>(maxVoices);
    v.vibratoPhase = static_cast<double>(i) / static_cast<double>(maxVoices);
    v.driftPhase = static_cast<double>((i * 7) % maxVoices) / static_cast<double>(maxVoices);
}

} // namespace

void StringVoiceV3Device::Voice::reset()
{
    for (int u { 0 }; u < StringUnisonCount; u++) {
        const auto i { static_cast<size_t>(u) };
        // Every copy starts at the same point in the cycle, and it has to. Spreading them around it
        // instead cancels the section: three copies a third of a cycle apart put their fundamentals
        // at 0, 120 and 240 degrees, which sum to nothing, and the same goes for every harmonic that
        // is not a multiple of three. The detuning fans them out within a cycle or two anyway.
        stringOsc[i].sync(0.0);
        stringDriftPhase[i] = static_cast<double>(u) / StringUnisonCount;
    }
    voiceOsc8.sync(0.0);
    voiceOsc4.sync(0.0);

    stringEg.reset();
    voiceEg.reset();

    note = 0;
    velocity = 1.0f;
    active = false;
    triggerFrame = 0;
    pan = 0.5;
}

StringVoiceV3Device::StringVoiceV3Device(std::string name)
  : m_name { std::move(name) }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsBalance().toStdString(), 1.0f, 0, 100, 100, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceBalance().toStdString(), 1.0f, 0, 100, 100, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsUpper().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsLower().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsTone().toStdString(), 0.5f, 0, 100, 50, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString(), 0.5f, 0, 100, 50, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsAttack().toStdString(), 0.2f, 0, 100, 20, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsRelease().toStdString(), 0.4f, 0, 100, 40, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMale8().toStdString(), 0.8f, 0, 100, 80, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMale4().toStdString(), 0.0f, 0, 100, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceUpperMale8().toStdString(), 0.8f, 0, 100, 80, 100, Parameter::Type::Continuous, { Constants::NahdXml::xmlKeyVoiceFemale8().toStdString() } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceFemale4().toStdString(), 0.0f, 0, 100, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceAttack().toStdString(), 0.25f, 0, 100, 25, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceRelease().toStdString(), 0.45f, 0, 100, 45, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVibratoRate().toStdString(), 0.596f, 0, 100, 60, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVibratoDepth().toStdString(), 0.0f, 0, 100, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVibratoDelay().toStdString(), 0.0f, 0, 100, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyEnsembleEnabled().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyEnsembleMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVocoderEnabled().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVocoderSidechain().toStdString(), -1.0f, -1, static_cast<int>(Constants::deviceRackSize()) - 1, -1, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.0f, 0, 10000, 0, 100 });

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);

    // Second order, which is what the measured curve asks for: the low pass drops about 3 dB in the
    // octave above its corner and about 12 in the next, and a steeper one overshoots both.
    for (auto * f : { &m_stringsLpfL, &m_stringsLpfR }) {
        f->setMode(CascadedSvf::Mode::LowPass);
        f->setOrder(2);
    }

    for (int i { 0 }; i < MaxVoices; ++i) {
        auto & v { m_voices[static_cast<size_t>(i)] };
        for (int u { 0 }; u < StringUnisonCount; u++) {
            const auto ui { static_cast<size_t>(u) };
            v.stringOsc[ui].setWaveform(PolyBlepOscillator::Waveform::Saw);
            // Rates that share no small common multiple, so the copies keep moving apart instead of
            // falling back into step with one another every few seconds.
            v.stringDriftPhase[ui] = static_cast<double>(u) / StringUnisonCount;
            v.stringDriftRateRatio[ui] = 1.0 + 0.37 * static_cast<double>(u);
        }
        v.voiceOsc8.setWaveform(PolyBlepOscillator::Waveform::Saw);
        v.voiceOsc4.setWaveform(PolyBlepOscillator::Waveform::Saw);

        v.stringEg.setSustainLevel(1.0);
        v.voiceEg.setSustainLevel(1.0);
        v.stringEg.setDecayTime(0.0);
        v.voiceEg.setDecayTime(0.0);

        // Fixed per-voice pitch drift (+/- 0.8 cents), mimicking the natural tuning
        // imprecision of independent analog divide-down oscillator channels. This
        // gives even a single sustained note some "analog" character before the
        // BBD ensemble chorus engages.
        const double spread { (static_cast<double>(i) - (MaxVoices - 1) / 2.0) / (MaxVoices - 1) };
        const double detuneCents { spread * 1.6 };
        v.detuneRatio = std::pow(2.0, detuneCents / 1200.0);

        // A choir is not one singer multiplied. Each voice gets its own vibrato phase and a rate a
        // couple of percent off the others, so they never settle into the lockstep a single shared
        // LFO gives them, plus a slow drift of its own that keeps the tuning from sitting perfectly
        // still the way only a synthesiser's does.
        v.vibratoRateRatio = 1.0 + spread * 0.08;
        v.driftRateRatio = 1.0 + spread * 0.55;
        setStartPhases(v, i, MaxVoices);
    }

    syncParameters();

    // What a parameter automation may drive, and how it reads out: the controls worth riding, in
    // the units their knobs show.
    DeviceParameterDisplay::filters(*this);
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVoiceBalance(), QT_TRANSLATE_NOOP("ParameterDisplay", "Human Voice"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyStringsBalance(), QT_TRANSLATE_NOOP("ParameterDisplay", "Strings"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVibratoRate(), QT_TRANSLATE_NOOP("ParameterDisplay", "Vibrato Rate"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVibratoDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "Vibrato Depth"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVibratoDelay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Vibrato Delay"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyPanSpread(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pan Spread"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyStringsAttack(), QT_TRANSLATE_NOOP("ParameterDisplay", "Strings Attack"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyStringsRelease(), QT_TRANSLATE_NOOP("ParameterDisplay", "Strings Release"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVoiceMale8(), QT_TRANSLATE_NOOP("ParameterDisplay", "Male 8'"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVoiceMale4(), QT_TRANSLATE_NOOP("ParameterDisplay", "Male 4'"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVoiceUpperMale8(), QT_TRANSLATE_NOOP("ParameterDisplay", "Upper Male 8'"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVoiceFemale8(), QT_TRANSLATE_NOOP("ParameterDisplay", "Female 8'"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVoiceFemale4(), QT_TRANSLATE_NOOP("ParameterDisplay", "Female 4'"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVoiceAttack(), QT_TRANSLATE_NOOP("ParameterDisplay", "Voice Attack"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVoiceRelease(), QT_TRANSLATE_NOOP("ParameterDisplay", "Voice Release"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyStringsTone(), QT_TRANSLATE_NOOP("ParameterDisplay", "Strings Tone"));
    DeviceParameterDisplay::percent(*this, Constants::NahdXml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity Sensitivity"));
}

StringVoiceV3Device::~StringVoiceV3Device() = default;

std::string StringVoiceV3Device::name() const
{
    return m_name;
}

std::string StringVoiceV3Device::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string StringVoiceV3Device::typeName() const
{
    return Constants::stringVoiceV3DeviceName().toStdString();
}

std::string StringVoiceV3Device::typeIdString()
{
    return "3b6571a4-fe03-4762-94c0-0c98db013ffe";
}

std::string StringVoiceV3Device::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> StringVoiceV3Device::deviceMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SustainPedal), "Sustain" }
    };
}

int StringVoiceV3Device::findVoiceForNote(uint8_t note) const
{
    for (int i { 0 }; i < MaxVoices; ++i) {
        if (m_voices[i].active && m_voices[i].note == note) {
            return i;
        }
    }
    return -1;
}

int StringVoiceV3Device::allocateVoice()
{
    // 1. Prefer a fully idle voice.
    for (int i { 0 }; i < MaxVoices; ++i) {
        if (!m_voices[static_cast<size_t>(i)].active) {
            return i;
        }
    }

    // 2. All voices are busy: prefer stealing one that's already releasing,
    //    picking the quietest one so the cut is as inaudible as possible.
    int bestReleasing { -1 };
    double lowestLevel { std::numeric_limits<double>::max() };
    for (int i { 0 }; i < MaxVoices; ++i) {
        const auto & v { m_voices[static_cast<size_t>(i)] };
        const bool releasing { v.stringEg.state() == AdsrEnvelope::State::Release && v.voiceEg.state() == AdsrEnvelope::State::Release };
        if (releasing) {
            const double level { v.stringEg.value() + v.voiceEg.value() };
            if (level < lowestLevel) {
                lowestLevel = level;
                bestReleasing = i;
            }
        }
    }
    if (bestReleasing >= 0) {
        return bestReleasing;
    }

    // 3. No voice is releasing either: steal the oldest one still sounding.
    int oldest { 0 };
    uint64_t oldestTriggerId { m_voices[0].triggerId };
    for (int i { 1 }; i < MaxVoices; ++i) {
        if (m_voices[static_cast<size_t>(i)].triggerId < oldestTriggerId) {
            oldestTriggerId = m_voices[static_cast<size_t>(i)].triggerId;
            oldest = i;
        }
    }
    return oldest;
}

void StringVoiceV3Device::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    // Trace, not info: every note of every pattern comes through here, and no other device
    // announces its notes at all.
    juzzlin::L("StringVoiceV3Device").trace() << "Note On: " << int(note) << " velocity: " << int(velocity);

    int idx { findVoiceForNote(note) };
    if (idx < 0) {
        idx = allocateVoice();
    }

    auto & v { m_voices[static_cast<size_t>(idx)] };
    v.reset();
    v.triggerId = m_nextTriggerId++;

    const double sampleRateVal { static_cast<double>(sampleRate()) };
    for (int u { 0 }; u < StringUnisonCount; u++) {
        v.stringOsc[static_cast<size_t>(u)].setSampleRate(sampleRateVal);
    }
    v.voiceOsc8.setSampleRate(sampleRateVal);
    v.voiceOsc4.setSampleRate(sampleRateVal);

    v.stringEg.setSampleRate(sampleRateVal);
    v.voiceEg.setSampleRate(sampleRateVal);

    const double strAttackSec { ParameterMapper::mapExponential(m_stringsAttack, 0.001, 5.0) };
    const double strReleaseSec { ParameterMapper::mapExponential(m_stringsRelease, 0.005, 10.0) };
    const double vocAttackSec { ParameterMapper::mapExponential(m_voiceAttack, 0.001, 5.0) };
    const double vocReleaseSec { ParameterMapper::mapExponential(m_voiceRelease, 0.005, 10.0) };

    v.stringEg.setAttackTime(strAttackSec);
    v.stringEg.setReleaseTime(strReleaseSec);
    v.voiceEg.setAttackTime(vocAttackSec);
    v.voiceEg.setReleaseTime(vocReleaseSec);

    v.stringEg.trigger();
    v.voiceEg.trigger();

    v.note = note;
    v.velocity = static_cast<float>(velocity) / 127.0f;
    v.active = true;
    v.sustained = false;
    v.triggerFrame = 0;
    const float side { (idx % 2 == 0) ? -1.0f : 1.0f };
    v.pan = 0.5 + (side * m_panSpread * 0.5);
}

void StringVoiceV3Device::processDeviceMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    juzzlin::L("StringVoiceV3Device").trace() << "Note Off: " << int(note);

    for (auto & v : m_voices) {
        if (v.active && v.note == note && !v.sustained) {
            if (m_sustainPedal) {
                v.sustained = true;
            } else {
                releaseVoice(v);
            }
        }
    }
}

void StringVoiceV3Device::releaseVoice(Voice & voice) const
{
    voice.stringEg.setReleaseTime(ParameterMapper::mapExponential(m_stringsRelease, 0.005, 10.0));
    voice.voiceEg.setReleaseTime(ParameterMapper::mapExponential(m_voiceRelease, 0.005, 10.0));
    voice.stringEg.release();
    voice.voiceEg.release();
}

void StringVoiceV3Device::liftSustainPedal()
{
    for (auto & v : m_voices) {
        if (v.active && v.sustained) {
            v.sustained = false;
            releaseVoice(v);
        }
    }
    m_sustainPedal = false;
}

void StringVoiceV3Device::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed { false };
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
            liftSustainPedal();
        } else if (controller == static_cast<uint8_t>(Controller::SustainPedal)) {
            if (value >= 64) {
                m_sustainPedal = true;
            } else {
                liftSustainPedal();
            }
        } else {
            const float val { static_cast<float>(value) / 127.0f };
            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void StringVoiceV3Device::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto & v : m_voices) {
        if (v.active) {
            v.sustained = false;
            v.stringEg.release();
            v.voiceEg.release();
        }
    }
}

void StringVoiceV3Device::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    if (context.frameCount == 0) {
        return;
    }

    const double sRate { static_cast<double>(context.sampleRate) };
    const double invSampleRate { 1.0 / sRate };
    const double vibRateHz { m_vibratoRate * 9.9 + 0.1 };

    m_lpfL.setSampleRate(sRate);
    m_lpfR.setSampleRate(sRate);
    m_hpfL.setSampleRate(sRate);
    m_hpfR.setSampleRate(sRate);
    m_lpfL.setCutoff(static_cast<double>(m_lpfCutoff));
    m_lpfR.setCutoff(static_cast<double>(m_lpfCutoff));
    m_hpfL.setCutoff(static_cast<double>(m_hpfCutoff));
    m_hpfR.setCutoff(static_cast<double>(m_hpfCutoff));

    const bool rateChanged { sRate != m_lastAudioSampleRate };
    if (rateChanged || m_stringsTone != m_lastStringsTone) {
        if (rateChanged) {
            m_stringsHpfL.calculate(StringsHpfHz, sRate);
            m_stringsHpfR.calculate(StringsHpfHz, sRate);
            m_stringsLpfL.setSampleRate(sRate);
            m_stringsLpfR.setSampleRate(sRate);
            m_formantFiltersL.setSampleRate(sRate);
            m_formantFiltersR.setSampleRate(sRate);
            m_ensemble.setSampleRate(sRate);
            m_vocoder.setSampleRate(sRate);
            for (auto & v : m_voices) {
                for (int u { 0 }; u < StringUnisonCount; u++) {
                    v.stringOsc[static_cast<size_t>(u)].setSampleRate(sRate);
                }
                v.voiceOsc8.setSampleRate(sRate);
                v.voiceOsc4.setSampleRate(sRate);
                v.stringEg.setSampleRate(sRate);
                v.voiceEg.setSampleRate(sRate);
            }
        }
        // Tone sweeps the low pass either side of where it was measured, the middle of the travel
        // landing on the measurement itself.
        const double tone { static_cast<double>(m_stringsTone) };
        const double toneHz { tone <= 0.5
                                ? StringsToneMinHz * std::pow(StringsToneCentreHz / StringsToneMinHz, tone * 2.0)
                                : StringsToneCentreHz * std::pow(StringsToneMaxHz / StringsToneCentreHz, (tone - 0.5) * 2.0) };
        const double cutoffPos { toCutoffPosition(toneHz) };
        m_stringsLpfL.setCutoff(cutoffPos);
        m_stringsLpfR.setCutoff(cutoffPos);
        m_lastAudioSampleRate = sRate;
        m_lastStringsTone = m_stringsTone;
    }

    m_panner.setPan(static_cast<double>(panInternal()));

    // Check Vocoder sidechain
    const auto scIdx { vocoderSidechainIndex() };
    const bool hasSidechain { m_vocoderEnabled && scIdx && *scIdx < context.deviceOutputBuffers.size() };
    const auto sidechainBuffer { hasSidechain ? context.deviceOutputBuffers[*scIdx] : std::span<const double> {} };

    const size_t frames { context.frameCount };
    if (m_stringsSumL.size() < frames) {
        m_stringsSumL.resize(frames);
        m_stringsSumR.resize(frames);
        m_maleSumL.resize(frames);
        m_maleSumR.resize(frames);
        m_femaleSumL.resize(frames);
        m_femaleSumR.resize(frames);
    }
    std::fill_n(m_stringsSumL.data(), frames, 0.0);
    std::fill_n(m_stringsSumR.data(), frames, 0.0);
    std::fill_n(m_maleSumL.data(), frames, 0.0);
    std::fill_n(m_maleSumR.data(), frames, 0.0);
    std::fill_n(m_femaleSumL.data(), frames, 0.0);
    std::fill_n(m_femaleSumR.data(), frames, 0.0);

    // No headroom compensation by voice count, deliberately. Scaling every voice by 1/sqrt(n)
    // keeps the total steady but makes each note's level depend on how many others happen to be
    // held, so releasing two notes of a chord audibly lifts the one still down. A divide-down
    // string machine simply sums its keys, and the peaks that leaves -- around 0.3 for a six-note
    // chord -- sit far enough below the safety soft-clip to need no correcting.

    constexpr double pwmRateHz { 0.13 }; // slow, subtle pulse-width drift for a less static choir tone

    // Depth and speed of the per-voice tuning drift. Small enough to read as a singer rather than
    // as a detuned oscillator: a held sung note wanders by a few cents over a second or two.
    constexpr double DriftCents { 3.5 };
    constexpr double DriftRateHz { 0.45 };
    constexpr double Inv1200 { 1.0 / 1200.0 };
    constexpr std::array<double, StringUnisonCount> StrOffsetOctaves {
        StringsDetuneCents * -1.0 / 1200.0,
        0.0,
        StringsDetuneCents * 1.0 / 1200.0
    };

    const bool hasVibrato { m_vibratoDepth > 0.0f };
    const double maxVibDepth { static_cast<double>(m_vibratoDepth) * 0.06 }; // scale depth to max ~1 semi-tone
    const double delaySamples { static_cast<double>(m_vibratoDelay) * 3.0 * sRate };
    const double deviceGain { linearGainInternal() * 0.25 };
    const double stringsBalance { static_cast<double>(m_stringsBalance) };
    const double voiceBalance { static_cast<double>(m_voiceBalance) };
    const double pwmInc { pwmRateHz * invSampleRate };

    double * __restrict strOutL { m_stringsSumL.data() };
    double * __restrict strOutR { m_stringsSumR.data() };
    double * __restrict maleOutBufL { m_maleSumL.data() };
    double * __restrict maleOutBufR { m_maleSumR.data() };
    double * __restrict femOutBufL { m_femaleSumL.data() };
    double * __restrict femOutBufR { m_femaleSumR.data() };

    for (auto & v : m_voices) {
        if (!v.active) {
            continue;
        }

        const bool upper { v.note >= SplitNote };
        const bool stringsOn { upper ? m_stringsUpper : m_stringsLower };
        const double maleLevel { static_cast<double>(upper ? m_voiceUpperMale8 : m_voiceMale8) };
        const double voice4Level { static_cast<double>(upper ? m_voiceFemale4 : m_voiceMale4) };

        const bool renderStrings { stringsOn && stringsBalance > 0.0 };
        const bool renderVoice8 { voiceBalance > 0.0 && maleLevel > 0.0 };
        const bool renderVoice4 { voiceBalance > 0.0 && voice4Level > 0.0 };
        const bool renderAnyVoice { renderVoice8 || renderVoice4 };

        // Sensitivity at 0 ignores how hard the key was struck, at 1 follows it the whole way.
        const double velocityLevel { 1.0 - m_velocitySensitivity + m_velocitySensitivity * static_cast<double>(v.velocity) };
        const double voiceGain { velocityLevel * deviceGain };
        const double strScale { (StringsSectionGain / StringUnisonCount) * voiceGain * stringsBalance };
        const double vocScale { voiceGain * voiceBalance };
        const double panL { 1.0 - v.pan };
        const double panR { v.pan };

        const double baseNoteFreq { midiNoteToFreq(v.note) * v.detuneRatio };
        const double vibInc { vibRateHz * v.vibratoRateRatio * invSampleRate };
        const double driftInc { DriftRateHz * v.driftRateRatio * invSampleRate };
        const std::array<double, StringUnisonCount> strDriftInc {
            StringsDriftRateHz * v.stringDriftRateRatio[0] * invSampleRate,
            StringsDriftRateHz * v.stringDriftRateRatio[1] * invSampleRate,
            StringsDriftRateHz * v.stringDriftRateRatio[2] * invSampleRate
        };

        for (uint32_t i { 0 }; i < context.frameCount; ++i) {
            const double strEnv { v.stringEg.nextSample() };
            const double vocEnv { v.voiceEg.nextSample() };

            if (v.stringEg.isSilent() && v.voiceEg.isSilent()) {
                v.active = false;
                break;
            }

            v.vibratoPhase += vibInc;
            if (v.vibratoPhase >= 1.0) {
                v.vibratoPhase -= 1.0;
            }

            double pitchModRatio { 1.0 };
            if (hasVibrato) {
                double currentVibratoDepth { maxVibDepth };
                if (delaySamples > 0.0) {
                    const double age { static_cast<double>(v.triggerFrame) };
                    if (age < delaySamples) {
                        currentVibratoDepth *= (age / delaySamples);
                    }
                }
                const double vibratoLfoVal { fastSineCycles(v.vibratoPhase) };
                pitchModRatio = fastSmallExp2(vibratoLfoVal * currentVibratoDepth);
            }
            v.triggerFrame++;

            v.driftPhase += driftInc;
            if (v.driftPhase >= 1.0) {
                v.driftPhase -= 1.0;
            }

            v.pwmPhase += pwmInc;
            if (v.pwmPhase >= 1.0) {
                v.pwmPhase -= 1.0;
            }

            const double baseFreq { baseNoteFreq * pitchModRatio };

            if (renderStrings) {
                double strSampleRaw { 0.0 };
                for (size_t ui { 0 }; ui < StringUnisonCount; ui++) {
                    v.stringDriftPhase[ui] += strDriftInc[ui];
                    if (v.stringDriftPhase[ui] >= 1.0) {
                        v.stringDriftPhase[ui] -= 1.0;
                    }
                    const double wander { fastSineCycles(v.stringDriftPhase[ui]) * StringsDriftCents };
                    const double copyFreq { baseFreq * fastSmallExp2(StrOffsetOctaves[ui] + wander * Inv1200) };

                    v.stringOsc[ui].setFrequency(copyFreq);
                    const double phase { v.stringOsc[ui].phase() };
                    strSampleRaw += v.stringOsc[ui].nextSample() + fastSineCycles(phase) * StringsFundamentalLift;
                }
                const double strSample { strSampleRaw * strEnv * strScale };
                strOutL[i] += strSample * panL;
                strOutR[i] += strSample * panR;
            } else {
                for (size_t ui { 0 }; ui < StringUnisonCount; ui++) {
                    v.stringDriftPhase[ui] += strDriftInc[ui];
                    if (v.stringDriftPhase[ui] >= 1.0) {
                        v.stringDriftPhase[ui] -= 1.0;
                    }
                }
            }

            if (renderAnyVoice) {
                const double driftOctaves { fastSineCycles(v.driftPhase) * (DriftCents * Inv1200) };
                const double voiceFreq { baseFreq * fastSmallExp2(driftOctaves) };
                const double shapeVal { 0.05 + 0.05 * fastSineCycles(v.pwmPhase) };

                double vocSample8 { 0.0 };
                if (renderVoice8) {
                    v.voiceOsc8.setShape(shapeVal);
                    v.voiceOsc8.setFrequency(voiceFreq);
                    vocSample8 = v.voiceOsc8.nextSample();
                }

                double vocSample4 { 0.0 };
                if (renderVoice4) {
                    v.voiceOsc4.setShape(shapeVal);
                    v.voiceOsc4.setFrequency(voiceFreq * 2.0);
                    vocSample4 = v.voiceOsc4.nextSample();
                }

                const double envGain { vocEnv * vocScale };
                const double maleSample { (vocSample8 * maleLevel + (upper ? 0.0 : vocSample4 * voice4Level)) * envGain };
                maleOutBufL[i] += maleSample * panL;
                maleOutBufR[i] += maleSample * panR;

                if (upper && renderVoice4) {
                    const double femaleSample { vocSample4 * voice4Level * envGain };
                    femOutBufL[i] += femaleSample * panL;
                    femOutBufR[i] += femaleSample * panR;
                }
            }
        }
    }

    for (uint32_t i { 0 }; i < context.frameCount; ++i) {
        // Apply Formant filter bank to Voice section (male and female registers
        // are filtered independently so they don't bleed into each other's
        // formant coloration)
        double maleOutL { 0.0 };
        double maleOutR { 0.0 };
        double femaleOutL { 0.0 };
        double femaleOutR { 0.0 };
        m_formantFiltersL.process(m_maleSumL[i], m_femaleSumL[i], maleOutL, femaleOutL);
        m_formantFiltersR.process(m_maleSumR[i], m_femaleSumR[i], maleOutR, femaleOutR);
        const double voiceFilteredSumL { maleOutL + femaleOutL };
        const double voiceFilteredSumR { maleOutR + femaleOutR };

        // The Strings section's tone network, which the voice section does not share.
        m_stringsHpfL.process(m_stringsSumL[i]);
        m_stringsHpfR.process(m_stringsSumR[i]);
        double stringsOutL { m_stringsLpfL.process(m_stringsHpfL.highPass()) };
        double stringsOutR { m_stringsLpfR.process(m_stringsHpfR.highPass()) };

        // Ensemble belongs to the Voice section alone. On the hardware the switch is wired to the
        // Human Voice and does not reach the Strings at all; the Strings shimmer either way, and
        // where that comes from is the unison above rather than anything here. V2 shared the one
        // Ensemble across both sections, which left its Strings perfectly still with the switch off.
        double voiceOutL { voiceFilteredSumL };
        double voiceOutR { voiceFilteredSumR };
        m_ensemble.process(voiceOutL, voiceOutR);

        // Combine Strings and Choir paths
        double sampleL { stringsOutL + voiceOutL };
        double sampleR { stringsOutR + voiceOutR };

        // Apply Vocoder if active. It runs after both sections have been through their own chorus,
        // so that what it takes as its carrier is the sound the sections actually make.
        if (m_vocoderEnabled && !sidechainBuffer.empty()) {
            const double modL { sidechainBuffer[i * 2] };
            const double modR { sidechainBuffer[i * 2 + 1] };
            double vocL { 0.0 };
            double vocR { 0.0 };
            m_vocoder.process(sampleL, sampleR, modL, modR, vocL, vocR);
            sampleL = vocL;
            sampleR = vocR;
        }

        // Built-in tone shaping
        sampleL = m_hpfL.process(m_lpfL.process(sampleL));
        sampleR = m_hpfR.process(m_lpfR.process(sampleR));

        // Safety soft-clip: transparent well below unity, only engages to
        // prevent hard clipping in extreme parameter combinations.
        sampleL = std::tanh(sampleL);
        sampleR = std::tanh(sampleR);

        // Apply global panner. The fader is applied by the engine, either side of the insert
        // rack depending on the device's fader position.
        m_panner.process(sampleL, sampleR);

        // Write to accumulation buffers
        context.buffer[i * 2] += sampleL;
        context.buffer[i * 2 + 1] += sampleR;
    }
}

bool StringVoiceV3Device::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

std::vector<size_t> StringVoiceV3Device::sidechainDependencies() const
{
    auto deps { Device::sidechainDependencies() };
    if (m_vocoderEnabled) {
        if (const auto idx { vocoderSidechainIndex() }) {
            deps.push_back(*idx);
        }
    }
    return deps;
}

void StringVoiceV3Device::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void StringVoiceV3Device::resetDeviceAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_sustainPedal = false;
    m_vocoder.reset();
    m_nextTriggerId = 1;
    for (int i { 0 }; i < MaxVoices; ++i) {
        auto & v { m_voices[static_cast<size_t>(i)] };
        v.reset();
        setStartPhases(v, i, MaxVoices);
    }
    m_formantFiltersL.reset();
    m_formantFiltersR.reset();
    m_ensemble.reset();
    m_lpfL.reset();
    m_lpfR.reset();
    m_hpfL.reset();
    m_hpfR.reset();
    m_stringsHpfL.reset();
    m_stringsHpfR.reset();
    m_stringsLpfL.reset();
    m_stringsLpfR.reset();
    m_lastAudioSampleRate = -1.0;
    m_lastStringsTone = -1.0f;
}

void StringVoiceV3Device::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeMidiMappingsToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void StringVoiceV3Device::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token { reader.readNext() };
            if (token == ProjectReader::TokenType::EndElement && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                break;
            }
            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == Constants::NahdXml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyMidiMappings()) {
                    deserializeMidiMappingsFromXml(reader);
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
    }
    emit dataChanged();
}

// Getters and Setters implementation
float StringVoiceV3Device::stringsBalance() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyStringsBalance(), m_stringsBalance);
}

void StringVoiceV3Device::setStringsBalance(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsBalance().toStdString(), val);
}

float StringVoiceV3Device::voiceBalance() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVoiceBalance(), m_voiceBalance);
}

void StringVoiceV3Device::setVoiceBalance(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceBalance().toStdString(), val);
}

bool StringVoiceV3Device::stringsUpper() const
{
    return m_stringsUpper;
}

void StringVoiceV3Device::setStringsUpper(bool val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsUpper().toStdString(), val ? 1.0f : 0.0f);
}

bool StringVoiceV3Device::stringsLower() const
{
    return m_stringsLower;
}

void StringVoiceV3Device::setStringsLower(bool val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsLower().toStdString(), val ? 1.0f : 0.0f);
}

float StringVoiceV3Device::stringsTone() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyStringsTone(), m_stringsTone);
}

float StringVoiceV3Device::velocitySensitivity() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
}

void StringVoiceV3Device::setVelocitySensitivity(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString(), val);
}

void StringVoiceV3Device::setStringsTone(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsTone().toStdString(), val);
}

float StringVoiceV3Device::stringsAttack() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyStringsAttack(), m_stringsAttack);
}

void StringVoiceV3Device::setStringsAttack(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsAttack().toStdString(), val);
}

float StringVoiceV3Device::stringsRelease() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyStringsRelease(), m_stringsRelease);
}

void StringVoiceV3Device::setStringsRelease(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsRelease().toStdString(), val);
}

float StringVoiceV3Device::voiceMale8() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVoiceMale8(), m_voiceMale8);
}

void StringVoiceV3Device::setVoiceMale8(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceMale8().toStdString(), val);
}

float StringVoiceV3Device::voiceMale4() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVoiceMale4(), m_voiceMale4);
}

void StringVoiceV3Device::setVoiceMale4(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceMale4().toStdString(), val);
}

float StringVoiceV3Device::voiceUpperMale8() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVoiceUpperMale8(), m_voiceUpperMale8);
}

void StringVoiceV3Device::setVoiceUpperMale8(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceUpperMale8().toStdString(), val);
}

float StringVoiceV3Device::voiceFemale4() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVoiceFemale4(), m_voiceFemale4);
}

void StringVoiceV3Device::setVoiceFemale4(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceFemale4().toStdString(), val);
}

float StringVoiceV3Device::voiceAttack() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVoiceAttack(), m_voiceAttack);
}

void StringVoiceV3Device::setVoiceAttack(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceAttack().toStdString(), val);
}

float StringVoiceV3Device::voiceRelease() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVoiceRelease(), m_voiceRelease);
}

void StringVoiceV3Device::setVoiceRelease(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceRelease().toStdString(), val);
}

float StringVoiceV3Device::vibratoRate() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVibratoRate(), m_vibratoRate);
}

void StringVoiceV3Device::setVibratoRate(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoRate().toStdString(), val);
}

float StringVoiceV3Device::vibratoDepth() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVibratoDepth(), m_vibratoDepth);
}

void StringVoiceV3Device::setVibratoDepth(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoDepth().toStdString(), val);
}

float StringVoiceV3Device::vibratoDelay() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVibratoDelay(), m_vibratoDelay);
}

void StringVoiceV3Device::setVibratoDelay(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoDelay().toStdString(), val);
}

bool StringVoiceV3Device::ensembleEnabled() const
{
    return m_ensembleEnabled;
}

void StringVoiceV3Device::setEnsembleEnabled(bool val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyEnsembleEnabled().toStdString(), val ? 1.0f : 0.0f);
}

int StringVoiceV3Device::ensembleMode() const
{
    return m_ensembleMode;
}

void StringVoiceV3Device::setEnsembleMode(int val)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyEnsembleMode().toStdString(), val);
}

bool StringVoiceV3Device::vocoderEnabled() const
{
    return m_vocoderEnabled;
}

void StringVoiceV3Device::setVocoderEnabled(bool val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVocoderEnabled().toStdString(), val ? 1.0f : 0.0f);
}

int StringVoiceV3Device::vocoderSidechain() const
{
    return m_vocoderSidechain;
}

void StringVoiceV3Device::setVocoderSidechain(int val)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVocoderSidechain().toStdString(), val);
}

std::optional<size_t> StringVoiceV3Device::vocoderSidechainIndex() const
{
    // Resolved once: this is read every block on the audio thread, and building the key would
    // allocate both a QString and a std::string each time.
    static const std::string key = Constants::NahdXml::xmlKeyVocoderSidechain().toStdString();
    if (const auto p = parameter(key); p) {
        const int val { p->get().xmlValue() };
        if (val >= 0) {
            return static_cast<size_t>(val);
        }
    }
    return std::nullopt;
}

float StringVoiceV3Device::lpfCutoff() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyLpfCutoff(), m_lpfCutoff);
}

void StringVoiceV3Device::setLpfCutoff(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), val);
}

float StringVoiceV3Device::hpfCutoff() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyHpfCutoff(), m_hpfCutoff);
}

void StringVoiceV3Device::setHpfCutoff(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), val);
}

float StringVoiceV3Device::panSpread() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyPanSpread(), m_panSpread);
}

void StringVoiceV3Device::setPanSpread(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPanSpread().toStdString(), val);
}

void StringVoiceV3Device::syncParameters()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    Device::syncParameters();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsBalance().toStdString()); p) {
        m_stringsBalance = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceBalance().toStdString()); p) {
        m_voiceBalance = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsUpper().toStdString()); p) {
        m_stringsUpper = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsLower().toStdString()); p) {
        m_stringsLower = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsTone().toStdString()); p) {
        m_stringsTone = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString()); p) {
        m_velocitySensitivity = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsAttack().toStdString()); p) {
        m_stringsAttack = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsRelease().toStdString()); p) {
        m_stringsRelease = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceMale8().toStdString()); p) {
        m_voiceMale8 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceMale4().toStdString()); p) {
        m_voiceMale4 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceUpperMale8().toStdString()); p) {
        m_voiceUpperMale8 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceFemale4().toStdString()); p) {
        m_voiceFemale4 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceAttack().toStdString()); p) {
        m_voiceAttack = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceRelease().toStdString()); p) {
        m_voiceRelease = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVibratoRate().toStdString()); p) {
        m_vibratoRate = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVibratoDepth().toStdString()); p) {
        m_vibratoDepth = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVibratoDelay().toStdString()); p) {
        m_vibratoDelay = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyEnsembleEnabled().toStdString()); p) {
        m_ensembleEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyEnsembleMode().toStdString()); p) {
        m_ensembleMode = p->get().xmlValue();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVocoderEnabled().toStdString()); p) {
        m_vocoderEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVocoderSidechain().toStdString()); p) {
        m_vocoderSidechain = p->get().xmlValue();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        m_lpfCutoff = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        m_hpfCutoff = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) {
        m_panSpread = p->get().value();
    }

    m_ensemble.setEnabled(m_ensembleEnabled);
    m_ensemble.setMode(m_ensembleMode);
}

std::vector<std::string> StringVoiceV3Device::modulationPriority() const
{
    namespace NahdXml = Constants::NahdXml;
    return {
        NahdXml::xmlKeyLpfCutoff().toStdString(),
        NahdXml::xmlKeyVoiceBalance().toStdString(),
        NahdXml::xmlKeyVibratoDepth().toStdString(),
        NahdXml::xmlKeyVibratoRate().toStdString(),
        NahdXml::xmlKeyStringsTone().toStdString(),
        NahdXml::xmlKeyStringsBalance().toStdString(),
        NahdXml::xmlKeyStringsAttack().toStdString(),
        NahdXml::xmlKeyStringsRelease().toStdString(),
        NahdXml::xmlKeyHpfCutoff().toStdString(),
        NahdXml::xmlKeyPanSpread().toStdString()
    };
}

std::vector<std::string> StringVoiceV3Device::modulationWheelTargets() const
{
    return { Constants::NahdXml::xmlKeyVibratoDepth().toStdString() };
}

} // namespace noteahead
