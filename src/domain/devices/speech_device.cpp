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
#include <numbers>

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

//! What a voice type sets.
//!
//! A voice is not a pitch. Four things separate two speakers saying the same word at the same note:
//! the length of the tract, which moves the whole vowel space; the share of the period the folds
//! stay open, which decides whether the voice is pressed or breathy and is most of what a listener
//! hears as its character; the breath mixed in behind it; and how steady it all is. The pitch is a
//! fifth, and that one is the note being played, which is why a type has to carry the other four or
//! it is only a transposition.
//!
//! Two numbers used to carry it -- tract length and a low pass -- and those two fight: the shorter
//! tract brightens and the low pass darkens. Measured on a held vowel they nearly cancelled, and the
//! female voice came out with a *lower* spectral centroid than the male one where a real one is
//! higher. That is why she did not read as female, and it is the reason the rolloff is gone from
//! every type here: the softness belongs to the source, where the open quotient now puts it.
struct VoiceTypeSettings
{
    //! Tract length, as a multiplier on every formant. Above 1 is a shorter tract, so a smaller
    //! speaker.
    double formantShift;
    //! Corner of the extra rolloff on the source, or 0 for none. Only the legacy types use it.
    double sourceRolloff;
    //! Share of the period the folds are open. Low is pressed, high is breathy.
    double openQuotient;
    //! Aspiration behind the voice, before the user's own Breathiness is added.
    double breathiness;
    //! How unsteady the folds are, as a share of full travel.
    double perturbation;
};

//! What Male and Female were before the source could tell them apart. Reached only by a project
//! saved against them, which has to keep sounding as it did -- see VoiceEngine.
constexpr VoiceTypeSettings LegacyVoices[] {
    { 1.0, 0.0, 0.5, 0.0, 0.0 },
    { 1.17, 3500.0, 0.5, 0.0, 0.0 }
};

//! The voices, in the order the dialog lists them.
//!
//! Male and Female keep positions 0 and 1 because the ordinal is what a project stores: moving them
//! would silently turn every saved Speech device into a different speaker. New ones are appended.
constexpr VoiceTypeSettings VoiceTypes[] {
    //! Modal: the folds shut firmly and the voice is even. The tract is the one the phoneme table
    //! was measured against, so this is the reference the other four are stated against.
    { 1.0, 0.0, 0.48, 0.04, 0.35 },
    //! A sixth shorter, and breathier -- the open quotient does the softening the low pass used to
    //! attempt, and does it without darkening the formants that make her sound like a woman.
    { 1.17, 0.0, 0.62, 0.10, 0.45 },
    //! A child: shorter again, breathier again, and markedly less steady. Control of the folds is
    //! something that is learned, and its absence is most of what makes a young voice recognisable.
    { 1.35, 0.0, 0.66, 0.12, 0.75 },
    //! Deep: a long tract and a hard, short pulse. Pressed rather than merely low, because a voice
    //! sung an octave down is still the same voice unless the source changes with it.
    { 0.87, 0.0, 0.34, 0.02, 0.30 },
    //! Breathy: the folds never quite meet, so most of the energy is in the first harmonic and the
    //! rest is air. The tract is the male one, which is what keeps it a manner of speaking rather
    //! than a fifth speaker.
    { 1.04, 0.0, 0.80, 0.30, 0.55 }
};

//! Which engine a device is running.
//!
//! The source was a sawtooth through a tilt until the Rosenberg pulse replaced it, and a project
//! saved before that has to go on sounding the way it did when it was saved. So the engine is a
//! setting rather than a version of the program: a device constructed now gets Modern, and
//! deserializeFromXml() forces Legacy before it reads, so a file that carries no such parameter --
//! which is every file written until now -- keeps the voice it was written with.
enum class VoiceEngine
{
    Legacy = 0,
    Modern = 1
};

bool isLegacyEngine(float voiceEngine)
{
    return static_cast<int>(voiceEngine) != static_cast<int>(VoiceEngine::Modern);
}

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

//! What the Openness control spans either side of whatever the voice type chose, so that half
//! travel is the type's own value and the knob is a deviation from a voice rather than a voice.
constexpr double OpenQuotientRange = 0.5;

//! What the Jitter control spans, as a multiplier on the type's own unsteadiness. Half travel is
//! the type unaltered, which is what makes the default the voice the type describes.
constexpr double PerturbationRange = 1.0;

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
    addParameter(Parameter(Constants::NahdXml::xmlKeySibilance().toStdString(), 0.31f, 0, 10000, 3100, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyVoiceType().toStdString(), 0.0f, 0, 4, 0, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Constants::NahdXml::xmlKeyOpenQuotient().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyVoicePerturbation().toStdString(), 0.5f, 0, 10000, 5000, 100));
    // Modern for a device made now; deserializeFromXml() forces Legacy before it reads, so a project
    // that predates this parameter keeps the voice it was saved with.
    addParameter(Parameter(Constants::NahdXml::xmlKeyVoiceEngine().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Discrete));
    addParameter(Parameter(Constants::NahdXml::xmlKeyVelocitySensitivity().toStdString(), 0.5f, 0, 10000, 5000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyIntonation().toStdString(), 0.4f, 0, 10000, 4000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyVibratoRate().toStdString(), 0.3f, 0, 10000, 3000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyVibratoDepth().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100));
    addParameter(Parameter(Constants::NahdXml::xmlKeyTriggerMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete));
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

