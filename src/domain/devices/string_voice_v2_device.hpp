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

#ifndef STRING_VOICE_V2_DEVICE_HPP
#define STRING_VOICE_V2_DEVICE_HPP

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/ensemble_chorus.hpp"
#include "../dsp/formant_filter_bank.hpp"
#include "../dsp/one_pole_filter.hpp"
#include "../dsp/poly_blep_oscillator.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "../dsp/vocoder.hpp"
#include "device.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace noteahead {

class StringVoiceV2Device : public Device
{
public:
    explicit StringVoiceV2Device(std::string name);
    ~StringVoiceV2Device() override;

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

    std::vector<size_t> sidechainDependencies() const override;

    void reset() override;
    void resetAudio() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    // Getters and Setters for Parameters
    float stringsBalance() const;
    void setStringsBalance(float val);

    float voiceBalance() const;
    void setVoiceBalance(float val);

    bool stringsUpper() const;
    void setStringsUpper(bool val);

    bool stringsLower() const;
    void setStringsLower(bool val);

    float stringsTone() const;
    void setStringsTone(float val);

    float velocitySensitivity() const;
    void setVelocitySensitivity(float val);

    float stringsAttack() const;
    void setStringsAttack(float val);

    float stringsRelease() const;
    void setStringsRelease(float val);

    float voiceMale8() const;
    void setVoiceMale8(float val);

    float voiceMale4() const;
    void setVoiceMale4(float val);

    float voiceUpperMale8() const;
    void setVoiceUpperMale8(float val);

    float voiceFemale4() const;
    void setVoiceFemale4(float val);

    float voiceAttack() const;
    void setVoiceAttack(float val);

    float voiceRelease() const;
    void setVoiceRelease(float val);

    float vibratoRate() const;
    void setVibratoRate(float val);

    float vibratoDepth() const;
    void setVibratoDepth(float val);

    float vibratoDelay() const;
    void setVibratoDelay(float val);

    float panSpread() const;
    void setPanSpread(float val);

    bool ensembleEnabled() const;
    void setEnsembleEnabled(bool val);

    int ensembleMode() const;
    void setEnsembleMode(int val);

    bool vocoderEnabled() const;
    void setVocoderEnabled(bool val);

    int vocoderSidechain() const;
    void setVocoderSidechain(int val);

    float lpfCutoff() const;
    void setLpfCutoff(float val);

    float hpfCutoff() const;
    void setHpfCutoff(float val);

protected:
    void syncParameters() override;

private:
    static constexpr int MaxVoices { 32 };

    //! Where the keyboard splits into Lower and Upper, as the hardware's does: below it a note
    //! sounds the Lower registers and the Lower strings switch, at or above it the Upper ones.
    static constexpr uint8_t SplitNote { 60 }; // C4

    struct Voice
    {
        PolyBlepOscillator stringOsc;
        //! Sine at the note's own frequency, lifting the fundamental above a saw's. See
        //! StringsFundamentalLift.
        PolyBlepOscillator stringFundamental;
        PolyBlepOscillator voiceOsc8;
        PolyBlepOscillator voiceOsc4;

        AdsrEnvelope stringEg;
        AdsrEnvelope voiceEg;

        uint8_t note { 0 };
        float velocity { 1.0f };
        bool active { false };
        uint32_t triggerFrame { 0 }; // used to calculate vibrato delay time
        uint64_t triggerId { 0 }; // monotonic allocation order, used for stealing
        double detuneRatio { 1.0 }; // fixed per-voice pitch drift (analog divider imprecision)
        double pwmPhase { 0.0 }; // per-voice PWM LFO phase offset for choir movement
        double vibratoPhase { 0.0 }; // own vibrato phase, so a chord does not vibrate in lockstep
        double vibratoRateRatio { 1.0 };
        double driftPhase { 0.0 }; // slow tuning wander, the voice section's alone
        double driftRateRatio { 1.0 };
        double pan { 0.5 };

        void reset();
    };

    std::optional<size_t> vocoderSidechainIndex() const;
    int findVoiceForNote(uint8_t note) const;
    int allocateVoice();

    std::string m_name;
    std::array<Voice, MaxVoices> m_voices;
    uint64_t m_nextTriggerId { 1 };

    // Device Parameters cache
    //! Section balance, the hardware's own Balance sliders: one level per section, so a patch can
    //! be tipped between strings and voices without touching each register's footage.
    float m_stringsBalance { 1.0f };
    float m_voiceBalance { 1.0f };

    // The hardware's Strings section has no footage of its own: the two switches only say which
    // half of the split it sounds on, and Balance sets how loud.
    bool m_stringsUpper { true };
    bool m_stringsLower { true };
    //! Middle by default, which is where the tone network was measured.
    float m_stringsTone { 0.5f };

    //! Shared by both sections, as the hardware's keyboard is. Half by default: the sections keep
    //! some life from how hard a key is struck without going silent under a light one.
    float m_velocitySensitivity { 0.5f };
    float m_stringsAttack { 50.0f };
    float m_stringsRelease { 800.0f };

    // The registers follow the hardware's two switches: Lower carries Male 8' and 4', Upper carries
    // Male 8' and Female 4'. There is no Female 8' - the female voice exists only an octave up, in
    // the upper half of the keyboard.
    float m_voiceMale8 { 0.8f };
    float m_voiceMale4 { 0.0f };
    float m_voiceUpperMale8 { 0.8f };
    float m_voiceFemale4 { 0.0f };
    float m_voiceAttack { 100.0f };
    float m_voiceRelease { 1000.0f };

    float m_vibratoRate { 6.0f };
    float m_vibratoDepth { 0.0f };
    float m_vibratoDelay { 0.0f };
    float m_panSpread { 0.0f };

    bool m_ensembleEnabled { true };
    int m_ensembleMode { 0 };

    bool m_vocoderEnabled { false };
    int m_vocoderSidechain { -1 };

    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };

    // Common DSP blocks
    FormantFilterBank m_formantFiltersL;
    FormantFilterBank m_formantFiltersR;
    EnsembleChorus m_ensemble;
    TrueStereoPanner m_panner;
    Vocoder m_vocoder;
    CascadedSvf m_lpfL;
    CascadedSvf m_lpfR;
    CascadedSvf m_hpfL;
    CascadedSvf m_hpfR;

    // The Strings section's own tone network, ahead of the mix with the voice section. The high
    // pass is first order, which is what the measurement asks for: the response climbs about
    // 6 dB/octave below its corner, half what a second-order one would give.
    OnePoleFilter m_stringsHpfL;
    OnePoleFilter m_stringsHpfR;
    CascadedSvf m_stringsLpfL;
    CascadedSvf m_stringsLpfR;
};

} // namespace noteahead

#endif // STRING_VOICE_V2_DEVICE_HPP
