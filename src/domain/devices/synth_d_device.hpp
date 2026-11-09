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

#ifndef SYNTH_D_DEVICE_HPP
#define SYNTH_D_DEVICE_HPP

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/synth_d_voice.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <optional>
#include <string>
#include <vector>

namespace noteahead {

//! A monophonic three-VCO synthesizer after the Behringer Model D: see SynthDVoice for the sound.
//!
//! The keyboard is the Model D's: one note at a time, the lowest held key by default, and legato
//! unless multi-trigger is on. With multi-trigger, going back to a key still held retriggers the
//! contours as a new key does, the firmware 1.0.6 way.
class SynthDDevice : public Device
{
public:
    enum class NotePriority
    {
        Low,
        High,
        Last
    };

    explicit SynthDDevice(std::string name);
    ~SynthDDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;

    static std::string typeIdString();

    void processDeviceMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processDeviceMidiNoteOff(uint8_t note) override;
    void processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiPitchBend(uint16_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(AudioContext & context) override;
    void advanceIdle(const AudioContext & context) override;
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
    //! A switch or a selector, by its key
    int choice(const QString & key) const;
    void setChoice(const QString & key, int value);

    //! The note the voice plays, for tests
    std::optional<uint8_t> soundingNote() const;

    //! Which LED slot shows what
    static constexpr size_t LfoLed { 0 };
    static constexpr size_t OverloadLed { 1 };

protected:
    void syncParameters() override;

private:
    float value(const QString & key) const;
    //! The key the voice should play among those held, by the note priority
    std::optional<uint8_t> priorityNote() const;
    //! Moves the voice to whichever key now has priority, after a key went down or up
    void followKeys();

    std::string m_name;
    SynthDVoice m_voice;
    SynthDVoice::Settings m_settings {};

    //! The keys held, in the order they went down
    std::vector<uint8_t> m_heldKeys;
    std::optional<uint8_t> m_soundingNote;
    double m_velocity { 1.0 };

    CascadedSvf m_lpfL {};
    CascadedSvf m_lpfR {};
    CascadedSvf m_hpfL {};
    CascadedSvf m_hpfR {};
    TrueStereoPanner m_panner {};
    DcBlocker m_dcBlockerL {};
    DcBlocker m_dcBlockerR {};

    NotePriority m_notePriority { NotePriority::Low };
    bool m_multiTrigger { false };
    int m_pitchBendRange { 2 };
    float m_velocitySensitivity { 0.0f };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
};

} // namespace noteahead

#endif // SYNTH_D_DEVICE_HPP
