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

#ifndef TRUE_PEAK_LIMITER_HPP
#define TRUE_PEAK_LIMITER_HPP

#include "../../../domain/dsp/true_peak_limiter_core.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../domain/effects/effect_presets.hpp"

#include <string>
#include <vector>

namespace noteahead {

//! Standalone 4x true-peak brickwall lookahead limiter rack effect powered by TruePeakLimiterCore.
class TruePeakLimiter : public Effect
{
public:
    TruePeakLimiter();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;
    const EffectPresetList & factoryPresets() const override;

    void setSampleRate(double sampleRate) override;
    void setBpm(float bpm) override;
    size_t latencyFrames(double sampleRate) const override;
    bool isSettled() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    float reductionDb() const;
    float outputTruePeakDb() const;
    float outputTruePeakHoldDb() const;
    void resetPeakHold();

    static float thresholdPosition(double db);
    static float ceilingPosition(double db);
    static float releasePosition(double ms);
    static float lookaheadPosition(double ms);

private:
    void syncParameters();

    TruePeakLimiterCore m_core;
};

} // namespace noteahead

#endif // TRUE_PEAK_LIMITER_HPP
