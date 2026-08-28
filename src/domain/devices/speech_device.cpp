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

#include "speech_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../dsp/speech/text_to_phonemes.hpp"
#include <QVariant>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Headroom allowance. FormantVoice is already calibrated to the level the rest of the rack sits
//! at, so this only leaves room for the vibrato and the filters.
constexpr double OutputGain = 0.9;

//! What the Rate control spans, as a speaking-speed multiplier.
constexpr double MinRate = 0.35;
constexpr double RateRange = 2.65;

//! What the Glide control spans, in seconds of formant transition. Below the floor a transition is
//! too abrupt to be heard as one; above the ceiling the phonemes run into each other.
constexpr double MinGlideSeconds = 0.005;
constexpr double GlideSecondsRange = 0.075;

//! Vocal tract length, as a multiplier on every formant frequency, trimming whatever the voice type
//! has chosen. Half travel is no trim at all, so the type alone decides the voice unless the user
//! says otherwise.
constexpr double MinFormantShift = 0.8;
constexpr double FormantShiftRange = 0.4;

//! What a voice type sets: how much shorter the tract is than the male one the phoneme table is
//! written for, and where the glottal tilt sits.
//!
//! A woman's voice is not a man's moved up. The tract is about a sixth shorter, which lifts the
//! formants, and the source falls away faster, which is why it reads as softer rather than merely
//! higher. The pitch is the third part and that is the note being played, so a female voice sung an
//! octave down will still sound like one person doing an impression of another.
struct VoiceTypeSettings
{
    double formantShift;
    //! Corner of the extra rolloff on the source, or 0 for none.
    double sourceRolloff;
};

constexpr VoiceTypeSettings MaleVoice { 1.0, 0.0 };
constexpr VoiceTypeSettings FemaleVoice { 1.17, 3500.0 };

//! Semitones a stressed syllable is lifted by at full intonation.
//!
//! A pitch accent, which is the other half of what marks stress -- the first half being length. A
//! language that marks its stressed syllables only by making them longer sounds like one being read
//! rather than spoken.
constexpr double StressAccentSemitones = 3.0;

//! What the Consonant control spans, as a multiplier. Half travel is unity, which is where the
//! phoneme table's own levels are heard: those are set against measured speech, so the control is a
//! deviation from correct in either direction rather than a level that has to be dialled in. Being
//! able to go above unity matters -- consonants are the first thing a dense mix buries, and they
//! are what carries the words.
constexpr double ConsonantLevelRange = 2.0;

//! What the Vibrato Rate control spans, in Hz.
constexpr double MinVibratoRate = 2.0;
constexpr double VibratoRateRange = 5.0;

//! Vibrato depth at full, in semitones.
constexpr double VibratoDepthSemitones = 0.5;

double noteToFrequency(uint8_t note)
{
    return 440.0 * std::pow(2.0, (static_cast<double>(note) - 69.0) / 12.0);
}

} // namespace

SpeechDevice::SpeechDevice(std::string name)
  : m_name { std::move(name) }
  , m_phrase { defaultPhrase() }
{
    addParameter(Parameter(Constants::NahdXml::xmlKeyRate().toStdString(), 0.25f, 0, 10000, 2500, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyGlide().toStdString(), 0.35f, 0, 10000, 3500, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyFormantShift().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyBreathiness().toStdString(), 0.1f, 0, 10000, 1000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyConsonantLevel().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeySibilance().toStdString(), 0.4f, 0, 10000, 4000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeySibilance().toStdString(), 0.31f, 0, 10000, 3100, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyVoiceType().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyIntonation().toStdString(), 0.4f, 0, 10000, 4000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyVibratoRate().toStdString(), 0.3f, 0, 10000, 3000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyVibratoDepth().toStdString(), 0.15f, 0, 10000, 1500, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyTriggerMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Constants::NahdXml::xmlKeySyncMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete));
    // In sixteenths, so a whole number of them is a musically meaningful length either way.
    addParameter(Parameter(Constants::NahdXml::xmlKeySyncLength().toStdString(), 16.0f, 1, 64, 16, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Constants::NahdXml::xmlKeySyncDivision().toStdString(), 2.0f, 1, 16, 2, 1, Parameter::Type::Discrete));

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);
    m_vibrato.setWaveform(Lfo::Waveform::Sine);

    compilePhrase();
    SpeechDevice::syncParameters();
}

