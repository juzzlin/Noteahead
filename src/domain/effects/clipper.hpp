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

#ifndef CLIPPER_HPP
#define CLIPPER_HPP

#include "effect.hpp"

#include <cstdint>
#include <memory>

namespace noteahead {

class Clipper : public Effect
{
public:
    enum class Mode
    {
        Hard,
        Soft
    };

    Clipper();
    ~Clipper() override;

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void process(AudioContext & context) override;
    void reset() override;
    void sync() override;

    float reductionDb() const;

private:
    void syncParameters();
    float clipSample(float sample, double thresholdLin) const;

    Mode m_mode { Mode::Soft };
    float m_thresholdDb { 0.0f };
    float m_gainDb { 0.0f };

    double m_reductionDb { 0.0 };
    double m_meterReleaseCoeff { 0.0 };
    uint32_t m_lastSampleRate { 0 };

    struct Oversampling;
    std::unique_ptr<Oversampling> m_oversampling;
};

} // namespace noteahead

#endif // CLIPPER_HPP
