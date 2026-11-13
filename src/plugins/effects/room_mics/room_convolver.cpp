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

#include "room_convolver.hpp"

#include "../../../domain/dsp/fft.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Below this the convolution counts as silent: -120 dB.
constexpr double silentLevel { 1.0e-6 };

} // namespace

void RoomConvolver::configure(size_t maxLength)
{
    constexpr size_t transform = BlockSize * 2;
    m_maxPartitions = std::max<size_t>(1, (maxLength + BlockSize - 1) / BlockSize);
    const auto spectrum = [] {
        return Spectrum { std::vector<double>(BlockSize + 1, 0.0), std::vector<double>(BlockSize + 1, 0.0) };
    };
    for (auto && set : m_sets) {
        for (size_t k = 0; k < KernelCount; k++) {
            set.head[k].assign(BlockSize, 0.0);
            set.partitions[k].assign(m_maxPartitions, spectrum());
        }
        set.partitionCount = 0;
    }
    for (auto && staging : m_staging) {
        staging.assign(maxLength, 0.0);
    }
    for (size_t i = 0; i < InputCount; i++) {
        m_recent[i].assign(BlockSize * 2, 0.0);
        m_blockInput[i].assign(transform, 0.0);
        m_history[i].assign(m_maxPartitions, spectrum());
    }
    for (size_t o = 0; o < OutputCount; o++) {
        m_tail[o].assign(BlockSize, 0.0);
        m_fadeTail[o].assign(BlockSize, 0.0);
    }
    m_scratchRe.assign(transform, 0.0);
    m_scratchIm.assign(transform, 0.0);
    m_mixRe.assign(BlockSize + 1, 0.0);
    m_mixIm.assign(BlockSize + 1, 0.0);
    m_frame.assign(transform, 0.0);
    m_active = 0;
    m_hasKernels = false;
    m_swapPending = false;
    m_fading = false;
    m_loading = false;
    m_stagingLength = 0;
    m_loadCursor = 0;
    reset();
}

bool RoomConvolver::isConfigured() const
{
    return m_maxPartitions > 0;
}

void RoomConvolver::reset()
{
    for (size_t i = 0; i < InputCount; i++) {
        std::ranges::fill(m_recent[i], 0.0);
        std::ranges::fill(m_blockInput[i], 0.0);
        for (auto && spectrum : m_history[i]) {
            std::ranges::fill(spectrum.re, 0.0);
            std::ranges::fill(spectrum.im, 0.0);
        }
    }
    for (size_t o = 0; o < OutputCount; o++) {
        std::ranges::fill(m_tail[o], 0.0);
        std::ranges::fill(m_fadeTail[o], 0.0);
    }
    m_recentIndex = 0;
    m_historyIndex = 0;
    m_position = 0;
    m_fading = false;
    m_blockPeak = 0.0;
    m_lastBlockPeak = 0.0;
    m_silentFrames = static_cast<size_t>(-1) / 2;
}

size_t RoomConvolver::kernelIndex(size_t input, size_t output)
{
    return input * OutputCount + output;
}

void RoomConvolver::startLoad(const std::array<const double *, KernelCount> & kernels, size_t length)
{
    if (!isConfigured()) {
        return;
    }
    m_stagingLength = std::min(length, m_staging[0].size());
    for (size_t k = 0; k < KernelCount; k++) {
        std::copy(kernels[k], kernels[k] + m_stagingLength, m_staging[k].begin());
    }
    m_loadCursor = 0;
    m_loading = true;
    // Whatever was waiting to go live is about to be written over
    m_swapPending = false;
}

bool RoomConvolver::isLoading() const
{
    return m_loading || m_swapPending;
}

void RoomConvolver::advanceLoad(size_t partitions)
{
    // The set being loaded is the one a crossfade is still reading
    if (!m_loading || m_fading) {
        return;
    }
    auto && set = m_sets[1 - m_active];
    const size_t blocks = (m_stagingLength + BlockSize - 1) / BlockSize;
    const size_t partitionCount = std::min(m_maxPartitions, blocks > 0 ? blocks - 1 : 0);
    const size_t perKernel = partitionCount + 1;
    const size_t end = std::min(KernelCount * perKernel, m_loadCursor + partitions);
    for (; m_loadCursor < end; m_loadCursor++) {
        const size_t k = m_loadCursor / perKernel;
        const size_t unit = m_loadCursor % perKernel;
        const auto & taps = m_staging[k];
        if (unit == 0) {
            // The head, reversed for the direct part's dot product
            for (size_t j = 0; j < BlockSize; j++) {
                const size_t tap = BlockSize - 1 - j;
                set.head[k][j] = tap < m_stagingLength ? taps[tap] : 0.0;
            }
            continue;
        }
        const size_t partition = unit - 1;
        const size_t first = (partition + 1) * BlockSize;
        for (size_t n = 0; n < BlockSize; n++) {
            m_frame[n] = first + n < m_stagingLength ? taps[first + n] : 0.0;
        }
        std::fill(m_frame.begin() + static_cast<std::ptrdiff_t>(BlockSize), m_frame.end(), 0.0);
        auto && spectrum = set.partitions[k][partition];
        Fft::forwardReal(m_frame.data(), spectrum.re.data(), spectrum.im.data(), static_cast<int>(BlockSize * 2));
    }
    if (m_loadCursor >= KernelCount * perKernel) {
        set.partitionCount = partitionCount;
        m_loading = false;
        m_swapPending = true;
    }
}

