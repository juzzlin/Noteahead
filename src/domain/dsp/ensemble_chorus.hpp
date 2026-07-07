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

#ifndef ENSEMBLE_CHORUS_HPP
#define ENSEMBLE_CHORUS_HPP

#include "dsp_component.hpp"
#include "lfo.hpp"

#include <vector>

namespace noteahead {

class EnsembleChorus : public DspComponent
{
public:
    EnsembleChorus();

    void setSampleRate(double sampleRate) override;
    void process(double & left, double & right);
    void reset();

    void setEnabled(bool enabled);
    bool enabled() const;

    void setMode(int mode); // 0 = Chorus I, 1 = Chorus II, 2 = Chorus I + II
    int mode() const;

private:
    void updateBuffers();

    bool m_enabled { true };
    int m_mode { 0 };

    std::vector<double> m_bufferL;
    std::vector<double> m_bufferR;
    uint32_t m_writePos { 0 };

    Lfo m_slowLfo1;
    Lfo m_slowLfo2;
    Lfo m_slowLfo3;

    Lfo m_fastLfo1;
    Lfo m_fastLfo2;
    Lfo m_fastLfo3;
};

} // namespace noteahead

#endif // ENSEMBLE_CHORUS_HPP
