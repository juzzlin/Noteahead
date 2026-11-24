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

#include "speech_v2_device.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/xml/project_reader.hpp"
#include "../../../common/xml/project_writer.hpp"
#include "../../../domain/devices/device_parameter_display.hpp"
#include "../../../domain/dsp/speech/text_to_phonemes.hpp"
#include "../../../infra/midi/midi_cc_mapping.hpp"

#include <QVariant>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double OutputGain = 0.9;
constexpr double MinRate = 0.35;
constexpr double RateRange = 2.65;
constexpr double PortamentoSecondsRange = 0.3;
constexpr double MinGlideSeconds = 0.005;
constexpr double GlideSecondsRange = 0.075;
constexpr double MinFormantShift = 0.8;
constexpr double FormantShiftRange = 0.4;

struct VoiceTypeSettings
{
    double formantShift;
    double sourceRolloff;
    double openQuotient;
    double breathiness;
    double perturbation;
    double speedQuotient;
    double returnPhase;
    double subharmonic;
};

constexpr VoiceTypeSettings VoiceTypes[] {
    { 1.0, 0.0, 0.48, 0.04, 0.35, 2.5, 0.010, 0.0 },
    { 1.17, 0.0, 0.62, 0.10, 0.45, 2.5, 0.018, 0.0 },
    { 1.35, 0.0, 0.66, 0.12, 0.75, 2.5, 0.022, 0.0 },
    { 0.87, 0.0, 0.34, 0.02, 0.30, 2.8, 0.006, 0.0 },
    { 1.04, 0.0, 0.80, 0.30, 0.55, 2.0, 0.045, 0.0 },
    { 0.72, 0.0, 0.38, 0.05, 0.45, 2.6, 0.008, 0.0 },
    { 0.96, 0.0, 0.72, 0.20, 1.00, 2.2, 0.032, 0.0 },
    { 1.55, 0.0, 0.30, 0.02, 0.08, 3.2, 0.004, 0.0 },
    { 0.80, 0.0, 0.32, 0.06, 0.70, 4.5, 0.005, 0.60 }
};

constexpr double StressAccentSemitones = 3.0;
constexpr double ConsonantLevelRange = 2.0;
constexpr double OpenQuotientRange = 0.5;
constexpr double PerturbationRange = 1.0;
constexpr double MinVibratoRate = 2.0;
constexpr double VibratoRateRange = 5.0;
constexpr double VibratoDepthSemitones = 0.5;

double noteToFrequency(uint8_t note)
{
    return 440.0 * std::pow(2.0, (static_cast<double>(note) - 69.0) / 12.0);
}

} // namespace