void RoomConvolver::finishLoad()
{
    advanceLoad(static_cast<size_t>(-1));
}

bool RoomConvolver::hasKernels() const
{
    return m_hasKernels;
}

bool RoomConvolver::isRinging() const
{
    const size_t length = (m_sets[m_active].partitionCount + 1) * BlockSize;
    return m_lastBlockPeak > silentLevel || m_silentFrames < length;
}

double RoomConvolver::directPart(const KernelSet & set, size_t output) const
{
    double sum = 0.0;
    for (size_t input = 0; input < InputCount; input++) {
        const double * recent = m_recent[input].data() + m_recentIndex + 1;
        const double * head = set.head[kernelIndex(input, output)].data();
        for (size_t j = 0; j < BlockSize; j++) {
            sum += head[j] * recent[j];
        }
    }
    return sum;
}

void RoomConvolver::process(double & left, double & right)
{
    if (!isConfigured()) {
        return;
    }
    const std::array<double, InputCount> input { left, right };
    for (size_t i = 0; i < InputCount; i++) {
        // Twice, so the last BlockSize samples are always in one piece
        m_recent[i][m_recentIndex] = input[i];
        m_recent[i][m_recentIndex + BlockSize] = input[i];
        m_blockInput[i][BlockSize + m_position] = input[i];
    }
    m_silentFrames = std::max(std::abs(left), std::abs(right)) > silentLevel ? 0 : m_silentFrames + 1;
    // The newest sample sits at m_recentIndex + BlockSize, the window starts one after the index
    std::array<double, OutputCount> output {};
    const auto & active = m_sets[m_active];
    for (size_t o = 0; o < OutputCount; o++) {
        output[o] = directPart(active, o) + m_tail[o][m_position];
    }
    if (m_fading) {
        const auto & previous = m_sets[1 - m_active];
        const double t = static_cast<double>(m_position + 1) / static_cast<double>(BlockSize);
        for (size_t o = 0; o < OutputCount; o++) {
            const double old = directPart(previous, o) + m_fadeTail[o][m_position];
            output[o] = old * (1.0 - t) + output[o] * t;
        }
    }
    m_recentIndex = (m_recentIndex + 1) % BlockSize;
    left = output[0];
    right = output[1];
    m_blockPeak = std::max({ m_blockPeak, std::abs(left), std::abs(right) });
    if (++m_position == BlockSize) {
        m_position = 0;
        processBlock();
    }
}

void RoomConvolver::convolve(const KernelSet & set, size_t output, std::vector<double> & out)
{
    std::ranges::fill(m_mixRe, 0.0);
    std::ranges::fill(m_mixIm, 0.0);
    for (size_t input = 0; input < InputCount; input++) {
        const auto & partitions = set.partitions[kernelIndex(input, output)];
        for (size_t q = 0; q < set.partitionCount; q++) {
            const auto & x = m_history[input][(m_historyIndex + m_maxPartitions - q) % m_maxPartitions];
            const auto & h = partitions[q];
            for (size_t bin = 0; bin <= BlockSize; bin++) {
                m_mixRe[bin] += x.re[bin] * h.re[bin] - x.im[bin] * h.im[bin];
                m_mixIm[bin] += x.re[bin] * h.im[bin] + x.im[bin] * h.re[bin];
            }
        }
    }
    constexpr size_t transform = BlockSize * 2;
    for (size_t bin = 0; bin <= BlockSize; bin++) {
        m_scratchRe[bin] = m_mixRe[bin];
        m_scratchIm[bin] = m_mixIm[bin];
    }
    for (size_t bin = 1; bin < BlockSize; bin++) {
        m_scratchRe[transform - bin] = m_mixRe[bin];
        m_scratchIm[transform - bin] = -m_mixIm[bin];
    }
    Fft::inverse(m_scratchRe.data(), m_scratchIm.data(), static_cast<int>(transform));
    // Overlap-save: the second half is the part that did not wrap around
    for (size_t n = 0; n < BlockSize; n++) {
        out[n] = m_scratchRe[BlockSize + n];
    }
}

void RoomConvolver::processBlock()
{
    m_lastBlockPeak = m_blockPeak;
    m_blockPeak = 0.0;
    m_fading = false;
    if (m_swapPending) {
        m_active = 1 - m_active;
        // Faded in from what was playing, or straight in when nothing was
        m_fading = m_hasKernels;
        m_hasKernels = true;
        m_swapPending = false;
    }

    // What the partitions after the first add to the next block, from the inputs up to this one
    for (size_t i = 0; i < InputCount; i++) {
        auto && slot = m_history[i][m_historyIndex];
        Fft::forwardReal(m_blockInput[i].data(), slot.re.data(), slot.im.data(), static_cast<int>(BlockSize * 2));
        std::copy(m_blockInput[i].begin() + static_cast<std::ptrdiff_t>(BlockSize), m_blockInput[i].end(), m_blockInput[i].begin());
    }
    for (size_t o = 0; o < OutputCount; o++) {
        convolve(m_sets[m_active], o, m_tail[o]);
        if (m_fading) {
            convolve(m_sets[1 - m_active], o, m_fadeTail[o]);
        }
    }
    m_historyIndex = (m_historyIndex + 1) % m_maxPartitions;
}

} // namespace noteahead
