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

#ifndef KICK_SYNTH_DEVICE_HPP
#define KICK_SYNTH_DEVICE_HPP

#include "../../../domain/devices/device.hpp"
#include "../../../domain/dsp/cascaded_svf.hpp"
#include "../../../domain/dsp/dc_blocker.hpp"
#include "../../../domain/dsp/drum/drum_amp_envelope.hpp"
#include "../../../domain/dsp/drum/kick_pm_engine.hpp"
#include "../../../domain/dsp/true_stereo_panner.hpp"

#include <optional>
#include <string>

namespace noteahead {

//! Physically modelled acoustic bass drum: see KickPmEngine.
//!
//! Every note plays the drum as it is set, unless Chromatic is on: then the note plays it like a
//! sampler plays a sample. C-3 plays it as set. The engine runs at the sample rate divided by the
//! note's speed, so an octave above C-3 every pitch is an octave higher and every decay half as
//! long. The output filters stay where they are set.
class KickSynthDevice : public Device
{
public:
    explicit KickSynthDevice(std::string name);
    ~KickSynthDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;
    std::vector<std::string> modulationWheelTargets() const override;

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
    void resetDeviceAudio() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    void loadPreset(int index);
    void applyPreset(int index, bool authored);

    //! A knob of the drum, by its key, 0..1 as stored. For the controller, which has one property per
    //! knob and would otherwise need a getter and a setter for each.
    float knob(const QString & key) const;
    void setKnob(const QString & key, float value);

    // What the knobs stand for, for the presets and the dialog's read-outs
    static double tuneHz(float knob);
    static double decaySeconds(float knob);
    static double frontTuneRatio(float knob);
    static double ringLevel(float knob);
    static double ringDecaySeconds(float knob);
    static constexpr double MinTuneHz { 25.0 };
    static constexpr double MaxTuneHz { 400.0 };
    static constexpr double MinDecaySeconds { 0.1 };
    static constexpr double MaxDecaySeconds { 8.0 };
    static constexpr double MinFrontTune { 0.8 };
    static constexpr double MaxFrontTune { 3.0 };
    static constexpr double MaxRing { 2.5 };
    static constexpr double MinRingDecaySeconds { 0.03 };
    static constexpr double MaxRingDecaySeconds { 1.5 };

    //! The envelope's times as the knobs set them, for the waveform overlay
    double ampAttackSeconds() const;
    double ampHoldSeconds() const;
    double ampDecaySeconds() const;
    double ampReleaseSeconds() const;
    //! False at the defaults: Sustain at full with no attack or release leaves the drum as it is.
    bool ampEnvelopeEnabled() const;

    bool chromatic() const;
    void setChromatic(bool chromatic);

protected:
    void syncParameters() override;

private:
    double speedRatio(uint8_t note) const;
    double engineSampleRate(double sampleRate) const;
    void syncEngine();
    void syncAmpEnvelope();
    float value(const QString & key) const;

    //! C-3: the note that plays the drum as it is set in Chromatic mode.
    static constexpr int RootNote { 36 };

    std::string m_name;
    KickPmEngine m_engine {};

    CascadedSvf m_lpfL {};
    CascadedSvf m_lpfR {};
    CascadedSvf m_hpfL {};
    CascadedSvf m_hpfR {};

    TrueStereoPanner m_panner {};

    DrumAmpEnvelope m_ampEnvelope {};
    bool m_ampEnvelopeEnabled { false };

    DcBlocker m_dcBlockerL {};
    DcBlocker m_dcBlockerR {};

    bool m_chromatic { false };
    //! The speed of the note that last struck the drum.
    double m_speedRatio { 1.0 };
    float m_velocitySensitivity { 0.7f };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
};

} // namespace noteahead

#endif // KICK_SYNTH_DEVICE_HPP
