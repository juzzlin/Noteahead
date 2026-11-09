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

#ifndef SYNTH_D_VOICE_HPP
#define SYNTH_D_VOICE_HPP

#include "analog_drift.hpp"
#include "noise_source.hpp"
#include "transistor_ladder_filter.hpp"
#include "upsampler.hpp"

#include <array>
#include <cstdint>
#include <utility>

namespace noteahead {

//! The sound of a Model D: three VCOs, a mixer that overdrives the filter, the transistor ladder,
//! two ADS contour generators and the modulation bus, as one monophonic voice.
//!
//! The panel is laid out as the Behringer's, which adds an LFO and a high-pass mode to the original.
//! Ext In is the main output fed back into the mixer, which is what it is on the hardware when
//! nothing is patched into it.
//!
//! Pan Spread places OSC 1 to the left and OSC 2 to the right, OSC 3 and the noise in the middle.
//! That needs a filter per side, so the voice runs two ladders and two feedback loops; at no
//! spread they are fed the same and the voice is the mono instrument.
class SynthDVoice
{
public:
    enum class Range
    {
        Lo,
        Feet32,
        Feet16,
        Feet8,
        Feet4,
        Feet2
    };

    //! The second waveform is the triangle-saw on OSC 1 and 2 and the reverse saw on OSC 3
    enum class Waveform
    {
        Triangle,
        TriangleSaw,
        Saw,
        Square,
        WidePulse,
        NarrowPulse
    };

    enum class ModSource1
    {
        Osc3,
        FilterEg
    };

    enum class ModSource2
    {
        Noise,
        Lfo
    };

    enum class LfoWaveform
    {
        Triangle,
        Square
    };

    enum class NoiseColour
    {
        White,
        Pink
    };

    struct Oscillator
    {
        Range range { Range::Feet8 };
        Waveform waveform { Waveform::Saw };
        //! Semitones, -7..7; OSC 1 has no knob of its own and stays at 0
        double frequency { 0.0 };
        double volume { 1.0 }; //!< 0..1, the panel's 0..10
        bool on { true };
    };

    struct Contour
    {
        double attack { 0.0 }; //!< Knob travel 0..1
        double decay { 0.5 }; //!< Knob travel 0..1
        double sustain { 1.0 }; //!< Level 0..1
        bool decaySwitch { false }; //!< Release takes the decay time; off, the note stops at once
    };

    struct Settings
    {
        double tune { 0.0 }; //!< Semitones, -2..2
        double glide { 0.0 }; //!< Knob travel 0..1
        double modMix { 0.0 }; //!< 0 = source 1 only, 1 = source 2 only
        double modDepth { 0.0 }; //!< Knob travel 0..1
        ModSource1 modSource1 { ModSource1::Osc3 };
        ModSource2 modSource2 { ModSource2::Noise };
        bool oscillatorModulation { false };
        bool filterModulation { false };
        LfoWaveform lfoWaveform { LfoWaveform::Triangle };
        double lfoRate { 0.5 }; //!< Knob travel 0..1

        std::array<Oscillator, 3> oscillators {};
        bool osc3KeyboardControl { true };

        double feedback { 0.0 }; //!< Ext In volume, 0..1
        bool feedbackOn { false };
        double noise { 0.0 }; //!< 0..1
        bool noiseOn { false };
        NoiseColour noiseColour { NoiseColour::White };

        TransistorLadderFilter::Mode filterMode { TransistorLadderFilter::Mode::LowPass };
        double cutoff { 0.5 }; //!< Knob travel 0..1, the panel's -4..4
        double emphasis { 0.0 }; //!< 0..1
        double contourAmount { 0.0 }; //!< 0..1
        bool keyboardControl1 { false };
        bool keyboardControl2 { false };
        Contour filterContour {};
        Contour loudnessContour {};

        double drift { 0.0 }; //!< 0..1, the three VCOs wandering together
        double instability { 0.0 }; //!< 0..1, each VCO wandering on its own
        double tolerance { 0.0 }; //!< 0..1, the VCOs' and the filter's fixed offsets
        double panSpread { 0.0 }; //!< 0..1
    };

