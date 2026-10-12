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

#ifndef AMBIENCE_HPP
#define AMBIENCE_HPP

#include "../dsp/one_pole_filter.hpp"
#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace noteahead {

//! A short, dense space to glue a mix with, rather than a tail to hear: what a studio ambience
//! program does under a whole mix at a low level.
//!
//! Three stages, each there for what the plain reverbs get wrong at short settings. Input diffusion
//! smears a transient before anything reflects it, so a snare comes back as a burst of noise
//! rather than as a slap. A dense early field of irregular reflections gives the room its size and
//! shape, and the ear most of its sense of space. A short late tail joins the two up: an eight-line
//! feedback network whose lines drift slowly in length, each at its own rate, which is what stops a
//! decay this short from ringing at the network's own resonances.
//!
//! The wet signal has its own low and high cut, so the space adds air without adding mud, and the
//! dry is left whole: Mix adds the space on top, which also makes it a send amount on a send bus.
class Ambience : public Effect
{
public:
    Ambience();

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

    static constexpr size_t NumTaps { 24 };
    static constexpr size_t NumDiffusers { 4 };
    static constexpr size_t NumLines { 8 };

    // Parameter conversions, shared with the presets, the dialog and the tests
    static double decaySeconds(float position);
    static float decayPosition(double seconds);
    static double preDelayMs(float position);
    static float preDelayPosition(double ms);
    static double modRateHz(float position);
    static float modRatePosition(double hz);
    static double lowCutHz(float position);
    static float lowCutPosition(double hz);
    static double highCutHz(float position);
    static float highCutPosition(double hz);

private:
    struct Allpass
    {
        std::vector<double> buffer;
        uint32_t writePos { 0 };
        double coeff { 0.0 };
        double process(double input);
        void reset();
    };

    struct Line
    {
        std::vector<double> buffer;
        uint32_t writePos { 0 };
        double lengthSamples { 0.0 };
        double gain { 0.0 };
        double lfoPhase { 0.0 };
        double lfoIncrement { 0.0 };
        OnePoleFilter damping;
        //! The all-pass interpolator's last output.
        double interpolatorState { 0.0 };
        double read(double delaySamples);
        void write(double sample);
        void reset();
    };

    struct Channel
    {
        //! The pre-delayed, diffused signal the early taps read from.
        std::vector<double> earlyBuffer;
        uint32_t earlyWritePos { 0 };
        std::vector<double> preDelayBuffer;
        uint32_t preDelayWritePos { 0 };
        std::array<Allpass, NumDiffusers> diffusers;
        std::array<uint32_t, NumTaps> taps {};
        std::array<double, NumTaps> tapGains {};
        OnePoleFilter earlyDamping;
        SvfFilter lowCut;
        SvfFilter highCut;
        void reset();
    };

    void syncParameters();
    void updateBuffers();
    void renderFrame(double & left, double & right);
    double diffuseAndStore(Channel & channel, double input);
    double renderEarly(const Channel & channel) const;
    void updateTaps(Channel & channel, const std::array<double, NumTaps> & times, const std::array<double, NumTaps> & gains) const;

    float m_size { 0.4f };
    double m_decaySeconds { 0.8 };
    double m_preDelaySamples { 0.0 };
    double m_diffusion { 0.7 };
    double m_shape { 0.3 };
    double m_earlyGain { 0.0 };
    double m_lateGain { 0.0 };
    double m_dampingHz { 20000.0 };
    bool m_damped { false };
    double m_modRateHz { 0.5 };
    double m_modDepthSamples { 0.0 };
    bool m_lowCutEnabled { true };
    bool m_highCutEnabled { true };
    double m_width { 1.0 };

    Channel m_left;
    Channel m_right;
    std::array<Line, NumLines> m_lines;

    //! The loudest the tail reached in the last block, for isSettled().
    double m_tailPeak { 0.0 };

    uint32_t m_lastSampleRate { 0 };
    bool m_shouldSyncParameters { true };
};

} // namespace noteahead

#endif // AMBIENCE_HPP
