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

#ifndef SPEECH_DEVICE_HPP
#define SPEECH_DEVICE_HPP

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/speech/formant_voice.hpp"
#include "../dsp/speech/speech_sequencer.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <array>
#include <string>

namespace noteahead {

//! Speaks a written phrase, pitched to the notes played.
//!
//! Monophonic, and deliberately so: a second speaking voice costs a whole formant bank and two
//! people talking over each other are less intelligible than one, not more. Notes overlapping is
//! read as a re-pitch of what is already being said rather than as a second utterance.
//!
//! The phrase is a string, so unlike every other setting on the device it is not a Parameter. It is
//! written as an attribute on the device element, the way SamplerDevice writes a sample path.
class SpeechDevice : public Device
{
public:
    explicit SpeechDevice(std::string name);
    ~SpeechDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;

    static std::string typeIdString();

    //! What the device says when it has never been given anything to say. A device that is silent
    //! until configured reads as broken.
    static std::string defaultPhrase();

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    bool wantsNoteIndexSeek() const override;
    void seekToNoteIndex(size_t noteIndex) override;

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void reset() override;
    void resetAudio() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    //! The phrase as the user typed it, escapes and all.
    std::string phrase() const;
    void setPhrase(const std::string & phrase);

    //! What the letter-to-sound rules made of the phrase, for showing the user. Space separated,
    //! double spaced between words.
    std::string phrasePhonemes() const;

    float rate() const;
    void setRate(float rate);
    float glide() const;
    void setGlide(float glide);
    float formantShift() const;
    void setFormantShift(float formantShift);
    float breathiness() const;
    void setBreathiness(float breathiness);
    float consonantLevel() const;
    void setConsonantLevel(float consonantLevel);
    float sibilance() const;
    void setSibilance(float sibilance);
    int voiceType() const;
    void setVoiceType(int voiceType);
    float openQuotient() const;
    void setOpenQuotient(float openQuotient);
    float voicePerturbation() const;
    void setVoicePerturbation(float voicePerturbation);
    //! Which source the device runs: 0 for the sawtooth every project until now was written
    //! against, 1 for the glottal pulse. A setting rather than a program version, so that opening an
    //! old song does not change how it sounds and upgrading one stays the user's own decision.
    int voiceEngine() const;
    void setVoiceEngine(int voiceEngine);
    float velocitySensitivity() const;
    void setVelocitySensitivity(float sensitivity);
    float intonation() const;
    void setIntonation(float intonation);
    float vibratoRate() const;
    void setVibratoRate(float vibratoRate);
    float vibratoDepth() const;
    void setVibratoDepth(float vibratoDepth);
    float lpfCutoff() const;
    void setLpfCutoff(float lpfCutoff);
    float hpfCutoff() const;
    void setHpfCutoff(float hpfCutoff);
    int triggerMode() const;
    void setTriggerMode(int triggerMode);
    int syncMode() const;
    void setSyncMode(int syncMode);
    int syncLength() const;
    void setSyncLength(int syncLength);
    int syncDivision() const;
    void setSyncDivision(int syncDivision);

    //! Which syllable the next note would speak in Step mode.
    size_t syllableCursor() const;
    size_t syllableCount() const;

    //! How many lines the phrase divides into. One unless it holds a full stop.
    size_t lineCount() const;

protected:
    void syncParameters() override;

private:
    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    void compilePhrase();
    //! The trigger mode as the sequencer's enum, clamped to what this build knows. A project
    //! written by a later version can carry a value this one has no mode for.
    SpeechSequencer::TriggerMode triggerModeEnum() const;
    //! Fundamental the contour asks for: the note, the declination and the stress accent. What the
    //! voice is actually given is this smoothed and fluttered.
    double currentFrequency() const;
    //! The flutter's multiplier on the fundamental, advancing its phases by one frame.
    double nextFlutter(double sampleRate);

    //! Semitones the contour falls across an utterance at full intonation.
    //!
    //! Kept smaller than the accent a stressed syllable takes, or the two fight: at four semitones
    //! the fall had eaten the whole accent by the middle of a phrase, and the syllables the accent
    //! was supposed to mark came out no higher than the ones around them.
    static constexpr double IntonationRange = 2.0;