SpeechV2Device::SpeechV2Device(std::string name)
  : m_name { std::move(name) }
  , m_phrase { defaultPhrase() }
{
    namespace Xml = Constants::NahdXml;
    addParameter(Parameter(Xml::xmlKeyRate().toStdString(), 0.25f, 0, 10000, 2500, 100));
    addParameter(Parameter(Xml::xmlKeyPortamento().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Xml::xmlKeyGlide().toStdString(), 0.35f, 0, 10000, 3500, 100));
    addParameter(Parameter(Xml::xmlKeyFormantShift().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Xml::xmlKeyBreathiness().toStdString(), 0.1f, 0, 10000, 1000, 100));
    addParameter(Parameter(Xml::xmlKeyConsonantLevel().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Xml::xmlKeySibilance().toStdString(), 0.31f, 0, 10000, 3100, 100));
    addParameter(Parameter(Xml::xmlKeyVoiceType().toStdString(), 0.0f, 0, 8, 0, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Xml::xmlKeyOpenQuotient().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Xml::xmlKeyVoicePerturbation().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Xml::xmlKeyVelocitySensitivity().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Xml::xmlKeyIntonation().toStdString(), 0.4f, 0, 10000, 4000, 100));
    addParameter(Parameter(Xml::xmlKeyVibratoRate().toStdString(), 0.3f, 0, 10000, 3000, 100));
    addParameter(Parameter(Xml::xmlKeyVibratoDepth().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Xml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100));
    addParameter(Parameter(Xml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Xml::xmlKeyTriggerMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Xml::xmlKeySyncMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Xml::xmlKeySyncLength().toStdString(), 16.0f, 1, 64, 16, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Xml::xmlKeySyncDivision().toStdString(), 2.0f, 1, 16, 2, 1, Parameter::Type::Discrete));

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);
    m_vibrato.setWaveform(Lfo::Waveform::Sine);

    compilePhrase();
    SpeechV2Device::syncParameters();

    DeviceParameterDisplay::filters(*this);
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyFormantShift(), QT_TRANSLATE_NOOP("ParameterDisplay", "Formant Shift"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyGlide(), QT_TRANSLATE_NOOP("ParameterDisplay", "Glide"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyConsonantLevel(), QT_TRANSLATE_NOOP("ParameterDisplay", "Consonants"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeySibilance(), QT_TRANSLATE_NOOP("ParameterDisplay", "Sibilance"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyBreathiness(), QT_TRANSLATE_NOOP("ParameterDisplay", "Breathiness"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyOpenQuotient(), QT_TRANSLATE_NOOP("ParameterDisplay", "Openness"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVoicePerturbation(), QT_TRANSLATE_NOOP("ParameterDisplay", "Jitter"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyIntonation(), QT_TRANSLATE_NOOP("ParameterDisplay", "Intonation"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVibratoRate(), QT_TRANSLATE_NOOP("ParameterDisplay", "Vibrato Rate"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVibratoDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "Vibrato Depth"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyRate(), QT_TRANSLATE_NOOP("ParameterDisplay", "Rate"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyPortamento(), QT_TRANSLATE_NOOP("ParameterDisplay", "Portamento"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity Sensitivity"));
}

SpeechV2Device::~SpeechV2Device() = default;

std::string SpeechV2Device::name() const
{
    return m_name;
}

std::string SpeechV2Device::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string SpeechV2Device::typeName() const
{
    return Constants::speechV2DeviceName().toStdString();
}

std::string SpeechV2Device::typeIdString()
{
    return "9a6e4c21-8b3f-4d72-b105-7e4a2f8c93d1";
}

std::string SpeechV2Device::typeId() const
{
    return typeIdString();
}

std::string SpeechV2Device::defaultPhrase()
{
    return "hello world";
}

std::vector<MidiCcController> SpeechV2Device::deviceMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SoundController5), "LPF Cutoff" },
        { static_cast<uint8_t>(Controller::GeneralPurpose6), "HPF Cutoff" },
        { static_cast<uint8_t>(Controller::SoundController2), "Formant Shift" }
    };
}

void SpeechV2Device::compilePhrase()
{
    m_sequencer.setPhonemes(textToPhonemes(m_phrase));
}

std::string SpeechV2Device::phrase() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_phrase;
}

void SpeechV2Device::setPhrase(const std::string & phrase)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_phrase == phrase) {
            return;
        }
        m_phrase = phrase;
        compilePhrase();
    }
    emit dataChanged();
}

std::string SpeechV2Device::phrasePhonemes() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return phonemeNames(m_sequencer.phonemes(), triggerModeEnum() == SpeechSequencer::TriggerMode::Line);
}

size_t SpeechV2Device::syllableCursor() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.syllableCursor();
}

size_t SpeechV2Device::lineCursor() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.lineCursor();
}

size_t SpeechV2Device::syllableCount() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.syllableCount();
}

size_t SpeechV2Device::lineCount() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.lineCount();
}

