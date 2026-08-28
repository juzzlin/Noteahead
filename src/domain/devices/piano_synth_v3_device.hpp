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

#ifndef PIANO_SYNTH_V3_DEVICE_HPP
#define PIANO_SYNTH_V3_DEVICE_HPP

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/modal_piano_string_v3.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <array>
#include <string>

namespace noteahead {

// Acoustic piano voiced against a Yamaha CP-80 recording, built on a bank of resonators
// rather than a string loop. See ModalPianoStringV3 for why, and for what this revision
// corrects in the string the second one plays.
//
// A separate device rather than a change to PianoSynthV2Device: the corrections move the
// sound, so a song that was written against V2 keeps playing V2.
class PianoSynthV3Device : public Device
{
public:
    explicit PianoSynthV3Device(std::string name);
    ~PianoSynthV3Device() override;

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

    float brightness() const;
    void setBrightness(float brightness);
    float decay() const;
    void setDecay(float decay);
    float inharmonicity() const;
    void setInharmonicity(float inharmonicity);
    float hammerHardness() const;
    void setHammerHardness(float hardness);
    float stringDetune() const;
    void setStringDetune(float detune);
    float stretch() const;
    void setStretch(float stretch);
    float richness() const;
    void setRichness(float richness);
    float doubleDecay() const;
    void setDoubleDecay(float doubleDecay);

    //! Scales the ramp the strike opens with, half being the one the model was fitted with.
    float attack() const;
    void setAttack(float attack);

    //! How much of the strike's level follows velocity, one being the reference's own law and
    //! half the default, which is what makes the top of the keyboard playable.
    float velocitySensitivity() const;
    void setVelocitySensitivity(float sensitivity);
    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);
    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);
    float releaseTime() const;
    void setReleaseTime(float releaseTime);
    float stereoWidth() const;
    void setStereoWidth(float stereoWidth);

protected:
    void syncParameters() override;

private:
    static constexpr int MaxVoices = 16;

    struct Voice
    {
        ModalPianoStringV3 string;
        uint8_t note { 0 };
        bool active { false };
        bool pendingRelease { false };

        void reset();
    };

    std::array<Voice, MaxVoices> m_voices;
    int m_nextVoiceToSteal { 0 };
    bool m_sustainPedal { false };

    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;

    CascadedSvf m_lpfL;
    CascadedSvf m_lpfR;
    CascadedSvf m_hpfL;
    CascadedSvf m_hpfR;

    TrueStereoPanner m_panner;
    TrueStereoPanner m_voicePanner;

    float m_brightness { 0.5f };
    float m_decay { 0.5f };
    float m_inharmonicity { 0.5f };
    float m_hammerHardness { 0.5f };
    float m_stringDetune { 0.35f };
    float m_stretch { 0.0f };
    float m_richness { 0.7f };
    float m_doubleDecay { 0.5f };
    float m_attack { 0.5f };
    float m_velocitySensitivity { 0.5f };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
    float m_releaseTime { 0.15f };
    float m_stereoWidth { 0.2f };

    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    void releaseVoice(Voice & voice) const;
    ModalPianoStringV3::Settings stringSettings() const;
    int findVoiceForNote(uint8_t note) const;
    int allocateVoice();
    float noteToPan(uint8_t note) const;

    std::string m_name;
};

} // namespace noteahead

#endif // PIANO_SYNTH_V3_DEVICE_HPP