    //! Seconds the output takes to fade when an utterance ends or is cut off. Without it the end of
    //! a phrase is a step to zero, which clicks.
    static constexpr double OutputFadeTime = 0.006;

    //! How far the fade has to have closed before the voice stops being rendered at all.
    //!
    //! The same figure decides when the device reports itself silent, and it has to: rendering a
    //! tail the mixer has already stopped asking for would lose the end of it, and reporting silence
    //! while a tail is still being rendered would cut it off -- which is the step the fade exists to
    //! prevent.
    static constexpr double FadeFloor = 0.0001;

    //! Seconds the fundamental takes to cover most of the way to the pitch the contour asks for.
    //!
    //! The contour is a staircase: whether a phoneme is stressed is a flag on the phoneme, so the
    //! accent switches on and off at phoneme boundaries with nothing in between. Measured on "the
    //! quick brown fox jumps over the lazy dog" that is a step of 1.2 semitones, down and up again,
    //! four times in the last second of the phrase -- and a tone jumped up and down several times a
    //! second is a yodel, not an accent. A real pitch accent is a rise and a fall spanning a couple
    //! of hundred milliseconds around the stressed vowel, so smoothing the staircase is most of the
    //! way to having one: at this time constant a step is 95% covered in about 135 ms, and a short
    //! unstressed syllable between two accents never reaches the bottom, which is also true of
    //! speech.
    //!
    //! Not applied to a note change. A new note is the melody the user wrote and has to land on
    //! pitch; it is the accent inside an utterance that has to move rather than jump.
    static constexpr double PitchGlideTime = 0.045;

    //! Flutter: a slow, irregular wander of the fundamental that every real voice has and no
    //! oscillator does.
    //!
    //! Three sinusoids at rates with no common period, which is Klatt's trick and the reason it
    //! works: any single rate is heard as vibrato, and vibrato is a thing a singer does on purpose.
    //! Summed, these never repeat, so what is left is an instability rather than a modulation.
    //!
    //! Always on, and not a control. It is a property of a voice in the way that the glottal tilt
    //! is, rather than something a user sets -- and at this depth, under a tenth of a semitone, the
    //! only thing there would be to hear is its absence.
    static constexpr double FlutterRates[] { 12.7, 7.1, 4.7 };
    static constexpr double FlutterDepth = 0.004;

    std::string m_name;
    std::string m_phrase;

    SpeechSequencer m_sequencer;
    FormantVoice m_voice;

    Lfo m_vibrato;
    CascadedSvf m_lpfL;
    CascadedSvf m_lpfR;
    CascadedSvf m_hpfL;
    CascadedSvf m_hpfR;
    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;
    TrueStereoPanner m_panner;

    uint8_t m_note { 0 };
    bool m_noteHeld { false };
    double m_velocity { 1.0 };

    //! Where the fundamental is now, in log2 Hz, as opposed to where the contour wants it. Log
    //! rather than linear because a semitone has to take the same time to cross wherever it is.
    //! Negative until a note has set it, which is how a note-on knows to land rather than glide.
    double m_pitch { -1.0 };
    double m_pitchCoefficient { 1.0 };
    std::array<double, std::size(FlutterRates)> m_flutterPhases {};

    double m_fade { 0.0 };
    double m_fadeCoefficient { 0.0 };

    float m_rate { 1.0f };
    float m_glide { 0.35f };
    float m_formantShift { 0.5f };
    float m_breathiness { 0.1f };
    float m_consonantLevel { 0.5f };
    float m_sibilance { 0.31f };
    float m_voiceType { 0.0f };
    float m_openQuotient { 0.5f };
    float m_voicePerturbation { 0.5f };
    float m_voiceEngine { 1.0f };
    float m_velocitySensitivity { 0.5f };
    float m_intonation { 0.4f };
    float m_vibratoRate { 0.3f };
    float m_vibratoDepth { 0.0f };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
    float m_triggerMode { 0.0f };
    float m_syncMode { 0.0f };
    float m_syncLength { 4.0f };
    float m_syncDivision { 0.5f };
};

} // namespace noteahead

#endif // SPEECH_DEVICE_HPP
