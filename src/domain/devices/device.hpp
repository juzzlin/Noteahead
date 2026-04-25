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

#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace noteahead {

class Device
{
public:
    virtual ~Device() = default;

    virtual std::string name() const = 0;

    virtual void processMidiNoteOn(uint8_t note, uint8_t velocity) = 0;
    virtual void processMidiNoteOff(uint8_t note) = 0;
    virtual void processMidiCc(uint8_t controller, uint8_t value) = 0;
    virtual void processMidiAllNotesOff() = 0;

    //! Process audio buffer. output is interleaved stereo.
    virtual void processAudio(float * output, uint32_t nFrames, uint32_t sampleRate) = 0;
};

} // namespace noteahead

#endif // DEVICE_HPP
