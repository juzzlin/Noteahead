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

#ifndef SNDFILE_READER_HPP
#define SNDFILE_READER_HPP

#include "audio_file_reader.hpp"

#include <sndfile.h>

namespace noteahead {

class SndFileReader : public AudioFileReader
{
public:
    SndFileReader();
    ~SndFileReader() override;

    bool open(const std::string & filePath, Mode mode, Info & info) override;
    void close() override;

    int64_t readFloat(std::span<float> data) override;
    int64_t readDouble(std::span<double> data) override;
    int64_t readInt(std::span<int32_t> data) override;

    int64_t writeFloat(std::span<const float> data) override;
    int64_t writeInt(std::span<const int32_t> data) override;

    bool seek(int64_t frames, int whence) override;

    bool isOpen() const override;
    Info info() const override;

private:
    SNDFILE * m_sndFile = nullptr;
    SF_INFO m_sfInfo = {};
};

} // namespace noteahead

#endif // SNDFILE_READER_HPP
