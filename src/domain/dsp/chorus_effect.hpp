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

#ifndef CHORUS_EFFECT_HPP
#define CHORUS_EFFECT_HPP

#include "../devices/effect.hpp"
#include "cascaded_svf.hpp"
#include "lfo.hpp"

#include <vector>

namespace noteahead {

class ChorusEffect : public Effect
{
public:
    ChorusEffect();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void reset() override;
    void sync() override;

    void setRate(double rate);
    double rate() const;

    void setDepth(double depth);
    double depth() const;

    void setDelay(double ms);
    double delay() const;

    void setMix(double mix);
    double mix() const;

    void setWidth(double width);
    double width() const;

    void setLpfCutoff(double cutoff);
    double lpfCutoff() const;

    void setHpfCutoff(double cutoff);
    double hpfCutoff() const;

private:
    void syncParameters();
    void updateBuffers();
    void updateFilters();

    double m_rate { 1.0 };
    double m_depth { 0.5 };
    double m_delayMs { 20.0 };
    double m_mix { 0.5 };
    double m_width { 1.0 };
    double m_lpfCutoff { 1.0 };
    double m_hpfCutoff { 0.0 };

    bool m_shouldSyncParameters { false };
    bool m_shouldUpdateBuffers { false };

    std::vector<double> m_bufferL;
    std::vector<double> m_bufferR;
    uint32_t m_writePos { 0 };
    uint32_t m_lastSampleRate { 0 };

    Lfo m_lfoL;
    Lfo m_lfoR;

    CascadedSvf m_hpfL;
    CascadedSvf m_hpfR;
    CascadedSvf m_lpfL;
    CascadedSvf m_lpfR;
};

} // namespace noteahead

#endif // CHORUS_EFFECT_HPP
