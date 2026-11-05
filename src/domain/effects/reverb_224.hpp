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

#ifndef REVERB_224_HPP
#define REVERB_224_HPP

#include "../dsp/one_pole_filter.hpp"
#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace noteahead {

//! A late-seventies digital reverb after the Lexicon 224, the reverb of the Blade Runner soundtrack.
//!
//! What is known of the machine is modelled; nothing is fitted to it, as there are no recordings
//! to fit to. The network runs at the machine's own rate, one pass of its hundred-step program per
//! sample (34.13 kHz), so it sounds the same whatever the host's rate. In front of it and behind it
//! are the machine's converters: an eighth-order band limit at about 8.1 kHz, and a 12-bit
//! converter behind a gain ranger, which keeps quiet signals in range in 6 dB steps and makes the
//! noise follow the signal. The tank is Dattorro's figure eight, which was drawn from the 224's
//! lineage: four input diffusers and eight delay elements, two of them slowly modulated.
//!
//! As on the machine, the decay is set in two bands, bass and mid, either side of a crossover, and
//! a treble cutoff above which the tail dies faster still. Depth balances the early reflections
//! against the tank on the programs that have them; the plates have none.
class Reverb224 : public Effect
{
public:
    Reverb224();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    //! Unsettled while the tail is still sounding: a silent device stops being processed, and a
    //! tail frozen part way would come back on the next note.
    bool isSettled() const override;

    const EffectPresetList & factoryPresets() const override;

    //! Persisted by index: append only.
    enum class Program
    {
        SmallHallA,
        SmallHallB,
        LargeHall,
        VocalPlate,
        Chamber,
        PercussionPlate,
        ConstantDensityPlate,
        Room
    };
    static constexpr size_t ProgramCount { 8 };

    //! The machine's sample rate: one pass of its hundred-step program.
    static constexpr double InternalRate { 34130.0 };

    static constexpr size_t NumDiffusers { 4 };
    static constexpr size_t NumEarlyTaps { 6 };
    static constexpr size_t NumBandwidthSections { 4 };

    // Parameter conversions, shared with the presets, the dialog and the tests
    static double decaySeconds(float position);
    static float decayPosition(double seconds);
    static double crossoverHz(float position);
    static float crossoverPosition(double hz);
    static double trebleHz(float position);
    static float treblePosition(double hz);
    static double lowCutHz(float position);
    static float lowCutPosition(double hz);
    static double preDelayMs(float position);
    static float preDelayPosition(double ms);

    //! The 12-bit converter behind its gain ranger, with full scale at 1.0.
    static double convert(double value);

    //! Whether a program has early reflections, which is what Depth balances the tank against.
    static bool hasEarlyReflections(Program program);

private:
    //! A delay read at a fractional length through an all-pass interpolator.
    struct Line
    {
        std::vector<double> buffer;
        uint32_t writePos { 0 };
        double interpolatorState { 0.0 };
        double read(double delaySamples);
        double tap(uint32_t delaySamples) const;
        void write(double sample);
        void reset();
    };

    //! A Schroeder all-pass around a Line, so it can be both modulated and tapped.
    struct Allpass
    {
        Line line;
        double length { 1.0 };
        double coeff { 0.0 };
        double process(double input, double modulation = 0.0);
        void reset();
    };

    //! One half of the figure eight: a modulated all-pass, a delay, the decay filters, a second
    //! all-pass and a second delay, whose output crosses into the other half.
    struct Half
    {
        Allpass modulated;
        Line firstDelay;
        uint32_t firstLength { 1 };
        OnePoleFilter treble;
        //! Bass decay against mid: the mid gain is applied flat, this takes the bass to its own.
        SvfFilter bassShelf;
        Allpass second;
        Line secondDelay;
        uint32_t secondLength { 1 };
        double midGain { 0.0 };
        double lfoPhase { 0.0 };
        double lfoIncrement { 0.0 };
        //! What the second delay put out last, for the other half to take in.
        double output { 0.0 };
        void reset();
    };

    void syncParameters();
    void updateHostRate();
    void updateLowCut();
    void renderFrame(double & left, double & right);
    //! One sample of the machine, at InternalRate.
    void tick(double input);
    double renderEarly(const std::array<uint32_t, NumEarlyTaps> & taps, const std::array<double, NumEarlyTaps> & gains) const;

    Program m_program { Program::SmallHallA };
    bool m_converters { true };
    double m_earlyGain { 0.0 };
    double m_tankGain { 1.0 };
    double m_excursionSamples { 0.0 };

    //! The machine's input, pre-delayed and tapped for the early reflections.
    Line m_input;
    uint32_t m_preDelaySamples { 0 };
    std::array<Allpass, NumDiffusers> m_diffusers;
    std::array<Half, 2> m_halves;
    std::array<uint32_t, NumEarlyTaps> m_earlyTapsLeft {};
    std::array<uint32_t, NumEarlyTaps> m_earlyTapsRight {};
    std::array<double, NumEarlyTaps> m_earlyGainsLeft {};
    std::array<double, NumEarlyTaps> m_earlyGainsRight {};
    //! The output taps into the tank, in samples, scaled with the program.
    std::array<uint32_t, 14> m_outputTaps {};

    //! A high-pass on the input, at the host's rate: Bass Decay sets how long the lows ring, this
    //! how much of them gets in at all.
    SvfFilter m_lowCut;
    double m_lowCutHz { 0.0 };
    bool m_lowCutActive { false };

    // The host side: the band limit, and the resampling to and from the machine's rate
    std::array<SvfFilter, NumBandwidthSections> m_inputFilters;
    std::array<SvfFilter, NumBandwidthSections> m_outputFiltersLeft;
    std::array<SvfFilter, NumBandwidthSections> m_outputFiltersRight;
    //! The last four host samples of the input, oldest first.
    std::array<double, 4> m_inputHistory {};
    //! The last four machine samples of the output, newest first.
    std::array<double, 4> m_outputHistoryLeft {};
    std::array<double, 4> m_outputHistoryRight {};
    //! Where the next machine sample falls, between the second and the third of the input history.
    double m_inputPhase { 0.0 };
    //! Host samples per machine sample.
    double m_hostPerInternal { 1.0 };

    //! The loudest the tail reached in the last block, for isSettled().
    double m_tailPeak { 0.0 };

    uint32_t m_lastSampleRate { 0 };
    bool m_shouldSyncParameters { true };
};

} // namespace noteahead

#endif // REVERB_224_HPP
