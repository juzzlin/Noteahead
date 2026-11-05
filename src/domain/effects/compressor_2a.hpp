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

#ifndef COMPRESSOR_2A_HPP
#define COMPRESSOR_2A_HPP

#include "../dsp/dc_blocker.hpp"
#include "../dsp/opto_compressor_core.hpp"
#include "effect.hpp"

#include <string>

namespace noteahead {

//! An optical leveling amplifier after the LA-2A: Peak Reduction, Gain and a Compress/Limit
//! switch, as on the unit's front panel, around the program-dependent opto cell modelled by
//! OptoCompressorCore. A valve output stage behind the Gain adds a little second harmonic, always,
//! as the unit does.
class Compressor2A : public Effect
{
public:
    //! Persisted by index: append only.
    enum class Mode
    {
        Compress,
        Limit
    };

    Compressor2A();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;
    bool isSettled() const override;

    const EffectPresetList & factoryPresets() const override;

    float reductionDb() const;

    // Parameter conversions, shared with the presets, the dialog and the tests
    static double gainDb(float position);
    static float gainPosition(double db);

    //! The valve output stage, at full scale 1.0.
    static double valve(double value);

private:
    void syncParameters();

    OptoCompressorCore m_core;
    DcBlocker m_dcBlockerLeft;
    DcBlocker m_dcBlockerRight;
    double m_gain { 1.0 };

    bool m_shouldSyncParameters { true };
};

} // namespace noteahead

#endif // COMPRESSOR_2A_HPP
