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

#ifndef PIANO_SYNTH_DEVICE_HPP
#define PIANO_SYNTH_DEVICE_HPP

#include "../dsp/dc_blocker.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "../dsp/waveguide_string.hpp"
#include "device.hpp"

#include <array>
#include <string>

namespace noteahead {

class PianoSynthDevice : public Device
{
public:
    explicit PianoSynthDevice(std::string name);
    ~PianoSynthDevice() override;

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
    float releaseTime() const;
    void setReleaseTime(float releaseTime);
    float stereoWidth() const;
    void setStereoWidth(float stereoWidth);
    float hammerHardness() const;
    void setHammerHardness(float hardness);

protected:
    void syncParameters() override;

private:
    static constexpr int MaxVoices = 16;

    struct Voice
    {
        WaveguideString string;
        uint8_t note { 0 };
        float velocity { 1.0f };
        bool active { false };
        bool pendingRelease { false };

        void reset();
    };

    std::array<Voice, MaxVoices> m_voices;
    int m_nextVoiceToSteal { 0 };
    bool m_sustainPedal { false };

    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;

    TrueStereoPanner m_panner;
    TrueStereoPanner m_voicePanner;

    float m_brightness { 0.5f };
    float m_decay { 0.5f };
    float m_inharmonicity { 0.3f };
    float m_releaseTime { 0.3f };
    float m_stereoWidth { 0.7f };
    float m_hammerHardness { 0.5f };

    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    int findVoiceForNote(uint8_t note) const;
    int allocateVoice();
    float noteToPan(uint8_t note) const;

    std::string m_name;
};

} // namespace noteahead

#endif // PIANO_SYNTH_DEVICE_HPP
