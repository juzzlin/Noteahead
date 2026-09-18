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

#ifndef LUFS_METER_HPP
#define LUFS_METER_HPP

#include "../effects/effect.hpp"
#include "loudness_meter.hpp"

namespace noteahead {

//! BS.1770-4 loudness metering as a rack effect: the measurement itself is LoudnessMeter, which a
//! device's own output tap owns directly. This is the version the user patches into a slot, and it
//! passes the signal through untouched.
class LufsMeter : public Effect
{
public:
    LufsMeter();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void setSampleRate(double sampleRate) override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

    float momentaryLufs() const;
    float shortTermLufs() const;
    //! Gated integrated loudness per ITU-R BS.1770-4, over everything measured since the last reset.
    float integratedLufs() const;

    //! Clear the meter from another thread. The readings blank immediately; the accumulated state is
    //! dropped by the audio thread at the next sample, so no state is touched from under it.
    void requestReset();

private:
    LoudnessMeter m_meter;
};

} // namespace noteahead

#endif // LUFS_METER_HPP
