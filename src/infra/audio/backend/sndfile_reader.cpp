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

#include "sndfile_reader.hpp"

namespace noteahead {

SndFileReader::SndFileReader() = default;

SndFileReader::~SndFileReader()
{
    close();
}

bool SndFileReader::open(const std::string & filePath, Mode mode, Info & info)
{
    close();

    m_sfInfo = {};
    if (mode == Mode::Write) {
        m_sfInfo.samplerate = info.samplerate;
        m_sfInfo.channels = info.channels;
        m_sfInfo.format = info.format;
    }

    m_sndFile = sf_open(filePath.c_str(), mode == Mode::Read ? SFM_READ : SFM_WRITE, &m_sfInfo);
    if (!m_sndFile) {
        return false;
    }

    info.frames = m_sfInfo.frames;
    info.samplerate = m_sfInfo.samplerate;
    info.channels = m_sfInfo.channels;
    info.format = m_sfInfo.format;

    return true;
}

void SndFileReader::close()
{
    if (m_sndFile) {
        sf_close(m_sndFile);
        m_sndFile = nullptr;
    }
}

int64_t SndFileReader::readFloat(std::span<float> data)
{
    return m_sndFile ? sf_readf_float(m_sndFile, data.data(), static_cast<sf_count_t>(data.size() / static_cast<size_t>(m_sfInfo.channels))) : 0;
}

int64_t SndFileReader::readDouble(std::span<double> data)
{
    return m_sndFile ? sf_readf_double(m_sndFile, data.data(), static_cast<sf_count_t>(data.size() / static_cast<size_t>(m_sfInfo.channels))) : 0;
}

int64_t SndFileReader::readInt(std::span<int32_t> data)
{
    return m_sndFile ? sf_readf_int(m_sndFile, data.data(), static_cast<sf_count_t>(data.size() / static_cast<size_t>(m_sfInfo.channels))) : 0;
}

int64_t SndFileReader::writeFloat(std::span<const float> data)
{
    return m_sndFile ? sf_writef_float(m_sndFile, data.data(), static_cast<sf_count_t>(data.size() / static_cast<size_t>(m_sfInfo.channels))) : 0;
}

int64_t SndFileReader::writeInt(std::span<const int32_t> data)
{
    return m_sndFile ? sf_writef_int(m_sndFile, data.data(), static_cast<sf_count_t>(data.size() / static_cast<size_t>(m_sfInfo.channels))) : 0;
}

bool SndFileReader::seek(int64_t frames, int whence)
{
    return m_sndFile ? (sf_seek(m_sndFile, static_cast<sf_count_t>(frames), whence) != -1) : false;
}

bool SndFileReader::isOpen() const
{
    return m_sndFile != nullptr;
}

SndFileReader::Info SndFileReader::info() const
{
    return {
        m_sfInfo.frames,
        m_sfInfo.samplerate,
        m_sfInfo.channels,
        m_sfInfo.format
    };
}

} // namespace noteahead
