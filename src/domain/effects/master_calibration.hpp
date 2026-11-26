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

#ifndef MASTER_CALIBRATION_HPP
#define MASTER_CALIBRATION_HPP

#include <array>
#include <cstdint>
#include <span>

namespace noteahead {

struct MasterCalibration
{
    static constexpr size_t AutoEqBandCount { 7 };

    bool valid { false };
    bool hasMeasuredSpectrum { false };
    std::array<double, AutoEqBandCount> measuredBandRelDb { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    std::array<double, AutoEqBandCount> autoEqBandDb { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    std::array<double, 3> multibandRefRmsDb { -18.0, -18.0, -20.0 };
    double measuredInputLufs { -70.0 };
};

class MasterCalibrationConsumer
{
public:
    virtual ~MasterCalibrationConsumer() = default;

    virtual void setCalibration(const MasterCalibration & calibration) = 0;
    virtual void clearCalibration() = 0;
    virtual MasterCalibration calibration() const = 0;
    virtual void setCalibrationBypass(bool bypass) = 0;
    virtual double thresholdDb() const = 0;
    virtual double ceilingDb() const = 0;
    virtual MasterCalibration calibrateFromStereo(std::span<const float> interleavedStereo, uint32_t sampleRate) const = 0;
};

} // namespace noteahead

#endif // MASTER_CALIBRATION_HPP
