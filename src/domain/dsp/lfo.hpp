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

#ifndef LFO_HPP
#define LFO_HPP

#include "dsp_component.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace noteahead {

class Lfo : public DspComponent
{
public:
    enum class Waveform
    {
        Saw = 0,
        Sine = 3,
        Square = 2,
        Triangle = 1,
        Random = 4,
    };

    static std::vector<std::string> waveformNames();

    enum class Mode
    {
        BPM = 1,
        Normal = 0,
        OneShot = 2,
    };

    void setSampleRate(double sampleRate) override;
    void setFrequency(double frequency);
    void setFrequency(double bpm, double syncRate);
    void setWaveform(Waveform waveform);
    void setMode(Mode mode);

    //! Time from the trigger before the LFO starts moving, in seconds. Zero, the default, engages it
    //! on the note itself, which is what this LFO has always done.
    void setDelayTime(double seconds);
    //! Time the output takes to reach full depth once the delay has elapsed, in seconds. Zero, the
    //! default, hands over the full depth at once.
    void setFadeTime(double seconds);

    void setPhase(double phase);
    double phase() const;
    //! Restarts the shape and the delay/fade envelope, leaving the random sequence where it is.
    void trigger();
    double nextSample();
    void reset();

private:
    double m_frequency { 1.0 };

    Waveform m_waveform { Waveform::Triangle };
    Mode m_mode { Mode::Normal };
    double m_phase { 0.0 };
    double m_phaseStep { 0.0 };
    bool m_oneShotActive { true };
    //! Value a finished one-shot parks on: the level its shape ends at.
    double m_oneShotHold { 0.0 };

    double m_delayTime { 0.0 };
    double m_fadeTime { 0.0 };
    double m_delaySamples { 0.0 };
    double m_fadeSamples { 0.0 };
    //! Samples since the last trigger, which is what both the delay and the fade are measured
    //! against. Counted rather than turned into a deadline at trigger time: the devices push the
    //! times once per block, so a note arriving before the first push would otherwise be armed with
    //! a delay of zero and never wait at all.
    uint64_t m_elapsedSamples { 0 };

    double m_randomValue { 0.0 };
    std::mt19937 m_rng { 0 };
    std::uniform_real_distribution<double> m_dist { -1.0, 1.0 };

    void updatePhaseStep();
    void updateEnvelopeTimes();

    //! Depth in effect \a samplesSinceDelay samples after the delay expired, 0..1.
    double fadeLevel(double samplesSinceDelay) const;

    double waveformValue(double phase) const;
};

} // namespace noteahead

#endif // LFO_HPP
