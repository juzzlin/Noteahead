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

#ifndef ROOM_CONVOLVER_HPP
#define ROOM_CONVOLVER_HPP

#include <array>
#include <cstddef>
#include <vector>

namespace noteahead {

//! Convolves two inputs with four impulse responses, one from each input to each output, with no
//! latency.
//!
//! The first BlockSize taps of each response are applied directly, sample by sample, and the rest
//! as a uniformly partitioned convolution of BlockSize partitions. The partitioned part works a
//! block behind, which is exactly the delay of the taps it covers, so the two join without a gap.
//!
//! New responses are loaded a partition at a time by advanceLoad(), each one a transform, and
//! crossfaded in over one block once all are in, so that moving a mic never clicks.
class RoomConvolver
{
public:
    static constexpr size_t BlockSize { 128 };
    static constexpr size_t InputCount { 2 };
    static constexpr size_t OutputCount { 2 };
    static constexpr size_t KernelCount { InputCount * OutputCount };

    //! Sizes everything for responses of up to @p maxLength samples, which allocates.
    void configure(size_t maxLength);
    bool isConfigured() const;

    //! Clears the signal held, keeping the responses.
    void reset();

    static size_t kernelIndex(size_t input, size_t output);

    //! Starts loading new responses, @p length samples of each, copying them in. Abandons any load
    //! still running.
    void startLoad(const std::array<const double *, KernelCount> & kernels, size_t length);
    bool isLoading() const;
    //! Transforms up to @p partitions partitions. The responses go live at the next block once
    //! all are done.
    void advanceLoad(size_t partitions);
    void finishLoad();
    //! Whether any responses have been loaded yet.
    bool hasKernels() const;
    //! Whether there is still signal in the convolution.
    bool isRinging() const;

    void process(double & left, double & right);

private:
    struct Spectrum
    {
        std::vector<double> re;
        std::vector<double> im;
    };

    struct KernelSet
    {
        //! The first BlockSize taps, reversed, for the direct part.
        std::array<std::vector<double>, KernelCount> head;
        //! The partitions after the first, each the spectrum of BlockSize taps.
        std::array<std::vector<Spectrum>, KernelCount> partitions;
        size_t partitionCount { 0 };
    };

    void processBlock();
    void convolve(const KernelSet & set, size_t output, std::vector<double> & out);
    double directPart(const KernelSet & set, size_t output) const;

    size_t m_maxPartitions { 0 };

    std::array<KernelSet, 2> m_sets;
    size_t m_active { 0 };
    bool m_hasKernels { false };
    //! The new set is loaded and goes live at the next block boundary.
    bool m_swapPending { false };
    //! Crossfading from the other set over the current block.
    bool m_fading { false };

    // The load under way
    bool m_loading { false };
    std::array<std::vector<double>, KernelCount> m_staging;
    size_t m_stagingLength { 0 };
    size_t m_loadCursor { 0 };

    // The input: the last BlockSize samples twice over for the direct part, and the previous and
    // current block for the partitioned part's overlap-save
    std::array<std::vector<double>, InputCount> m_recent;
    size_t m_recentIndex { 0 };
    std::array<std::vector<double>, InputCount> m_blockInput;
    //! Spectra of the latest input blocks, newest at m_historyIndex.
    std::array<std::vector<Spectrum>, InputCount> m_history;
    size_t m_historyIndex { 0 };
    size_t m_position { 0 };

    //! The partitioned part's output for the current block, and the fading set's.
    std::array<std::vector<double>, OutputCount> m_tail;
    std::array<std::vector<double>, OutputCount> m_fadeTail;

    //! The loudest output or input in the block under way and the one before, and how long the
    //! input has been silent: a response can still be on its way long after the output went quiet.
    double m_blockPeak { 0.0 };
    double m_lastBlockPeak { 0.0 };
    size_t m_silentFrames { static_cast<size_t>(-1) / 2 };

    // Scratch
    std::vector<double> m_scratchRe;
    std::vector<double> m_scratchIm;
    std::vector<double> m_mixRe;
    std::vector<double> m_mixIm;
    std::vector<double> m_frame;
};

} // namespace noteahead

#endif // ROOM_CONVOLVER_HPP
