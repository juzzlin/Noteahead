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

#include "audio_scope.hpp"

namespace noteahead {

AudioScope::AudioScope()
  : m_ringL(ringSize, 0.0f)
  , m_ringR(ringSize, 0.0f)
{
}

void AudioScope::setActive(bool active)
{
    m_active.store(active);
}

bool AudioScope::active() const
{
    return m_active.load();
}

void AudioScope::write(const double * interleavedStereo, uint32_t frameCount, uint32_t sampleRate)
{
    if (!m_active.load()) {
        return;
    }

    m_sampleRate.store(sampleRate);

    const std::lock_guard<std::mutex> lock { m_mutex };
    for (uint32_t i = 0; i < frameCount; i++) {
        m_ringL[m_writePos] = static_cast<float>(interleavedStereo[i * 2]);
        m_ringR[m_writePos] = static_cast<float>(interleavedStereo[i * 2 + 1]);
        m_writePos = (m_writePos + 1) % ringSize;
    }
}

AudioScope::Snapshot AudioScope::snapshot(size_t maxPoints) const
{
    // Reorder both rings into chronological linear buffers ending at the newest sample.
    std::vector<float> linearL(ringSize, 0.0f);
    std::vector<float> linearR(ringSize, 0.0f);
    {
        const std::lock_guard<std::mutex> lock { m_mutex };
        for (size_t j = 0; j < ringSize; j++) {
            const size_t index = (m_writePos + j) % ringSize;
            linearL[j] = m_ringL[index];
            linearR[j] = m_ringR[index];
        }
    }

    // Align the display window to the first rising zero-crossing of the left channel so the trace
    // is stable across frames; both channels share the same start to stay phase-aligned.
    const size_t searchEnd = ringSize - displayLength;
    size_t start = 0;
    for (size_t i = 0; i < searchEnd; i++) {
        if (linearL[i] <= 0.0f && linearL[i + 1] > 0.0f) {
            start = i;
            break;
        }
    }

    // Decimate each channel's window to at most maxPoints samples.
    const size_t count = (maxPoints > 0 && maxPoints < displayLength) ? maxPoints : displayLength;
    Snapshot result;
    result.left.reserve(count);
    result.right.reserve(count);
    for (size_t k = 0; k < count; k++) {
        const size_t index = start + (k * displayLength) / count;
        result.left.push_back(linearL[index]);
        result.right.push_back(linearR[index]);
    }
    return result;
}

uint32_t AudioScope::sampleRate() const
{
    return m_sampleRate.load();
}

} // namespace noteahead