void SpeechV2Device::handleNoteOn(uint8_t note, uint8_t velocity)
{
    const auto mode = triggerModeEnum();
    const bool legato = !m_heldNotes.empty()
      && m_sequencer.phoneme() != nullptr
      && mode != SpeechSequencer::TriggerMode::Step;

    m_heldNotes.insert(note);
    m_note = note;
    m_velocity = std::clamp(static_cast<double>(velocity) / 127.0, 0.0, 1.0);
    if (!legato) {
        m_pitch = -1.0;
        m_notePitch.reset();
    }

    m_sequencer.setNoteBeats(mode == SpeechSequencer::TriggerMode::Line ? noteBeats() : std::nullopt);

    if (!legato) {
        m_sequencer.trigger();
        m_vibrato.setPhase(0.0);
        m_vibratoAge = 0.0;
        m_vibratoEnvelope = 0.0;
    }
}

void SpeechV2Device::handleNoteOff(uint8_t note)
{
    m_heldNotes.erase(note);
    if (m_heldNotes.empty()) {
        m_sequencer.release();
    }
}

void SpeechV2Device::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void SpeechV2Device::processDeviceMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void SpeechV2Device::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel)
{
    Q_UNUSED(channel);
    using namespace MidiCcMapping;

    bool changed = false;
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        switch (static_cast<Controller>(controller)) {
        case Controller::ChannelVolumeMSB:
            changed = updateVolumeParameter(faderPositionFromMidiCc(value), false);
            break;
        case Controller::PanMSB:
            changed = updatePanParameter(static_cast<float>(value) / 127.0f, false);
            break;
        case Controller::SoundController5:
            if (auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
                p->get().setAutomationValue(static_cast<float>(value) / 127.0f);
                changed = true;
            }
            break;
        case Controller::GeneralPurpose6:
            if (auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
                p->get().setAutomationValue(static_cast<float>(value) / 127.0f);
                changed = true;
            }
            break;
        case Controller::SoundController2:
            if (auto p = parameter(Constants::NahdXml::xmlKeyFormantShift().toStdString()); p) {
                p->get().setAutomationValue(static_cast<float>(value) / 127.0f);
                changed = true;
            }
            break;
        case Controller::ResetAllControllers:
            changed = clearAutomationInternal();
            break;
        default:
            break;
        }

        if (changed) {
            syncParameters();
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void SpeechV2Device::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_heldNotes.clear();
    m_sequencer.stop();
    if (triggerModeEnum() == SpeechSequencer::TriggerMode::Line) {
        m_sequencer.setCursor(0);
    }
}

SpeechSequencer::TriggerMode SpeechV2Device::triggerModeEnum() const
{
    return static_cast<SpeechSequencer::TriggerMode>(std::clamp(static_cast<int>(m_triggerMode), 0, 2));
}

bool SpeechV2Device::wantsNoteIndexSeek() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return triggerModeEnum() == SpeechSequencer::TriggerMode::Line;
}

void SpeechV2Device::seekToNoteIndex(size_t noteIndex)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (triggerModeEnum() == SpeechSequencer::TriggerMode::Line) {
        m_sequencer.setCursor(noteIndex);
    }
}

