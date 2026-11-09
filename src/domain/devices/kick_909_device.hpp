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

#ifndef KICK_909_DEVICE_HPP
#define KICK_909_DEVICE_HPP

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/drum/drum_amp_envelope.hpp"
#include "../dsp/drum/kick_909_engine.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <string>

namespace noteahead {

//! The TR-909 bass drum with the RD-9's Pitch and Pitch Depth, plus Drive: see Kick909Engine.
//!
//! Every note plays the drum as it is set, unless Chromatic is on: then the note moves Pitch, C-3
//! playing it as set. The sweep moves with it and the decays stay where they are, so it plays as a
//! tuned 909 rather than as a sample played faster.
class Kick909Device : public Device
{
public:
    explicit Kick909Device(std::string name);
    ~Kick909Device() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;

    static std::string typeIdString();

    //! A note picks the pitch in Chromatic mode and nothing otherwise, so there is no octave to
    //! shift it by
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

    //! A knob, by its key, 0..1 as stored
    float knob(const QString & key) const;
    void setKnob(const QString & key, float value);

    // What the knobs stand for, for the presets and the dialog's read-outs
    static double pitchHz(float knob);
    static double pitchDepth(float knob);
    static float pitchKnob(double hz);
    static float pitchDepthKnob(double depth);
    static constexpr double MinPitchHz { 30.0 };
    static constexpr double MaxPitchHz { 120.0 };
    static constexpr double MaxPitchDepth { 2.0 };

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
    double notePitchHz(uint8_t note) const;
    void syncEngine();
    void syncAmpEnvelope();
    float value(const QString & key) const;

    //! C-3: the note that plays the drum as it is set in Chromatic mode
    static constexpr int RootNote { 36 };

    std::string m_name;
    Kick909Engine m_engine {};

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
    //! The note that last struck the drum, for Pitch to follow it while it rings
    uint8_t m_note { RootNote };
    float m_velocitySensitivity { 0.7f };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
};

} // namespace noteahead

#endif // KICK_909_DEVICE_HPP
