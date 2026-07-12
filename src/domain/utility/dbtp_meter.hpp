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

#ifndef DBTP_METER_HPP
#define DBTP_METER_HPP

#include "../effects/effect.hpp"

#include <array>
#include <cstdint>

namespace noteahead {

// ITU-R BS.1770-4 True Peak meter using 4x Hermite interpolation.
// Exposes per-channel running peak and 2-second peak hold, both in dBTP.
class DbTpMeter : public Effect
{
public:
    DbTpMeter();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void reset() override;
    void sync() override;

    float truePeakL() const;
    float truePeakR() const;
    float truePeakHoldL() const;
    float truePeakHoldR() const;

private:
    void updateDecayParams();

    double hermiteAt(const std::array<double, 4> & buf, double t) const;
    void processChannel(double sample, double & peak, double & peakHold, int & holdCounter, std::array<double, 4> & buf);

    static float linearToDbtp(double linear);

    // 4-sample delay lines for Hermite interpolation
    std::array<double, 4> m_bufL {};
    std::array<double, 4> m_bufR {};

    // Running peaks (linear amplitude, instant attack, exponential decay)
    double m_peakL { 0.0 };
    double m_peakR { 0.0 };

    // Peak holds (linear amplitude)
    double m_peakHoldL { 0.0 };
    double m_peakHoldR { 0.0 };

    // Hold counters (samples remaining in hold phase)
    int m_holdCounterL { 0 };
    int m_holdCounterR { 0 };

    // Per-sample decay factors (recomputed on sample-rate change)
    double m_decayPerSample { 0.0 };
    double m_holdDecayPerSample { 0.0 };
    int m_holdSamples { 0 };

    float m_truePeakL { -70.0f };
    float m_truePeakR { -70.0f };
    float m_truePeakHoldL { -70.0f };
    float m_truePeakHoldR { -70.0f };

    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // DBTP_METER_HPP
