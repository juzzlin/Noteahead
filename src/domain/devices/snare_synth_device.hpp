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

#ifndef SNARE_SYNTH_DEVICE_HPP
#define SNARE_SYNTH_DEVICE_HPP

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/drum/snare_pm_engine.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <string>

namespace noteahead {

//! Physical modelling snare drum synthesizer device.
//!
//! Simulates circular membrane modal resonators (batter and resonant heads), non-linear
//! snare wire chatter, stick strike modeling with hardness and position control, shell body
//! resonance, pan spread, and an output filtering stage.
//!
//! Every note plays the drum as it is set, unless Chromatic is on: then the note plays it like a
//! sampler plays a sample. C-3 plays it as set. The engine runs at the sample rate divided by the
//! note's speed, so an octave above C-3 every pitch is an octave higher and every decay half as
//! long, and an octave below it twice as long. The output filters stay where they are set.
class SnareSynthDevice : public Device
{
public:
    explicit SnareSynthDevice(std::string name);
    ~SnareSynthDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;

    static std::string typeIdString();

    //! A note picks the speed in Chromatic mode and nothing otherwise, so there is no octave to
    //! shift it by: one left saved would silently play every note faster.
    bool supportsOctave() const override;
    void processDeviceMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processDeviceMidiNoteOff(uint8_t note) override;
    void processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void reset() override;
    void resetAudio() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    void loadPreset(int index);
    void applyPreset(int index, bool authored);

    // Head / Membrane parameters
    float tune() const;
    void setTune(float tune);
    float tone() const;
    void setTone(float tone);
    float decay() const;
    void setDecay(float decay);
    float headTension() const;
    void setHeadTension(float ratio);
    float pitchDepth() const;
    void setPitchDepth(float depth);
    float pitchDecay() const;
    void setPitchDecay(float decay);

    // Snare wire parameters
    float snappy() const;
    void setSnappy(float snappy);
    float snareDecay() const;
    void setSnareDecay(float decay);
    float snareTension() const;
    void setSnareTension(float tension);
    float snareBuzz() const;
    void setSnareBuzz(float buzz);

    // Stick strike & body parameters
    float click() const;
    void setClick(float click);
    float hardness() const;
    void setHardness(float hardness);
    float position() const;
    void setPosition(float position);
    float body() const;
    void setBody(float body);
    float drive() const;
    void setDrive(float drive);
    float panSpread() const;
    void setPanSpread(float spread);

    // Playback parameters
    bool chromatic() const;
    void setChromatic(bool chromatic);
    float velocitySensitivity() const;
    void setVelocitySensitivity(float sensitivity);

    // Output filters
    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);
    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);

protected:
    void syncParameters() override;

private:
    float tuneFrequency() const;
    //! How much faster @p note plays the drum: 1 unless Chromatic, and then 2 an octave above
    //! C-3.
    double speedRatio(uint8_t note) const;
    double engineSampleRate(double sampleRate) const;

    //! C-3: the note that plays the drum as it is set in Chromatic mode.
    static constexpr int RootNote { 36 };

    std::string m_name;
    SnarePmEngine m_engine {};

    CascadedSvf m_lpfL {};
    CascadedSvf m_lpfR {};
    CascadedSvf m_hpfL {};
    CascadedSvf m_hpfR {};

    TrueStereoPanner m_panner {};

    DcBlocker m_dcBlockerL {};
    DcBlocker m_dcBlockerR {};

    // Internal parameter values
    float m_tune { 0.2857f };
    float m_tone { 0.5f };
    float m_decay { 0.1554f };
    float m_headTension { 0.35f };
    float m_pitchDepth { 0.25f };
    float m_pitchDecay { 0.2105f };

    float m_snappy { 0.6f };
    float m_snareDecay { 0.2137f };
    float m_snareTension { 0.5f };
    float m_snareBuzz { 0.5f };

    float m_click { 0.4f };
    float m_hardness { 0.5f };
    float m_position { 0.2f };
    float m_body { 0.3f };
    float m_drive { 0.0f };
    float m_panSpread { 0.5f };

    bool m_chromatic { false };
    //! The speed of the note that last struck the drum.
    double m_speedRatio { 1.0 };
    float m_velocitySensitivity { 0.7f };

    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
};

} // namespace noteahead

#endif // SNARE_SYNTH_DEVICE_HPP
