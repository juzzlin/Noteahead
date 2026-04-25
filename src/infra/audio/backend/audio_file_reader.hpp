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

#ifndef AUDIO_FILE_READER_HPP
#define AUDIO_FILE_READER_HPP

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace noteahead {

class AudioFileReader
{
public:
    virtual ~AudioFileReader() = default;

    struct Info
    {
        int64_t frames = 0;
        int samplerate = 0;
        int channels = 0;
        int format = 0;
    };

    enum class Mode {
        Read,
        Write
    };

    virtual bool open(const std::string & filePath, Mode mode, Info & info) = 0;
    virtual void close() = 0;

    virtual int64_t readFloat(std::span<float> data) = 0;
    virtual int64_t readDouble(std::span<double> data) = 0;
    virtual int64_t readInt(std::span<int32_t> data) = 0;

    virtual int64_t writeFloat(std::span<const float> data) = 0;
    virtual int64_t writeInt(std::span<const int32_t> data) = 0;

    virtual bool seek(int64_t frames, int whence) = 0;

    virtual bool isOpen() const = 0;
    virtual Info info() const = 0;
};

} // namespace noteahead

#endif // AUDIO_FILE_READER_HPP
