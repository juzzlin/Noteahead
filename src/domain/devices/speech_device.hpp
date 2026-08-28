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

    std::vector<MidiCcController> availableMidiCcControllers() const override;

    static std::string typeIdString();

    //! What the device says when it has never been given anything to say. A device that is silent
    //! until configured reads as broken.
    static std::string defaultPhrase();

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

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

protected:
    void syncParameters() override;

private:
    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    void compilePhrase();
    //! Fundamental for the note being spoken, with the intonation contour and vibrato on it.
    double currentFrequency() const;

    //! Semitones the contour falls across an utterance at full intonation.
    //!
    //! Kept smaller than the accent a stressed syllable takes, or the two fight: at four semitones
    //! the fall had eaten the whole accent by the middle of a phrase, and the syllables the accent
    //! was supposed to mark came out no higher than the ones around them.
    static constexpr double IntonationRange = 2.0;

    //! Seconds the output takes to fade when an utterance ends or is cut off. Without it the end of
    //! a phrase is a step to zero, which clicks.
    static constexpr double OutputFadeTime = 0.006;

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

    double m_fade { 0.0 };
    double m_fadeCoefficient { 0.0 };

    float m_rate { 1.0f };
    float m_glide { 0.35f };
    float m_formantShift { 0.5f };
    float m_breathiness { 0.1f };
    float m_consonantLevel { 0.5f };
    float m_sibilance { 0.31f };
    float m_voiceType { 0.0f };
    float m_velocitySensitivity { 0.5f };
    float m_intonation { 0.4f };
    float m_vibratoRate { 0.3f };
    float m_vibratoDepth { 0.15f };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
    float m_triggerMode { 0.0f };
    float m_syncMode { 0.0f };
    float m_syncLength { 4.0f };
    float m_syncDivision { 0.5f };
};

} // namespace noteahead

#endif // SPEECH_DEVICE_HPP