void SpeechV2Device::syncParameters()
{
    Device::syncParameters();

    namespace Xml = Constants::NahdXml;
    const auto read = [this](const QString & key, float fallback) {
        const auto p = parameter(key.toStdString());
        return p ? p->get().value() : fallback;
    };

    m_rate = read(Xml::xmlKeyRate(), m_rate);
    m_portamento = read(Xml::xmlKeyPortamento(), m_portamento);
    m_glide = read(Xml::xmlKeyGlide(), m_glide);
    m_formantShift = read(Xml::xmlKeyFormantShift(), m_formantShift);
    m_breathiness = read(Xml::xmlKeyBreathiness(), m_breathiness);
    m_consonantLevel = read(Xml::xmlKeyConsonantLevel(), m_consonantLevel);
    m_sibilance = read(Xml::xmlKeySibilance(), m_sibilance);
    m_voiceType = read(Xml::xmlKeyVoiceType(), m_voiceType);
    m_openQuotient = read(Xml::xmlKeyOpenQuotient(), m_openQuotient);
    m_voicePerturbation = read(Xml::xmlKeyVoicePerturbation(), m_voicePerturbation);
    m_velocitySensitivity = read(Xml::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
    m_intonation = read(Xml::xmlKeyIntonation(), m_intonation);
    m_vibratoRate = read(Xml::xmlKeyVibratoRate(), m_vibratoRate);
    m_vibratoDepth = read(Xml::xmlKeyVibratoDepth(), m_vibratoDepth);
    m_lpfCutoff = read(Xml::xmlKeyLpfCutoff(), m_lpfCutoff);
    m_hpfCutoff = read(Xml::xmlKeyHpfCutoff(), m_hpfCutoff);
    m_triggerMode = read(Xml::xmlKeyTriggerMode(), m_triggerMode);
    m_syncMode = read(Xml::xmlKeySyncMode(), m_syncMode);
    m_syncLength = read(Xml::xmlKeySyncLength(), m_syncLength);
    m_syncDivision = read(Xml::xmlKeySyncDivision(), m_syncDivision);

    m_sequencer.setTriggerMode(static_cast<SpeechSequencer::TriggerMode>(std::clamp(static_cast<int>(m_triggerMode), 0, 2)));
    m_sequencer.setSyncMode(static_cast<SpeechSequencer::SyncMode>(std::clamp(static_cast<int>(m_syncMode), 0, 2)));
    m_sequencer.setRate(MinRate + RateRange * static_cast<double>(m_rate));
    m_sequencer.setLengthBeats(static_cast<double>(m_syncLength) / 4.0);
    m_sequencer.setDivisionBeats(static_cast<double>(m_syncDivision) / 4.0);

    m_voice.setGlideTime(MinGlideSeconds + GlideSecondsRange * static_cast<double>(m_glide));

    const auto index = static_cast<size_t>(std::clamp(static_cast<int>(m_voiceType), 0, static_cast<int>(std::size(VoiceTypes)) - 1));
    const auto & voiceType = VoiceTypes[index];

    m_voice.setFormantShift(voiceType.formantShift * (MinFormantShift + FormantShiftRange * static_cast<double>(m_formantShift)));
    m_voice.setSourceRolloff(voiceType.sourceRolloff);
    m_voice.setOpenQuotient(voiceType.openQuotient + OpenQuotientRange * (static_cast<double>(m_openQuotient) - 0.5));
    const double returnPhase = std::clamp(voiceType.returnPhase * std::exp2((static_cast<double>(m_openQuotient) - 0.5) * 2.0), 0.003, 0.090);
    m_voice.setReturnPhase(returnPhase);
    m_voice.setVoicePerturbation(voiceType.perturbation * PerturbationRange * static_cast<double>(m_voicePerturbation) * 2.0);
    m_voice.setSpeedQuotient(voiceType.speedQuotient);
    m_voice.setSubharmonic(voiceType.subharmonic);
    m_voice.setBreathiness(static_cast<double>(m_breathiness) + voiceType.breathiness);
    m_voice.setConsonantLevel(static_cast<double>(m_consonantLevel) * ConsonantLevelRange);
    m_voice.setSibilance(static_cast<double>(m_sibilance));
    m_voice.setPhraseDynamics(static_cast<double>(m_intonation));
}

double SpeechV2Device::nextFlutter(double sampleRate)
{
    double sum = 0.0;
    for (size_t i = 0; i < m_flutterPhases.size(); i++) {
        m_flutterPhases[i] += FlutterRates[i] / sampleRate;
        m_flutterPhases[i] -= std::floor(m_flutterPhases[i]);
        sum += std::sin(2.0 * std::numbers::pi * m_flutterPhases[i]);
    }
    return 1.0 + FlutterDepth * sum / static_cast<double>(m_flutterPhases.size());
}

double SpeechV2Device::currentFrequency() const
{
    double frequency = m_notePitch.has_value() ? std::exp2(*m_notePitch) : noteToFrequency(m_note);
    const double declination = -IntonationRange * static_cast<double>(m_intonation) * m_sequencer.utteranceProgress();
    const double accent = m_sequencer.isStressed() ? StressAccentSemitones * static_cast<double>(m_intonation) : 0.0;
    frequency *= std::pow(2.0, (declination + accent) / 12.0);
    return frequency;
}

void SpeechV2Device::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    m_voice.setSampleRate(context.sampleRate);
    m_sequencer.setSampleRate(context.sampleRate);
    m_sequencer.setBpm(context.bpm);

    m_vibrato.setSampleRate(context.sampleRate);
    m_vibrato.setFrequency(MinVibratoRate + VibratoRateRange * static_cast<double>(m_vibratoRate));

    m_lpfL.setSampleRate(context.sampleRate);
    m_lpfR.setSampleRate(context.sampleRate);
    m_hpfL.setSampleRate(context.sampleRate);
    m_hpfR.setSampleRate(context.sampleRate);
    m_lpfL.setCutoff(static_cast<double>(m_lpfCutoff));
    m_lpfR.setCutoff(static_cast<double>(m_lpfCutoff));
    m_hpfL.setCutoff(static_cast<double>(m_hpfCutoff));
    m_hpfR.setCutoff(static_cast<double>(m_hpfCutoff));
    m_dcBlockerL.setSampleRate(context.sampleRate);
    m_dcBlockerR.setSampleRate(context.sampleRate);
    m_panner.setPan(static_cast<double>(panInternal()));

    m_fadeCoefficient = 1.0 - std::exp(-1.0 / (OutputFadeTime * context.sampleRate));
    m_pitchCoefficient = 1.0 - std::exp(-1.0 / (PitchGlideTime * context.sampleRate));
    const double portamentoSeconds = PortamentoSecondsRange * static_cast<double>(m_portamento);
    m_portamentoCoefficient = portamentoSeconds > 0.0 ? 1.0 - std::exp(-1.0 / (portamentoSeconds * context.sampleRate)) : 1.0;

    const double sensitivity = static_cast<double>(m_velocitySensitivity);
    const double gain = OutputGain * linearGainInternal() * (1.0 - sensitivity + sensitivity * m_velocity);

    const double dt = 1.0 / context.sampleRate;
    for (uint32_t i = 0; i < context.frameCount; i++) {
        const auto * spec = m_sequencer.phoneme();

        if (spec) {
            m_voice.setPhoneme(*spec, m_sequencer.nextPhoneme(), m_sequencer.phonemeSeconds());
            m_voice.setPhonemeProgress(m_sequencer.progress());
            m_voice.setUtteranceProgress(m_sequencer.utteranceProgress());

            const bool voicedSonorant = spec->type == PhonemeType::Vowel
              || spec->type == PhonemeType::Liquid
              || spec->type == PhonemeType::Nasal;
            if (voicedSonorant) {
                m_vibratoAge = std::min(VibratoOnsetSeconds, m_vibratoAge + dt);
            } else {
                m_vibratoAge = std::max(0.0, m_vibratoAge - dt * (VibratoOnsetSeconds / VibratoReleaseSeconds));
            }
            const double vt = m_vibratoAge / VibratoOnsetSeconds;
            m_vibratoEnvelope = vt * vt * (3.0 - 2.0 * vt);

            const double askedNote = std::log2(noteToFrequency(m_note));
            m_notePitch = m_notePitch.has_value() ? *m_notePitch + (askedNote - *m_notePitch) * m_portamentoCoefficient : askedNote;

            const double asked = std::log2(currentFrequency());
            m_pitch = m_pitch < 0.0 ? asked : m_pitch + (asked - m_pitch) * m_pitchCoefficient;

            const double vibrato = m_vibrato.nextSample() * static_cast<double>(m_vibratoDepth) * VibratoDepthSemitones * m_vibratoEnvelope;
            m_voice.setFrequency(std::exp2(m_pitch) * nextFlutter(context.sampleRate) * std::pow(2.0, vibrato / 12.0));
        } else {
            m_vibratoAge = std::max(0.0, m_vibratoAge - dt * (VibratoOnsetSeconds / VibratoReleaseSeconds));
            const double vt = m_vibratoAge / VibratoOnsetSeconds;
            m_vibratoEnvelope = vt * vt * (3.0 - 2.0 * vt);
        }

        m_fade += ((spec ? 1.0 : 0.0) - m_fade) * m_fadeCoefficient;

        double sample = (spec || m_fade > FadeFloor) ? m_voice.nextSample() : 0.0;
        sample *= m_fade * gain;

        if (spec) {
            m_sequencer.advance();
        }

        double outL = sample;
        double outR = sample;
        m_panner.processMono(sample, outL, outR);

        outL = m_lpfL.process(outL);
        outR = m_lpfR.process(outR);
        outL = m_hpfL.process(outL);
        outR = m_hpfR.process(outR);

        context.buffer[i * 2] += m_dcBlockerL.process(outL);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(outR);
    }
    publishLfoLed(0, m_sequencer.isActive() ? m_vibrato.shapeValue() * m_vibratoEnvelope : LfoLedDark);
}