SpeechDevice::~SpeechDevice() = default;

std::string SpeechDevice::name() const
{
    return m_name;
}

std::string SpeechDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string SpeechDevice::typeName() const
{
    return Constants::speechDeviceName().toStdString();
}

std::string SpeechDevice::typeIdString()
{
    return "0f4c7a91-2d6e-4b18-9a3f-6c05e8d2b774";
}

std::string SpeechDevice::typeId() const
{
    return typeIdString();
}

std::string SpeechDevice::defaultPhrase()
{
    return "hello world";
}

std::vector<MidiCcController> SpeechDevice::availableMidiCcControllers() const
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

void SpeechDevice::compilePhrase()
{
    m_sequencer.setPhonemes(textToPhonemes(m_phrase));
}

std::string SpeechDevice::phrase() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_phrase;
}

void SpeechDevice::setPhrase(const std::string & phrase)
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

std::string SpeechDevice::phrasePhonemes() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return phonemeNames(m_sequencer.phonemes());
}

size_t SpeechDevice::syllableCursor() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.syllableCursor();
}

size_t SpeechDevice::syllableCount() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.syllableCount();
}

void SpeechDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    m_note = note;
    m_velocity = std::clamp(static_cast<double>(velocity) / 127.0, 0.0, 1.0);

    // A note arriving while one is already speaking re-pitches rather than restarts, in Phrase mode:
    // that is what makes a held phrase singable across a melody. In Step mode every note is meant to
    // fetch the next syllable, so it always triggers.
    const bool retrigger = !m_noteHeld
      || m_sequencer.phoneme() == nullptr
      || static_cast<SpeechSequencer::TriggerMode>(static_cast<int>(m_triggerMode)) == SpeechSequencer::TriggerMode::Step;

    m_noteHeld = true;

    if (retrigger) {
        m_sequencer.trigger();
        // Lfo::trigger() is declared but has no implementation, so the phase is set directly.
        m_vibrato.setPhase(0.0);
    }
}

void SpeechDevice::handleNoteOff(uint8_t note)
{
    if (note != m_note) {
        return;
    }
    m_noteHeld = false;
    m_sequencer.release();
}

void SpeechDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void SpeechDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void SpeechDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t channel)
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

void SpeechDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_noteHeld = false;
    m_sequencer.stop();
}