std::vector<MidiCcController> SpeechDevice::deviceMidiCcControllers() const
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
    return phonemeNames(m_sequencer.phonemes(), triggerModeEnum() == SpeechSequencer::TriggerMode::Line);
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
    m_pitch = -1.0;

    // A note arriving while one is already speaking re-pitches rather than restarts, in Phrase mode:
    // that is what makes a held phrase singable across a melody. In Step and Line mode every note is
    // meant to fetch the next syllable or line, so it always triggers.
    const auto mode = triggerModeEnum();
    const bool retrigger = !m_noteHeld
      || m_sequencer.phoneme() == nullptr
      || mode == SpeechSequencer::TriggerMode::Step
      || mode == SpeechSequencer::TriggerMode::Line;

    // Only Line mode has anything to fit inside a note. Handed over before the trigger, because the
    // durations for the whole line are worked out there and cannot be revised afterwards.
    m_sequencer.setNoteBeats(mode == SpeechSequencer::TriggerMode::Line ? noteBeats() : std::nullopt);

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

void SpeechDevice::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel)
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

    // Line mode rewinds with it, so that pressing play speaks the lyric from its first line rather
    // than from wherever the last run was interrupted. PlayerService sends all-notes-off before it
    // starts playing, which is what makes this the clean slate the player's seed then lands on.
    //
    // Gated on the mode because Step mode's cursor has always survived a stop, and a song written
    // against that must keep doing what it did.
    if (triggerModeEnum() == SpeechSequencer::TriggerMode::Line) {
        m_sequencer.setCursor(0);
    }
}

SpeechSequencer::TriggerMode SpeechDevice::triggerModeEnum() const
{
    return static_cast<SpeechSequencer::TriggerMode>(std::clamp(static_cast<int>(m_triggerMode), 0, 2));
}

bool SpeechDevice::wantsNoteIndexSeek() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return triggerModeEnum() == SpeechSequencer::TriggerMode::Line;
}

void SpeechDevice::seekToNoteIndex(size_t noteIndex)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (triggerModeEnum() == SpeechSequencer::TriggerMode::Line) {
        m_sequencer.setCursor(noteIndex);
    }
}

size_t SpeechDevice::lineCount() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.lineCount();
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
    m_openQuotient = value(Constants::NahdXml::xmlKeyOpenQuotient(), m_openQuotient);
    m_voicePerturbation = value(Constants::NahdXml::xmlKeyVoicePerturbation(), m_voicePerturbation);
    m_voiceEngine = value(Constants::NahdXml::xmlKeyVoiceEngine(), m_voiceEngine);
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

    m_sequencer.setTriggerMode(static_cast<SpeechSequencer::TriggerMode>(std::clamp(static_cast<int>(m_triggerMode), 0, 2)));
    m_sequencer.setSyncMode(static_cast<SpeechSequencer::SyncMode>(std::clamp(static_cast<int>(m_syncMode), 0, 2)));
    m_sequencer.setRate(MinRate + RateRange * static_cast<double>(m_rate));
    // The lengths are given in sixteenths and the sequencer works in beats.
    m_sequencer.setLengthBeats(static_cast<double>(m_syncLength) / 4.0);
    m_sequencer.setDivisionBeats(static_cast<double>(m_syncDivision) / 4.0);

    m_voice.setGlideTime(MinGlideSeconds + GlideSecondsRange * static_cast<double>(m_glide));

    const bool legacy = isLegacyEngine(m_voiceEngine);
    // A legacy device only ever stored 0 or 1, so the table it indexes is the two-entry one. Should
    // a user pick one of the new types on such a device they have asked for a new voice, and the
    // modern table is what they get -- what must not change is a project nobody has touched.
    const auto index = static_cast<size_t>(std::clamp(static_cast<int>(m_voiceType), 0, static_cast<int>(std::size(VoiceTypes)) - 1));
    const auto & voiceType = (legacy && index < std::size(LegacyVoices)) ? LegacyVoices[index] : VoiceTypes[index];

    m_voice.setGlottalModel(legacy ? GlottalSource::Model::Saw : GlottalSource::Model::Rosenberg);
    m_voice.setFormantShift(voiceType.formantShift * (MinFormantShift + FormantShiftRange * static_cast<double>(m_formantShift)));
    m_voice.setSourceRolloff(voiceType.sourceRolloff);
    // The controls are offsets from what the voice type chose rather than absolutes, so that picking
    // a voice moves the sound and the knobs still mean what they meant. Half travel on Openness is
    // the type's own value, which is what makes a type audible without the user dialling anything.
    m_voice.setOpenQuotient(voiceType.openQuotient + OpenQuotientRange * (static_cast<double>(m_openQuotient) - 0.5));
    m_voice.setVoicePerturbation(legacy ? 0.0 : voiceType.perturbation * PerturbationRange * static_cast<double>(m_voicePerturbation) * 2.0);
    m_voice.setBreathiness(static_cast<double>(m_breathiness) + (legacy ? 0.0 : voiceType.breathiness));
    m_voice.setConsonantLevel(static_cast<double>(m_consonantLevel) * ConsonantLevelRange);
    m_voice.setSibilance(static_cast<double>(m_sibilance));
}

