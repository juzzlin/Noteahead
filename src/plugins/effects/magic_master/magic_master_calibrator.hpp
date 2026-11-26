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

#ifndef MAGIC_MASTER_CALIBRATOR_HPP
#define MAGIC_MASTER_CALIBRATOR_HPP

#include "../../../domain/effects/master_calibration.hpp"

#include <cstdint>
#include <span>

namespace noteahead {

class MagicMaster;

//! Computes optimal EQ balance, multiband threshold references, and exact static LUFS drive for a captured song.
class MagicMasterCalibrator
{
public:
    static MasterCalibration calibrate(const MagicMaster & source, std::span<const float> interleavedStereo, uint32_t sampleRate);
};

} // namespace noteahead

#endif // MAGIC_MASTER_CALIBRATOR_HPP