void SpeechV2Device::advanceIdle(const AudioContext &)
{
    publishLfoLed(0, LfoLedDark);
}

bool SpeechV2Device::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.isActive() || m_fade > FadeFloor;
}

void SpeechV2Device::reset()
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        Device::reset();
        m_phrase = defaultPhrase();
        compilePhrase();
        syncParameters();
    }
    resetAudio();
    emit dataChanged();
}

void SpeechV2Device::resetDeviceAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_sequencer.reset();
    m_voice.reset();
    m_vibrato.reset();
    m_lpfL.reset();
    m_lpfR.reset();
    m_hpfL.reset();
    m_hpfR.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
    m_heldNotes.clear();
    m_fade = 0.0;
    m_pitch = -1.0;
    m_notePitch.reset();
    m_flutterPhases = {};
    m_vibratoAge = 0.0;
    m_vibratoEnvelope = 0.0;
}

float SpeechV2Device::rate() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyRate(), m_rate);
}

void SpeechV2Device::setRate(float rate)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyRate().toStdString(), rate);
}

float SpeechV2Device::portamento() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyPortamento(), m_portamento);
}

void SpeechV2Device::setPortamento(float portamento)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPortamento().toStdString(), portamento);
}

float SpeechV2Device::glide() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyGlide(), m_glide);
}

