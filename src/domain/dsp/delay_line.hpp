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

#ifndef DELAY_LINE_HPP
#define DELAY_LINE_HPP

#include "dsp_component.hpp"

#include <cstddef>
#include <vector>

namespace noteahead {

// Integer-sample ring buffer. Read returns the sample written 'delay' writes ago.
// Convention: read() is called before write() each step so that delay==capacity
// gives exactly capacity samples of latency.
class DelayLine : public DspComponent
{
public:
    void setMaxDelay(size_t maxSamples);
    void setDelay(size_t samples);
    void write(double sample);
    double read() const;
    void reset();

    size_t delay() const;

private:
    std::vector<double> m_buffer;
    size_t m_writePos { 0 };
    size_t m_delay { 0 };
};

} // namespace noteahead

#endif // DELAY_LINE_HPP