    //! What a knob's travel means. Public for the tests and the device's parameter display.
    static double panelSeconds(double knob, double atZero, double atTen);
    static double attackSeconds(double knob);
    static double decaySeconds(double knob);
    static double glideSeconds(double knob);
    static double lfoHz(double knob);
    static double cutoffHz(double knob);
    //! Frequency of the VCO at @p range for @p note with no tuning or modulation
    static double oscillatorHz(Range range, double note);

    static constexpr double LowestCutoffHz { 10.0 };
    static constexpr double HighestCutoffHz { 20000.0 };
    //! How far a full contour amount opens the filter, as Moog specifies it
    static constexpr double MaxContourOctaves { 4.0 };
    //! Where full keyboard control leaves the cutoff where the knob puts it
    static constexpr int KeyboardControlReferenceNote { 48 };
    //! LO is this many octaves below 8'
    static constexpr double LoRangeOctaves { 7.0 };
    //! The key OSC 3 is tuned to when the keyboard does not control it
    static constexpr int Osc3FixedNote { 60 };

    void setSampleRate(double sampleRate);
    void setSettings(const Settings & settings);

    //! Moves the voice to @p note: at once, or gliding when the glide is up. @p trigger restarts
    //! both contours, which is a new note on a released keyboard, or any note with multi-trigger.
    void play(uint8_t note, double velocity, bool trigger);
    //! The last key is up: both contours go into their release
    void release();

    void setPitchBend(double semitones);

    //! One sample of the voice, added to @p left and @p right
    void process(double & left, double & right);
    //! Runs the LFO on for @p frames while nothing sounds, as the instrument's does
    void advanceLfo(uint32_t frames);

    bool isActive() const;
    //! The mixer's overload LED, 0..1
    double overload() const;
    //! The LFO, -1..1, for its LED
    double lfoValue() const;
    void reset();

    static constexpr size_t Oversampling { 2 };

private:
    //! An ADS contour generator: a capacitor charged and discharged through the time knobs
    class ContourGenerator
    {
    public:
        void set(const Contour & contour, double sampleRate);
        void trigger();
        void release();
        double next();
        double value() const;
        bool isIdle() const;
        void reset();

    private:
        enum class Stage
        {
            Idle,
            Attack,
            Decay,
            Release
        };
        Stage m_stage { Stage::Idle };
        double m_value { 0.0 };
        double m_attackStep { 1.0 };
        double m_decayCoefficient { 0.0 };
        double m_releaseCoefficient { 0.0 };
        double m_sustain { 1.0 };
    };

    struct Vco
    {
        double phase { 0.0 };
        double next(Waveform waveform, bool reverse, double increment);
    };

    double noteHz(double note) const;
    double modulationSource1(double filterEnvelope) const;

    Settings m_settings {};
    double m_sampleRate { 48000.0 };

    std::array<Vco, 3> m_vcos {};
    std::array<double, 3> m_vcoOutputs {};
    std::array<AnalogDrift, 3> m_vcoDrifts {};
    AnalogDrift m_drift {};
    NoiseSource m_noise { 0x4d4f4f47 };
    double m_noiseValue { 0.0 };
    double m_lfoPhase { 0.0 };
    double m_lfoValue { -1.0 };

    std::array<TransistorLadderFilter, 2> m_filters {};
    std::array<double, 2> m_feedback {};
    std::array<double, 2> m_feedbackDc {};
    std::array<Decimator, 2> m_decimators {};

    ContourGenerator m_filterContour {};
    ContourGenerator m_loudnessContour {};

    double m_note { 60.0 };
    double m_targetNote { 60.0 };
    double m_glideCoefficient { 0.0 };
    double m_pitchBend { 0.0 };
    double m_velocity { 1.0 };
    double m_overload { 0.0 };
    bool m_gate { false };
};

} // namespace noteahead

#endif // SYNTH_D_VOICE_HPP