void SpeechDevice::syncParameters()
{
    // The base reads the channel strip -- fader, gain, pan, fader position, send tap -- back out of
    // the parameters, so skipping it loses all of them on load.
    Device::syncParameters();

    const auto value = [this](const QString & key, float fallback) {
        const auto p = parameter(key.toStdString());
        return p ? p->get().value() : fallback;
    };

    m_rate = value(Constants::NahdXml::xmlKeyRate(), m_rate);
    m_glide = value(Constants::NahdXml::xmlKeyGlide(), m_glide);
    m_formantShift = value(Constants::NahdXml::xmlKeyFormantShift(), m_formantShift);
    m_breathiness = value(Constants::NahdXml::xmlKeyBreathiness(), m_breathiness);
    m_consonantLevel = value(Constants::NahdXml::xmlKeyConsonantLevel(), m_consonantLevel);
    m_sibilance = value(Constants::NahdXml::xmlKeySibilance(), m_sibilance);
    m_voiceType = value(Constants::NahdXml::xmlKeyVoiceType(), m_voiceType);
    m_velocitySensitivity = value(Constants::NahdXml::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
    m_intonation = value(Constants::NahdXml::xmlKeyIntonation(), m_intonation);
    m_vibratoRate = value(Constants::NahdXml::xmlKeyVibratoRate(), m_vibratoRate);
    m_vibratoDepth = value(Constants::NahdXml::xmlKeyVibratoDepth(), m_vibratoDepth);
    m_lpfCutoff = value(Constants::NahdXml::xmlKeyLpfCutoff(), m_lpfCutoff);
    m_hpfCutoff = value(Constants::NahdXml::xmlKeyHpfCutoff(), m_hpfCutoff);
    m_triggerMode = value(Constants::NahdXml::xmlKeyTriggerMode(), m_triggerMode);
    m_syncMode = value(Constants::NahdXml::xmlKeySyncMode(), m_syncMode);
    m_syncLength = value(Constants::NahdXml::xmlKeySyncLength(), m_syncLength);
    m_syncDivision = value(Constants::NahdXml::xmlKeySyncDivision(), m_syncDivision);

    m_sequencer.setTriggerMode(static_cast<SpeechSequencer::TriggerMode>(std::clamp(static_cast<int>(m_triggerMode), 0, 1)));
    m_sequencer.setSyncMode(static_cast<SpeechSequencer::SyncMode>(std::clamp(static_cast<int>(m_syncMode), 0, 2)));
    m_sequencer.setRate(MinRate + RateRange * static_cast<double>(m_rate));
    // The lengths are given in sixteenths and the sequencer works in beats.
    m_sequencer.setLengthBeats(static_cast<double>(m_syncLength) / 4.0);
    m_sequencer.setDivisionBeats(static_cast<double>(m_syncDivision) / 4.0);

    m_voice.setGlideTime(MinGlideSeconds + GlideSecondsRange * static_cast<double>(m_glide));
    const auto & voiceType = static_cast<int>(m_voiceType) == 1 ? FemaleVoice : MaleVoice;
    m_voice.setFormantShift(voiceType.formantShift * (MinFormantShift + FormantShiftRange * static_cast<double>(m_formantShift)));
    m_voice.setSourceRolloff(voiceType.sourceRolloff);
    m_voice.setBreathiness(static_cast<double>(m_breathiness));
    m_voice.setConsonantLevel(static_cast<double>(m_consonantLevel) * ConsonantLevelRange);
    m_voice.setSibilance(static_cast<double>(m_sibilance));
}

double SpeechDevice::currentFrequency() const
{
    double frequency = noteToFrequency(m_note);

    // Speech falls in pitch across an utterance and falls further at its end. Without it every
    // phrase is a monotone, which is the single thing that most makes a synthesizer sound like a
    // machine reading rather than a voice talking.
    const double declination = -IntonationRange * static_cast<double>(m_intonation) * m_sequencer.utteranceProgress();
    const double accent = m_sequencer.isStressed() ? StressAccentSemitones * static_cast<double>(m_intonation) : 0.0;
    frequency *= std::pow(2.0, (declination + accent) / 12.0);

    return frequency;
}

void SpeechDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    m_voice.setSampleRate(context.sampleRate);
    m_sequencer.setSampleRate(context.sampleRate);
    // Read per block rather than cached from setBpm(), so an offline render follows the same tempo
    // as playback does.
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

    // At zero the velocity is ignored and every note speaks at full level, at one it scales the
    // level outright. Half way is the default because a spoken phrase carries a lot of its meaning
    // in the consonants, and those are already the quietest part of it: scaled hard by velocity they
    // are the first thing to disappear.
    const double sensitivity = static_cast<double>(m_velocitySensitivity);
    const double gain = OutputGain * linearGainInternal() * (1.0 - sensitivity + sensitivity * m_velocity);

    for (uint32_t i = 0; i < context.frameCount; i++) {
        const auto * spec = m_sequencer.phoneme();

        double sample = 0.0;
        if (spec) {
            m_voice.setPhoneme(*spec, m_sequencer.nextPhoneme(), m_sequencer.phonemeSeconds());
            m_voice.setPhonemeProgress(m_sequencer.progress());

            const double vibrato = m_vibrato.nextSample() * static_cast<double>(m_vibratoDepth) * VibratoDepthSemitones;
            m_voice.setFrequency(currentFrequency() * std::pow(2.0, vibrato / 12.0));

            sample = m_voice.nextSample();
            m_sequencer.advance();
        }

        // The fade covers the end of an utterance and every cut-off, so the phrase running out is
        // not a step to zero.
        m_fade += ((spec ? 1.0 : 0.0) - m_fade) * m_fadeCoefficient;
        sample *= m_fade * gain;

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
}

bool SpeechDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.isActive() || m_fade > 0.0001;
}

