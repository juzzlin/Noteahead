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

#ifndef ALL_PASS_FILTER_HPP
#define ALL_PASS_FILTER_HPP

#include "../effects/effect.hpp"

#include <array>
#include <cstdint>

namespace noteahead {

class AllPassFilter : public Effect
{
public:
    static constexpr int maxStages = 4;

    AllPassFilter();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void reset() override;
    void sync() override;

private:
    void updateCoefficients();
    void syncParameters();

    double m_b0 { 1.0 };
    double m_b1 { 0.0 };
    double m_b2 { 1.0 };

    // Per-stage delay-line state: {xn1, xn2, yn1, yn2}
    using StageState = std::array<double, 4>;
    using StateArray = std::array<StageState, maxStages>;
    StateArray m_stateL {};
    StateArray m_stateR {};

    float m_frequency { 100.0f };
    float m_q { 0.707f };
    int m_stages { 1 };

    bool m_shouldSyncParameters { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // ALL_PASS_FILTER_HPP
