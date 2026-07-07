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

#ifndef STRING_VOICE_DEVICE_HPP
#define STRING_VOICE_DEVICE_HPP

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/ensemble_chorus.hpp"
#include "../dsp/formant_filter_bank.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/poly_blep_oscillator.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "../dsp/vocoder.hpp"
#include "device.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace noteahead {

class StringVoiceDevice : public Device
{
public:
    explicit StringVoiceDevice(std::string name);
    ~StringVoiceDevice() override;

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
    float stringsLevel8() const;
    void setStringsLevel8(float val);

    float stringsLevel4() const;
    void setStringsLevel4(float val);

    float stringsAttack() const;
    void setStringsAttack(float val);

    float stringsRelease() const;
    void setStringsRelease(float val);

    float voiceMale8() const;
    void setVoiceMale8(float val);

    float voiceMale4() const;
    void setVoiceMale4(float val);

    float voiceFemale8() const;
    void setVoiceFemale8(float val);

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

    struct Voice
    {
        PolyBlepOscillator stringOsc8;
        PolyBlepOscillator stringOsc4;
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
    float m_stringsLevel8 { 0.8f };
    float m_stringsLevel4 { 0.0f };
    float m_stringsAttack { 50.0f };
    float m_stringsRelease { 800.0f };

    float m_voiceMale8 { 0.8f };
    float m_voiceMale4 { 0.0f };
    float m_voiceFemale8 { 0.0f };
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
    Lfo m_vibratoLfo;
    Vocoder m_vocoder;
    CascadedSvf m_lpfL;
    CascadedSvf m_lpfR;
    CascadedSvf m_hpfL;
    CascadedSvf m_hpfR;
};

} // namespace noteahead

#endif // STRING_VOICE_DEVICE_HPP
