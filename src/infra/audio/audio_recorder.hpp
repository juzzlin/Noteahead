// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef AUDIO_RECORDER_HPP
#define AUDIO_RECORDER_HPP

#include <string>

namespace noteahead {

class AudioRecorder
{
public:
    explicit AudioRecorder();
    virtual ~AudioRecorder();

    virtual void start(const std::string & fileName);
    virtual void stop();
};

} // namespace noteahead

#endif // AUDIO_RECORDER_HPP
