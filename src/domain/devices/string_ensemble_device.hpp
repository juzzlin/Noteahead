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

#ifndef STRING_ENSEMBLE_DEVICE_HPP
#define STRING_ENSEMBLE_DEVICE_HPP

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/divide_down_generator.hpp"
#include "../dsp/ensemble_chorus.hpp"
#include "../dsp/phaser.hpp"
#include "../dsp/svf_filter.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace noteahead {

//! Divide-down string ensemble in the Solina tradition.
//!
//! Every key taps a shared top-octave divider (DivideDownGenerator) at 16', 8' and 4', gated by its
//! own Crescendo/Sustain envelope. The taps are summed onto section buses and coloured by one fixed
//! filter per register, which is how the hardware busses them: six filters in total, no matter how
//! many keys are held, so polyphony costs nothing but an envelope.
//!
//! Keys below the split feed the bass section (Contrabass 16', Cello 8') at Volume Bass level; keys
//! at or above it feed the upper section (Horn 16', Viola 8', Trumpet 8', Violin 4'). The summed
//! mono signal is turned into stereo by the ensemble chorus and then swept by the phaser.
class StringEnsembleDevice : public Device
{
public:
    explicit StringEnsembleDevice(std::string name);
    ~StringEnsembleDevice() override;

    //! First note of the upper section. Below it the keyboard drives the bass section, as on the
    //! hardware, whose lower two octaves are wired to Contrabass and Cello only.
    static constexpr uint8_t SplitNote { 59 }; // B3

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

    bool contrabassEnabled() const;
    void setContrabassEnabled(bool enabled);

    bool celloEnabled() const;
    void setCelloEnabled(bool enabled);

    bool violaEnabled() const;
    void setViolaEnabled(bool enabled);

    bool violinEnabled() const;
    void setViolinEnabled(bool enabled);

    bool trumpetEnabled() const;
    void setTrumpetEnabled(bool enabled);

    bool hornEnabled() const;
    void setHornEnabled(bool enabled);

    bool modulationEnabled() const;
    void setModulationEnabled(bool enabled);

    bool phaserEnabled() const;
    void setPhaserEnabled(bool enabled);

    float volumeBass() const;
    void setVolumeBass(float volumeBass);

    float crescendo() const;
    void setCrescendo(float crescendo);

    float sustainLength() const;
    void setSustainLength(float sustainLength);

    float phaserColor() const;
    void setPhaserColor(float color);

    float phaserRate() const;
    void setPhaserRate(float rate);

    float velocitySensitivity() const;
    void setVelocitySensitivity(float sensitivity);

    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);

    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);

protected:
    void syncParameters() override;

private:
    static constexpr int KeyCount { 128 };

    //! One register of the instrument: an octave tap plus the fixed voicing filter that names it.
    enum class Register
    {
        Contrabass,
        Cello,
        Horn,
        Viola,
        Trumpet,
        Violin
    };

    static constexpr int RegisterCount { 6 };

    struct KeyState
    {
        AdsrEnvelope gate;
        float velocity { 1.0f };
        bool active { false };
    };

    void updateKeyTimes();
    void updateRegisterFilters(double sampleRate);
    bool registerEnabled(Register reg) const;
    void releaseKey(uint8_t note);

    std::string m_name;

    DivideDownGenerator m_generator;
    std::array<KeyState, KeyCount> m_keys;
    std::vector<uint8_t> m_activeKeys;

    std::array<SvfFilter, RegisterCount> m_registerFilters;
    double m_lastRegisterFilterSampleRate { 0.0 };

    //! Frames the ensemble and phaser are still allowed to ring after the last key has closed.
    //! Without it the engine would drop the device while its delay lines still hold signal.
    uint32_t m_tailFrames { 0 };

    EnsembleChorus m_ensemble;
    Phaser m_phaser;
    TrueStereoPanner m_panner;
    CascadedSvf m_lpfL;
    CascadedSvf m_lpfR;
    CascadedSvf m_hpfL;
    CascadedSvf m_hpfR;

    bool m_contrabassEnabled { false };
    bool m_celloEnabled { false };
    bool m_violaEnabled { true };
    bool m_violinEnabled { true };
    bool m_trumpetEnabled { false };
    bool m_hornEnabled { false };

    bool m_modulationEnabled { true };
    bool m_phaserEnabled { false };

    float m_volumeBass { 0.7f };
    float m_crescendo { 0.15f };
    float m_sustainLength { 0.35f };
    float m_phaserColor { 0.5f };
    float m_phaserRate { 0.3f };
    float m_velocitySensitivity { 1.0f };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
};

} // namespace noteahead

#endif // STRING_ENSEMBLE_DEVICE_HPP
