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

#ifndef ELECTRIC_BASS_DEVICE_HPP
#define ELECTRIC_BASS_DEVICE_HPP

#include "../dsp/dc_blocker.hpp"
#include "../dsp/modal_bass_string.hpp"
#include "../dsp/svf_filter.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

//! Physically modelled, finger-played 5-string electric bass: see ModalBassString.
//!
//! The device is the bassist's left hand. Each note goes to a string, the lowest fret that reaches
//! it on a string not already held, and a repeated note to its own string. A note that overlaps a
//! held one within a hand's reach on that string is a hammer-on or a pull-off instead of a new
//! pluck, and letting go of it pulls off to the note still held under it. Letting go of the last
//! note on a string mutes it.
class ElectricBassDevice : public Device
{
public:
    explicit ElectricBassDevice(std::string name);
    ~ElectricBassDevice() override;

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

    int pitchBendRange() const;
    void setPitchBendRange(int range);

    //! Which string a note is sounding on, for tests: nullopt if none
    std::optional<size_t> stringOf(uint8_t note) const;

    static constexpr size_t StringCount = 5;
    //! B0 E1 A1 D2 G2, as MIDI notes
    static constexpr std::array<int, StringCount> OpenStrings { 23, 28, 33, 38, 43 };
    //! How far a fretting hand reaches for a hammer-on or a pull-off, in semitones
    static constexpr int LegatoReach = 5;

    // What the knobs stand for, for the presets and the dialog's read-outs
    static double pluckMm(float knob);
    static double fingerMm(float knob);
    static double sustainSeconds(float knob);
    static double lossPerHz(float knob);
    static double releaseSeconds(float knob);
    static constexpr double MinPluckMm { 40.0 };
    static constexpr double MaxPluckMm { 300.0 };
    static constexpr double SoftFingerMm { 25.0 };
    static constexpr double HardFingerMm { 3.0 };
    static constexpr double MinSustainSeconds { 2.0 };
    static constexpr double MaxSustainSeconds { 40.0 };
    static constexpr double DullLossPerHz { 0.010 };
    static constexpr double BrightLossPerHz { 0.001 };
    static constexpr double MinReleaseSeconds { 0.02 };
    static constexpr double MaxReleaseSeconds { 1.0 };

protected:
    void syncParameters() override;

private:
    struct Voice
    {
        ModalBassString string;
        //! The notes held on the string, the sounding one last
        std::vector<uint8_t> held;
        //! The note sounding, held or ringing on after it was let go
        std::optional<uint8_t> note;
        float velocity { 0.0f };
    };

    void startNote(size_t stringIndex, uint8_t note, float velocity, bool legato);
    size_t chooseString(uint8_t note) const;
    void updateTone(double sampleRate);
    float value(const QString & key) const;

    std::string m_name;
    std::array<Voice, StringCount> m_voices {};

    SvfFilter m_tone {};
    TrueStereoPanner m_panner {};
    DcBlocker m_dcBlockerL {};
    DcBlocker m_dcBlockerR {};

    ModalBassString::Settings m_stringSettings {};
    double m_toneHz { 0.0 };
    double m_toneQ { 0.0 };
    double m_toneSampleRate { 0.0 };
    float m_velocitySensitivity { 0.7f };
    int m_pitchBendRange { 2 };
    double m_bendTarget { 1.0 };
    double m_bend { 1.0 };
};

} // namespace noteahead

#endif // ELECTRIC_BASS_DEVICE_HPP
