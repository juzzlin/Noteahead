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

#ifndef LOUDNESS_ANALYZER_HPP
#define LOUDNESS_ANALYZER_HPP

#include "dbtp_meter.hpp"

#include <vector>

namespace noteahead {

class LoudnessAnalyzer
{
public:
    struct Result
    {
        float integratedLoudness = -70.0f;
        float loudnessRange = 0.0f;
        float truePeak = -70.0f;
        float threshold = -70.0f;
    };

    explicit LoudnessAnalyzer(double sampleRate);
    ~LoudnessAnalyzer() = default;

    void process(const float * data, size_t numSamples);
    Result calculate();

private:
    void updateCoefficients();
    double applyKWeightL(double x);
    double applyKWeightR(double x);

    double m_sampleRate = 48000.0;
    size_t m_blockSize = 4800;
    size_t m_blockSamples = 0;
    double m_blockPowerSum = 0.0;
    std::vector<double> m_blockPowers {};

    // K-weighting biquad coefficients
    double m_b0s1 = 1.0, m_b1s1 = 0.0, m_b2s1 = 0.0;
    double m_a1s1 = 0.0, m_a2s1 = 0.0;
    double m_b0s2 = 1.0, m_b1s2 = 0.0, m_b2s2 = 0.0;
    double m_a1s2 = 0.0, m_a2s2 = 0.0;

    // Filter states
    double m_z1s1L = 0.0, m_z2s1L = 0.0, m_z1s2L = 0.0, m_z2s2L = 0.0;
    double m_z1s1R = 0.0, m_z2s1R = 0.0, m_z1s2R = 0.0, m_z2s2R = 0.0;

    DbTpMeter m_dbtpMeter {};
    float m_maxDbTp = -70.0f;
};

} // namespace noteahead

#endif // LOUDNESS_ANALYZER_HPP