void SpeechDevice::reset()
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

void SpeechDevice::resetAudio()
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
    m_noteHeld = false;
    m_fade = 0.0;
}

float SpeechDevice::rate() const
{
    return m_rate;
}

void SpeechDevice::setRate(float rate)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyRate().toStdString(), rate);
}

float SpeechDevice::glide() const
{
    return m_glide;
}

void SpeechDevice::setGlide(float glide)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyGlide().toStdString(), glide);
}

float SpeechDevice::formantShift() const
{
    return m_formantShift;
}

void SpeechDevice::setFormantShift(float formantShift)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyFormantShift().toStdString(), formantShift);
}

float SpeechDevice::breathiness() const
{
    return m_breathiness;
}

void SpeechDevice::setBreathiness(float breathiness)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyBreathiness().toStdString(), breathiness);
}

float SpeechDevice::sibilance() const
{
    return m_sibilance;
}

void SpeechDevice::setSibilance(float sibilance)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeySibilance().toStdString(), sibilance);
}

float SpeechDevice::velocitySensitivity() const
{
    return m_velocitySensitivity;
}

void SpeechDevice::setVelocitySensitivity(float sensitivity)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString(), sensitivity);
}

int SpeechDevice::voiceType() const
{
    return static_cast<int>(m_voiceType);
}

void SpeechDevice::setVoiceType(int voiceType)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVoiceType().toStdString(), voiceType);
}

float SpeechDevice::consonantLevel() const
{
    return m_consonantLevel;
}

void SpeechDevice::setConsonantLevel(float consonantLevel)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyConsonantLevel().toStdString(), consonantLevel);
}

float SpeechDevice::intonation() const
{
    return m_intonation;
}

void SpeechDevice::setIntonation(float intonation)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyIntonation().toStdString(), intonation);
}

float SpeechDevice::vibratoRate() const
{
    return m_vibratoRate;
}

void SpeechDevice::setVibratoRate(float vibratoRate)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoRate().toStdString(), vibratoRate);
}

float SpeechDevice::vibratoDepth() const
{
    return m_vibratoDepth;
}

void SpeechDevice::setVibratoDepth(float vibratoDepth)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoDepth().toStdString(), vibratoDepth);
}

float SpeechDevice::lpfCutoff() const
{
    return m_lpfCutoff;
}

void SpeechDevice::setLpfCutoff(float lpfCutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), lpfCutoff);
}

float SpeechDevice::hpfCutoff() const
{
    return m_hpfCutoff;
}

void SpeechDevice::setHpfCutoff(float hpfCutoff)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), hpfCutoff);
}

int SpeechDevice::triggerMode() const
{
    return static_cast<int>(m_triggerMode);
}

void SpeechDevice::setTriggerMode(int triggerMode)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyTriggerMode().toStdString(), triggerMode);
}

int SpeechDevice::syncMode() const
{
    return static_cast<int>(m_syncMode);
}

void SpeechDevice::setSyncMode(int syncMode)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeySyncMode().toStdString(), syncMode);
}

int SpeechDevice::syncLength() const
{
    return static_cast<int>(m_syncLength);
}

void SpeechDevice::setSyncLength(int syncLength)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeySyncLength().toStdString(), syncLength);
}

int SpeechDevice::syncDivision() const
{
    return static_cast<int>(m_syncDivision);
}

void SpeechDevice::setSyncDivision(int syncDivision)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeySyncDivision().toStdString(), syncDivision);
}

void SpeechDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    // The phrase is the one setting that is not a number, so it goes on the element as an attribute
    // rather than through the parameter machinery.
    writer.writeAttribute(Constants::NahdXml::xmlKeyPhrase(), QString::fromStdString(m_phrase));

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void SpeechDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        // Read before the attributes are consumed, and only overwritten when the project actually
        // carries one, so a device written before the phrase existed keeps the default.
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

} // namespace noteahead