double SpeechDevice::nextFlutter(double sampleRate)
{
    double sum = 0.0;
    for (size_t i = 0; i < m_flutterPhases.size(); i++) {
        m_flutterPhases[i] += FlutterRates[i] / sampleRate;
        // Wrapped rather than left to run, or the phase loses its precision over a long render and
        // the flutter quietly turns into a frequency offset.
        m_flutterPhases[i] -= std::floor(m_flutterPhases[i]);
        sum += std::sin(2.0 * std::numbers::pi * m_flutterPhases[i]);
    }
    return 1.0 + FlutterDepth * sum / static_cast<double>(m_flutterPhases.size());
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
    m_pitchCoefficient = 1.0 - std::exp(-1.0 / (PitchGlideTime * context.sampleRate));

    // At zero the velocity is ignored and every note speaks at full level, at one it scales the
    // level outright. Half way is the default because a spoken phrase carries a lot of its meaning
    // in the consonants, and those are already the quietest part of it: scaled hard by velocity they
    // are the first thing to disappear.
    const double sensitivity = static_cast<double>(m_velocitySensitivity);
    const double gain = OutputGain * linearGainInternal() * (1.0 - sensitivity + sensitivity * m_velocity);

    for (uint32_t i = 0; i < context.frameCount; i++) {
        const auto * spec = m_sequencer.phoneme();

        if (spec) {
            m_voice.setPhoneme(*spec, m_sequencer.nextPhoneme(), m_sequencer.phonemeSeconds());
            m_voice.setPhonemeProgress(m_sequencer.progress());

            // The contour is smoothed, the modulation is not: smoothing the flutter and the vibrato
            // as well would be filtering the very thing they are there to add.
            const double asked = std::log2(currentFrequency());
            m_pitch = m_pitch < 0.0 ? asked : m_pitch + (asked - m_pitch) * m_pitchCoefficient;

            const double vibrato = m_vibrato.nextSample() * static_cast<double>(m_vibratoDepth) * VibratoDepthSemitones;
            m_voice.setFrequency(std::exp2(m_pitch) * nextFlutter(context.sampleRate) * std::pow(2.0, vibrato / 12.0));
        }

        // The fade covers the end of an utterance and every cut-off, so the phrase running out is
        // not a step to zero.
        m_fade += ((spec ? 1.0 : 0.0) - m_fade) * m_fadeCoefficient;

        // The voice goes on being rendered while the fade is still open, and that is the whole of
        // what makes the fade work. Zeroing the sample the moment the sequencer went quiet and then
        // multiplying it by the fade faded silence: the output stepped from wherever the waveform
        // happened to be straight to nothing, measured at a fifth of full scale in a single sample
        // at the end of a syllable, and a step is heard as a click however short the fade after it.
        // The voice still holds the phoneme it was last given, so what the fade closes over now is
        // that phoneme ringing down -- which is what a released note sounds like.
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
}

bool SpeechDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_sequencer.isActive() || m_fade > FadeFloor;
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
    m_pitch = -1.0;
    m_flutterPhases = {};
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

float SpeechDevice::openQuotient() const
{
    return m_openQuotient;
}

void SpeechDevice::setOpenQuotient(float openQuotient)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyOpenQuotient().toStdString(), openQuotient);
}

float SpeechDevice::voicePerturbation() const
{
    return m_voicePerturbation;
}

void SpeechDevice::setVoicePerturbation(float voicePerturbation)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoicePerturbation().toStdString(), voicePerturbation);
}

int SpeechDevice::voiceEngine() const
{
    return static_cast<int>(m_voiceEngine);
}

void SpeechDevice::setVoiceEngine(int voiceEngine)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVoiceEngine().toStdString(), voiceEngine);
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

        // Forced before the parameters are read rather than defaulted in the constructor, because
        // an absent parameter keeps whatever the container already holds. Every project written
        // before the Rosenberg source existed carries no voiceEngine, so this is what makes those
        // load as the voice they were saved with; one written since carries it and overwrites this.
        //
        // Written straight into the parameter rather than through setDiscreteParameterValue(), which
        // emits dataChanged(). That signal must not be emitted from here: this whole block holds the
        // device mutex, which is exactly why the emit at the end of this function sits outside it.
        // Under the lock it reaches MidiService, whose handler goes out to the MIDI worker thread,
        // and the project stopped loading at all. syncParameters() at the end of this function is
        // what carries the value into the member, so nothing else is needed here.
        if (const auto engine = parameter(Constants::NahdXml::xmlKeyVoiceEngine().toStdString()); engine) {
            engine->get().setFromXml(static_cast<int>(VoiceEngine::Legacy));
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
