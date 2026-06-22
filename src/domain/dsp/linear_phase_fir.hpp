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

#ifndef LINEAR_PHASE_FIR_HPP
#define LINEAR_PHASE_FIR_HPP

#include "matched_bell_filter.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace noteahead {

//! A stereo linear-phase filter with the magnitude response of a cascade of MatchedBellFilters.
//!
//! The kernel is the zero-phase impulse response of that magnitude, sampled at kernelLength() bins
//! and centred, so the filter delays everything by exactly half its length and smears nothing in
//! time. It runs as a uniformly partitioned convolution, which adds one partition of latency on top
//! and spreads the cost evenly over the audio rather than in one transform of the whole kernel.
//!
//! Redesigning the kernel is far too much work for one audio block -- at 48 kHz it is a 32768-point
//! transform, over a millisecond on its own -- so it is done as a job advanced by advanceDesign() a
//! slice at a time, and the finished kernel is crossfaded in over one partition so that a moving
//! slider never clicks. The slices are counted in frames rather than measured in time, which keeps a
//! render the same on every run however fast the machine is.
class LinearPhaseFir
{
public:
    //! Sizes everything for @p sampleRate, which allocates, and starts from a kernel that is a pure
    //! delay. Long enough to resolve a third-octave band at 16 Hz.
    void configure(double sampleRate);
    static size_t kernelLengthFor(double sampleRate);
    static size_t latencyFramesFor(double sampleRate);
    bool isConfigured() const;

    size_t kernelLength() const;
    size_t partitionSize() const;
    //! How late the output is: half the kernel and one partition.
    size_t latencyFrames() const;

    //! Clears the signal held in the filter, keeping its kernel.
    void reset();

    //! Starts designing a kernel with the response of @p sections in series, scaled by @p gain,
    //! abandoning any design still running.
    void startDesign(const std::vector<MatchedBellFilter::SquaredMagnitude> & sections, double gain);
    bool isDesigning() const;
    //! Does as much of the design as processing @p frames of audio pays for.
    void advanceDesign(size_t frames);

    //! Filters one frame of both channels in place.
    void process(double & left, double & right);

private:
    using Spectrum = std::vector<double>;

    //! One partition's spectrum, bins 0 to partitionSize() inclusive, real and imaginary parts.
    struct Partition
    {
        Spectrum re;
        Spectrum im;
    };

    using Kernel = std::vector<Partition>;

    struct Channel
    {
        //! The current partition being filled, and the one before it, which overlap-save needs.
        std::vector<double> input;
        std::vector<double> output;
        //! Spectra of the most recent input partitions, newest at deliveryIndex.
        std::vector<Partition> history;
    };

    enum class DesignStage
    {
        Idle,
        Magnitude,
        Transform,
        Partitions
    };

    void setIdentityKernel(Kernel & kernel) const;
    bool doDesignUnit();
    void transformUnit();
    void processPartition();
    void convolve(const Kernel & kernel, const Channel & channel, Spectrum & re, Spectrum & im) const;

    size_t m_kernelLength { 0 };
    size_t m_partitionSize { 0 };
    size_t m_partitionCount { 0 };

    std::array<Kernel, 2> m_kernels;
    size_t m_activeKernel { 0 };
    bool m_crossfadePending { false };

    std::array<Channel, 2> m_channels;
    size_t m_historyIndex { 0 };
    size_t m_position { 0 };

    // Scratch for the per-partition transforms
    std::vector<double> m_scratchRe;
    std::vector<double> m_scratchIm;
    std::vector<double> m_mixRe;
    std::vector<double> m_mixIm;
    std::vector<double> m_frame;
    std::vector<double> m_previousOutput;

    // The design job
    DesignStage m_stage { DesignStage::Idle };
    std::vector<MatchedBellFilter::SquaredMagnitude> m_sections;
    double m_gain { 1.0 };
    size_t m_designCursor { 0 };
    size_t m_transformLength { 0 };
    size_t m_transformGroup { 0 };
    size_t m_transformButterfly { 0 };
    double m_designCredit { 0.0 };
    std::vector<double> m_designRe;
    std::vector<double> m_designIm;
    std::vector<double> m_twiddleRe;
    std::vector<double> m_twiddleIm;
};

} // namespace noteahead

#endif // LINEAR_PHASE_FIR_HPP
