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

#ifndef MONITOR_HPP
#define MONITOR_HPP

#include "effect.hpp"

namespace noteahead {

//! Monitoring-only fold of the stereo image, for checking what a mix keeps when it stops being
//! heard in stereo.
//!
//! Mono sums at half, so material that is already the same on both sides comes through at the level
//! it went in at: what changes is what the two sides do to each other, not the gain. Side is the
//! difference at the same half, which is exactly the part that mono summing throws away -- silence
//! there means nothing is lost, and anything loud is about to be.
//!
//! Deliberately has no Mix, no Solo and no output trim. Half of a mono check is not a thing worth
//! hearing, and every control that could scale the comparison would make it a worse reference.
class Monitor : public Effect
{
public:
    Monitor();

    enum class Mode
    {
        Stereo,
        Mono,
        Left,
        Right,
        Side
    };

    static std::string typeIdString();

    std::string type() const override;
    std::string typeId() const override;

    void sync() override;

    Mode mode() const;

protected:
    void processSample(double & left, double & right) override;

    //! Where the export is kept clean. The mode is a property of the listening, so a block that is
    //! being written to a file rather than heard passes through whatever this is set to.
    void processBlock(AudioContext & context) override;

private:
    Mode m_mode { Mode::Stereo };
};

} // namespace noteahead

#endif // MONITOR_HPP
