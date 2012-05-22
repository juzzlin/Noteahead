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

#ifndef RTA_HPP
#define RTA_HPP

#include "../effects/effect.hpp"

#include "rta_core.hpp"

#include <atomic>
#include <cstdint>
#include <utility>
#include <vector>

namespace noteahead {

//! Real-time spectrum analyzer, passthrough insert effect. The analysis itself is RtaCore's.
class Rta : public Effect
{
public:
    Rta();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;

    void reset() override;
    void sync() override;

    //! Never settled while the analysis is on, for the same reason the Stereo Field Meter is not:
    //! the engine skips a quiet device and with it the insert rack, and the spectrum would then
    //! hang at the last block that had anything in it instead of falling away.
    bool isSettled() const override;

    void setAnalysisEnabled(bool enabled);

    std::vector<float> bandMagnitudesDb() const;
    std::vector<std::pair<float, float>> bandLogPositions() const;
    uint32_t layoutGeneration() const;

private:
    void syncParameters();

    RtaCore m_core;

    std::atomic<bool> m_analysisEnabled { false };
    bool m_shouldSync = false;
};

} // namespace noteahead

#endif // RTA_HPP
