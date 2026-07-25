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

#ifndef DRIVE_HPP
#define DRIVE_HPP

#include "effect.hpp"

namespace noteahead {

//! A simple overdrive: a drive amount pushes the signal into one of a few soft/hard/fold shaping
//! curves, blended back with the dry signal via a dry/wet Mix.
class Drive : public Effect
{
public:
    enum class Mode
    {
        Soft, //!< Smooth tanh overdrive
        Hard, //!< Hard clip
        Fold, //!< Wavefolder
        Dist //!< Aggressive asymmetric distortion (electric-guitar style)
    };

    Drive();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void reset() override;
    void sync() override;

private:
    void syncParameters();
    double shape(double x) const;

    Mode m_mode { Mode::Soft };
    float m_drive { 0.5f };
    float m_mix { 1.0f };
    float m_outputDb { 0.0f };
};

} // namespace noteahead

#endif // DRIVE_HPP
