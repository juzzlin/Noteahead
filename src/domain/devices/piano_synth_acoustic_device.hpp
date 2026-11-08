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

#ifndef PIANO_SYNTH_ACOUSTIC_DEVICE_HPP
#define PIANO_SYNTH_ACOUSTIC_DEVICE_HPP

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/modal_grand_string.hpp"
#include "../dsp/piano_soundboard.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <array>
#include <string>
#include <vector>

namespace noteahead {

//! Physically modelled acoustic grand, fitted to the Yamaha CP4's CFX sample: see ModalGrandString
//! for the strings and PianoSoundboard for the rest of the instrument.
//!
//! The pianist's side of it is here: the sustain pedal (CC 64) lifts every damper, so the keys ring
//! on after they are let go and the other strings ring in sympathy; the soft pedal (CC 67) moves the
//! hammers onto softer felt. The top keys have no dampers, as on the instrument.
class PianoSynthAcousticDevice : public Device
{
public:
    explicit PianoSynthAcousticDevice(std::string name);
    ~PianoSynthAcousticDevice() override;

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

    bool sustainPedal() const;
    //! How many keys are sounding, for tests
    int activeVoiceCount() const;

    static constexpr int MaxVoices = 32;

protected:
    void syncParameters() override;

private:
    struct Voice
    {
        ModalGrandString string;
        bool active { false };
        //! The key is up but the pedal holds the damper off
        bool pendingRelease { false };
        //! The key has been struck again: this string rings out but no longer answers to it
        bool retired { false };
    };

    float value(const QString & key) const;
    void damp(Voice & voice);
    int findVoice(uint8_t note) const;
    int allocateVoice();
    void updateOpenStrings();

    std::string m_name;
    std::array<Voice, MaxVoices> m_voices {};
    PianoSoundboard m_soundboard;

    CascadedSvf m_lpfL {};
    CascadedSvf m_lpfR {};
    CascadedSvf m_hpfL {};
    CascadedSvf m_hpfR {};
    TrueStereoPanner m_panner {};
    DcBlocker m_dcBlockerL {};
    DcBlocker m_dcBlockerR {};

    ModalGrandString::Settings m_stringSettings {};
    float m_knock { 0.5f };
    float m_resonance { 0.5f };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
    bool m_sustainPedal { false };
    bool m_softPedal { false };
    bool m_openStringsDirty { true };
};

} // namespace noteahead

#endif // PIANO_SYNTH_ACOUSTIC_DEVICE_HPP