void SpeechV2Device::setGlide(float glide)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyGlide().toStdString(), glide);
}

float SpeechV2Device::formantShift() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyFormantShift(), m_formantShift);
}

void SpeechV2Device::setFormantShift(float formantShift)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyFormantShift().toStdString(), formantShift);
}

float SpeechV2Device::breathiness() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyBreathiness(), m_breathiness);
}

void SpeechV2Device::setBreathiness(float breathiness)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyBreathiness().toStdString(), breathiness);
}

float SpeechV2Device::sibilance() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeySibilance(), m_sibilance);
}

void SpeechV2Device::setSibilance(float sibilance)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeySibilance().toStdString(), sibilance);
}

float SpeechV2Device::velocitySensitivity() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
}

void SpeechV2Device::setVelocitySensitivity(float sensitivity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString(), sensitivity);
}

int SpeechV2Device::voiceType() const
{
    return static_cast<int>(m_voiceType);
}

void SpeechV2Device::setVoiceType(int voiceType)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVoiceType().toStdString(), voiceType);
}

float SpeechV2Device::openQuotient() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyOpenQuotient(), m_openQuotient);
}

void SpeechV2Device::setOpenQuotient(float openQuotient)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyOpenQuotient().toStdString(), openQuotient);
}

float SpeechV2Device::voicePerturbation() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVoicePerturbation(), m_voicePerturbation);
}

