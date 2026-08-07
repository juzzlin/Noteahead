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

#ifndef KICK_808_DEVICE_HPP
#define KICK_808_DEVICE_HPP

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/drum/kick_808_engine.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <string>

namespace noteahead {

//! Monophonic TR-808-style bass drum.
//!
//! It is deliberately a pitched instrument rather than a drum machine voice. The 808 kick is played
//! as a bass line as often as it is played as a drum, so with Key Track on it follows the note
//! column across the keyboard and Glide slurs between notes. Turning Key Track off pins it to a
//! fixed pitch and it behaves like the hardware's single tuned voice.
//!
//! Notes are one-shot: the tail length is set by Decay and a note off does not cut it, matching the
//! hardware. Only an all-notes-off chokes a ringing hit.
class Kick808Device : public Device
{
public:
    explicit Kick808Device(std::string name);
    ~Kick808Device() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    std::vector<MidiCcController> availableMidiCcControllers() const override;

    static std::string typeIdString();

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

    float tune() const;
    void setTune(float tune);
    float tone() const;
    void setTone(float tone);
    float decay() const;
    void setDecay(float decay);
    float pitchDepth() const;
    void setPitchDepth(float depth);
    float pitchDecay() const;
    void setPitchDecay(float decay);
    float drive() const;
    void setDrive(float drive);
    float glide() const;
    void setGlide(float glide);
    bool keyTrack() const;
    void setKeyTrack(bool keyTrack);
    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);
    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);

protected:
    void syncParameters() override;

private:
    //! Pitch the voice sits at when Key Track is off, and the origin the Tune offset is applied to.
    static constexpr float ReferenceNote = 36.0f;
    //! Tune spans two octaves either side of the played note.
    static constexpr float TuneRangeSemitones = 24.0f;

    float effectiveNote(uint8_t note) const;

    Kick808Engine m_engine;
    // The voice is mono, so one filter each is enough: filtering ahead of the panner is equivalent
    // to a per-channel pair after it, at half the cost.
    CascadedSvf m_lpf;
    CascadedSvf m_hpf;
    TrueStereoPanner m_panner;
    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;

    float m_tune { 0.5f };
    float m_tone { 0.35f };
    float m_decay { 0.6f };
    float m_pitchDepth { 0.5f };
    float m_pitchDecay { 0.25f };
    float m_drive { 0.0f };
    float m_glide { 0.0f };
    bool m_keyTrack { true };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };

    std::string m_name;
};

} // namespace noteahead

#endif // KICK_808_DEVICE_HPP