void SpeechV2Device::setVoicePerturbation(float voicePerturbation)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoicePerturbation().toStdString(), voicePerturbation);
}

float SpeechV2Device::consonantLevel() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyConsonantLevel(), m_consonantLevel);
}

void SpeechV2Device::setConsonantLevel(float consonantLevel)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyConsonantLevel().toStdString(), consonantLevel);
}

float SpeechV2Device::intonation() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyIntonation(), m_intonation);
}

void SpeechV2Device::setIntonation(float intonation)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyIntonation().toStdString(), intonation);
}

float SpeechV2Device::vibratoRate() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVibratoRate(), m_vibratoRate);
}

void SpeechV2Device::setVibratoRate(float vibratoRate)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoRate().toStdString(), vibratoRate);
}

float SpeechV2Device::vibratoDepth() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyVibratoDepth(), m_vibratoDepth);
}

void SpeechV2Device::setVibratoDepth(float vibratoDepth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoDepth().toStdString(), vibratoDepth);
}

float SpeechV2Device::lpfCutoff() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyLpfCutoff(), m_lpfCutoff);
}

void SpeechV2Device::setLpfCutoff(float lpfCutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), lpfCutoff);
}

float SpeechV2Device::hpfCutoff() const
{
    return shownParameterValue(Constants::NahdXml::xmlKeyHpfCutoff(), m_hpfCutoff);
}

void SpeechV2Device::setHpfCutoff(float hpfCutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), hpfCutoff);
}

int SpeechV2Device::triggerMode() const
{
    return static_cast<int>(m_triggerMode);
}

void SpeechV2Device::setTriggerMode(int triggerMode)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyTriggerMode().toStdString(), triggerMode);
}

int SpeechV2Device::syncMode() const
{
    return static_cast<int>(m_syncMode);
}

void SpeechV2Device::setSyncMode(int syncMode)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeySyncMode().toStdString(), syncMode);
}

int SpeechV2Device::syncLength() const
{
    return static_cast<int>(m_syncLength);
}

void SpeechV2Device::setSyncLength(int syncLength)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeySyncLength().toStdString(), syncLength);
}

int SpeechV2Device::syncDivision() const
{
    return static_cast<int>(m_syncDivision);
}

void SpeechV2Device::setSyncDivision(int syncDivision)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeySyncDivision().toStdString(), syncDivision);
}

void SpeechV2Device::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeMidiMappingsToXml(writer);

    writer.writeAttribute(Constants::NahdXml::xmlKeyPhrase(), QString::fromStdString(m_phrase));

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void SpeechV2Device::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (const auto stored = reader.attribute(Constants::NahdXml::xmlKeyPhrase()); !stored.isNull()) {
            m_phrase = stored.toString().toStdString();
        }

        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
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

        compilePhrase();
        syncParameters();
    }
    emit dataChanged();
}

std::vector<std::string> SpeechV2Device::modulationPriority() const
{
    namespace NahdXml = Constants::NahdXml;
    return {
        NahdXml::xmlKeyFormantShift().toStdString(),
        NahdXml::xmlKeyIntonation().toStdString(),
        NahdXml::xmlKeyRate().toStdString(),
        NahdXml::xmlKeyBreathiness().toStdString(),
        NahdXml::xmlKeyVibratoDepth().toStdString(),
        NahdXml::xmlKeyVibratoRate().toStdString(),
        NahdXml::xmlKeyLpfCutoff().toStdString(),
        NahdXml::xmlKeyHpfCutoff().toStdString()
    };
}

std::vector<std::string> SpeechV2Device::modulationWheelTargets() const
{
    return { Constants::NahdXml::xmlKeyVibratoDepth().toStdString() };
}

} // namespace noteahead
