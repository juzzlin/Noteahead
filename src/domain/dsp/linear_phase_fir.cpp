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

#include "linear_phase_fir.hpp"

#include "fft.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Kernel length at 48 kHz and below. A third-octave band at 16 Hz rings for longer than any
//! shorter kernel holds, and truncating it costs the bottom bands most of a decibel even on a plain
//! boost; at this length a realistic curve comes out within a few hundredths. Doubled for each
//! doubling of the rate, so it holds the same time and so the same resolution at any of them.
constexpr size_t BaseKernelLength { 32768 };
//! Partitions per kernel: fewer makes each block's transform larger, more makes the sum of the
//! partitions longer. Thirty-two leaves both modest.
constexpr size_t PartitionCount { 32 };

// Work for one design slice: about the same time each, a few tens of microseconds.
constexpr size_t MagnitudeBinsPerUnit { 256 };
constexpr size_t ButterfliesPerUnit { 4096 };
//! Frames of audio that pay for one slice. A rebuild then takes the same wall time at any buffer
//! size -- around a tenth of a second at 48 kHz -- and costs a few percent of each block while it runs.
constexpr double FramesPerUnit { 32.0 };

} // namespace

size_t LinearPhaseFir::kernelLengthFor(double sampleRate)
{
    size_t kernelLength = BaseKernelLength;
    while (static_cast<double>(kernelLength) < static_cast<double>(BaseKernelLength) * sampleRate / 50000.0) {
        kernelLength *= 2;
    }
    return kernelLength;
}

size_t LinearPhaseFir::latencyFramesFor(double sampleRate)
{
    const size_t kernelLength = kernelLengthFor(sampleRate);
    return kernelLength / 2 + kernelLength / PartitionCount;
}

void LinearPhaseFir::configure(double sampleRate)
{
    const size_t kernelLength = kernelLengthFor(sampleRate);
    if (kernelLength == m_kernelLength) {
        return;
    }

    m_kernelLength = kernelLength;
    m_partitionCount = PartitionCount;
    m_partitionSize = kernelLength / PartitionCount;
    const size_t bins = m_partitionSize + 1;

    for (auto && kernel : m_kernels) {
        kernel.assign(m_partitionCount, Partition { Spectrum(bins, 0.0), Spectrum(bins, 0.0) });
        setIdentityKernel(kernel);
    }
    m_activeKernel = 0;
    m_crossfadePending = false;

    for (auto && channel : m_channels) {
        channel.input.assign(m_partitionSize * 2, 0.0);
        channel.output.assign(m_partitionSize, 0.0);
        channel.history.assign(m_partitionCount, Partition { Spectrum(bins, 0.0), Spectrum(bins, 0.0) });
    }
    m_historyIndex = 0;
    m_position = 0;

    m_scratchRe.assign(m_partitionSize * 2, 0.0);
    m_scratchIm.assign(m_partitionSize * 2, 0.0);
    m_mixRe.assign(bins, 0.0);
    m_mixIm.assign(bins, 0.0);
    m_frame.assign(m_partitionSize * 2, 0.0);
    m_previousOutput.assign(m_partitionSize, 0.0);

    m_designRe.assign(m_kernelLength, 0.0);
    m_designIm.assign(m_kernelLength, 0.0);
    m_twiddleRe.resize(m_kernelLength / 2);
    m_twiddleIm.resize(m_kernelLength / 2);
    for (size_t k = 0; k < m_kernelLength / 2; k++) {
        const double angle = -2.0 * std::numbers::pi * static_cast<double>(k) / static_cast<double>(m_kernelLength);
        m_twiddleRe[k] = std::cos(angle);
        m_twiddleIm[k] = std::sin(angle);
    }
    m_stage = DesignStage::Idle;
    m_designCredit = 0.0;
}

bool LinearPhaseFir::isConfigured() const
{
    return m_kernelLength > 0;
}

size_t LinearPhaseFir::kernelLength() const
{
    return m_kernelLength;
}

size_t LinearPhaseFir::partitionSize() const
{
    return m_partitionSize;
}

size_t LinearPhaseFir::latencyFrames() const
{
    return m_kernelLength / 2 + m_partitionSize;
}

void LinearPhaseFir::setIdentityKernel(Kernel & kernel) const
{
    // A unit impulse at the kernel's centre, which is the first tap of one partition: its spectrum
    // is one at every bin, and every other partition is silent
    for (auto && partition : kernel) {
        std::ranges::fill(partition.re, 0.0);
        std::ranges::fill(partition.im, 0.0);
    }
    std::ranges::fill(kernel.at(m_kernelLength / 2 / m_partitionSize).re, 1.0);
}

void LinearPhaseFir::reset()
{
    for (auto && channel : m_channels) {
        std::ranges::fill(channel.input, 0.0);
        std::ranges::fill(channel.output, 0.0);
        for (auto && partition : channel.history) {
            std::ranges::fill(partition.re, 0.0);
            std::ranges::fill(partition.im, 0.0);
        }
    }
    m_position = 0;
}

void LinearPhaseFir::startDesign(const std::vector<MatchedBellFilter::SquaredMagnitude> & sections, double gain)
{
    if (!isConfigured()) {
        return;
    }
    m_sections.assign(sections.begin(), sections.end());
    m_gain = gain;
    m_stage = DesignStage::Magnitude;
    m_designCursor = 0;
}

bool LinearPhaseFir::isDesigning() const
{
    return m_stage != DesignStage::Idle || m_crossfadePending;
}

void LinearPhaseFir::advanceDesign(size_t frames)
{
    if (m_stage == DesignStage::Idle) {
        m_designCredit = 0.0;
        return;
    }
    m_designCredit += static_cast<double>(frames) / FramesPerUnit;
    while (m_designCredit >= 1.0 && m_stage != DesignStage::Idle) {
        if (!doDesignUnit()) {
            break;
        }
        m_designCredit -= 1.0;
    }
}

bool LinearPhaseFir::doDesignUnit()
{
    const size_t length = m_kernelLength;
    switch (m_stage) {
    case DesignStage::Idle:
        return false;
    case DesignStage::Magnitude: {
        // The zero-phase spectrum: the cascade's magnitude, real and even. Scaled by 1/N here so that
        // the forward transform below is the inverse one -- they agree on a real, even sequence.
        const size_t end = std::min(m_designCursor + MagnitudeBinsPerUnit, length / 2 + 1);
        const double scale = m_gain / static_cast<double>(length);
        for (size_t bin = m_designCursor; bin < end; bin++) {
            const double half = std::sin(std::numbers::pi * static_cast<double>(bin) / static_cast<double>(length));
            const double phi1 = half * half;
            const double phi0 = 1.0 - phi1;
            const double phi2 = 4.0 * phi0 * phi1;
            double squared = 1.0;
            for (auto && section : m_sections) {
                const double numerator = section.numerator[0] * phi0 + section.numerator[1] * phi1 + section.numerator[2] * phi2;
                const double denominator = section.denominator[0] * phi0 + section.denominator[1] * phi1 + section.denominator[2] * phi2;
                squared *= numerator / denominator;
            }
            const double value = scale * std::sqrt(std::max(0.0, squared));
            m_designRe[bin] = value;
            m_designIm[bin] = 0.0;
            if (bin > 0 && bin < length / 2) {
                m_designRe[length - bin] = value;
                m_designIm[length - bin] = 0.0;
            }
        }
        m_designCursor = end;
        if (m_designCursor > length / 2) {
            // Bit reversal, which is cheap enough to be a slice of its own
            for (size_t i = 1, j = 0; i < length; i++) {
                size_t bit = length >> 1;
                for (; j & bit; bit >>= 1) {
                    j ^= bit;
                }
                j ^= bit;
                if (i < j) {
                    std::swap(m_designRe[i], m_designRe[j]);
                    std::swap(m_designIm[i], m_designIm[j]);
                }
            }
            m_stage = DesignStage::Transform;
            m_transformLength = 2;
            m_transformGroup = 0;
            m_transformButterfly = 0;
        }
        return true;
    }
    case DesignStage::Transform:
        transformUnit();
        return true;
    case DesignStage::Partitions: {
        // The kernel being written is the idle one, and it stays idle until it has been faded in
        if (m_crossfadePending) {
            return false;
        }
        // The impulse response is centred: tap t is the zero-phase response at t - N/2
        const size_t partition = m_designCursor;
        std::ranges::fill(m_frame, 0.0);
        for (size_t n = 0; n < m_partitionSize; n++) {
            const size_t tap = partition * m_partitionSize + n;
            m_frame[n] = m_designRe[(tap + length - length / 2) % length];
        }
        auto && target = m_kernels.at(1 - m_activeKernel).at(partition);
        Fft::forwardReal(m_frame.data(), target.re.data(), target.im.data(), static_cast<int>(m_partitionSize * 2));
        m_designCursor++;
        if (m_designCursor == m_partitionCount) {
            m_crossfadePending = true;
            m_stage = DesignStage::Idle;
        }
        return true;
    }
    }
    return false;
}

void LinearPhaseFir::transformUnit()
{
    // The radix-2 butterflies of an in-place forward transform, resumed where the last slice stopped
    const size_t length = m_kernelLength;
    size_t budget = ButterfliesPerUnit;
    while (budget > 0 && m_transformLength <= length) {
        const size_t half = m_transformLength / 2;
        const size_t stride = length / m_transformLength;
        const size_t i = m_transformGroup;
        const size_t j = m_transformButterfly;
        const double wRe = m_twiddleRe[j * stride];
        const double wIm = m_twiddleIm[j * stride];
        const double uRe = m_designRe[i + j];
        const double uIm = m_designIm[i + j];
        const double vRe = m_designRe[i + j + half] * wRe - m_designIm[i + j + half] * wIm;
        const double vIm = m_designRe[i + j + half] * wIm + m_designIm[i + j + half] * wRe;
        m_designRe[i + j] = uRe + vRe;
        m_designIm[i + j] = uIm + vIm;
        m_designRe[i + j + half] = uRe - vRe;
        m_designIm[i + j + half] = uIm - vIm;
        budget--;

        if (++m_transformButterfly == half) {
            m_transformButterfly = 0;
            m_transformGroup += m_transformLength;
            if (m_transformGroup >= length) {
                m_transformGroup = 0;
                m_transformLength *= 2;
            }
        }
    }
    if (m_transformLength > length) {
        m_stage = DesignStage::Partitions;
        m_designCursor = 0;
    }
}

void LinearPhaseFir::convolve(const Kernel & kernel, const Channel & channel, Spectrum & re, Spectrum & im) const
{
    std::ranges::fill(re, 0.0);
    std::ranges::fill(im, 0.0);
    for (size_t k = 0; k < m_partitionCount; k++) {
        const auto & input = channel.history.at((m_historyIndex + m_partitionCount - k) % m_partitionCount);
        const auto & taps = kernel.at(k);
        for (size_t bin = 0; bin < re.size(); bin++) {
            re[bin] += input.re[bin] * taps.re[bin] - input.im[bin] * taps.im[bin];
            im[bin] += input.re[bin] * taps.im[bin] + input.im[bin] * taps.re[bin];
        }
    }
}

void LinearPhaseFir::processPartition()
{
    const size_t size = m_partitionSize;
    const size_t transform = size * 2;
    for (auto && channel : m_channels) {
        // Overlap-save: the transform covers the partition before this one as well
        auto && slot = channel.history.at(m_historyIndex);
        Fft::forwardReal(channel.input.data(), slot.re.data(), slot.im.data(), static_cast<int>(transform));

        const auto inverse = [this, size, transform](Spectrum & re, Spectrum & im, std::vector<double> & out) {
            for (size_t bin = 0; bin <= size; bin++) {
                m_scratchRe[bin] = re[bin];
                m_scratchIm[bin] = im[bin];
            }
            for (size_t bin = 1; bin < size; bin++) {
                m_scratchRe[transform - bin] = re[bin];
                m_scratchIm[transform - bin] = -im[bin];
            }
            Fft::inverse(m_scratchRe.data(), m_scratchIm.data(), static_cast<int>(transform));
            for (size_t n = 0; n < size; n++) {
                out[n] = m_scratchRe[size + n];
            }
        };

        convolve(m_kernels.at(m_activeKernel), channel, m_mixRe, m_mixIm);
        inverse(m_mixRe, m_mixIm, channel.output);
        if (m_crossfadePending) {
            // The new kernel faded in over this partition, against the old one on the same input
            m_previousOutput = channel.output;
            convolve(m_kernels.at(1 - m_activeKernel), channel, m_mixRe, m_mixIm);
            inverse(m_mixRe, m_mixIm, channel.output);
            for (size_t n = 0; n < size; n++) {
                const double t = static_cast<double>(n + 1) / static_cast<double>(size);
                channel.output[n] = m_previousOutput[n] * (1.0 - t) + channel.output[n] * t;
            }
        }

        std::copy(channel.input.begin() + static_cast<std::ptrdiff_t>(size), channel.input.end(), channel.input.begin());
    }
    if (m_crossfadePending) {
        m_activeKernel = 1 - m_activeKernel;
        m_crossfadePending = false;
    }
    m_historyIndex = (m_historyIndex + 1) % m_partitionCount;
}

void LinearPhaseFir::process(double & left, double & right)
{
    if (!isConfigured()) {
        return;
    }
    m_channels[0].input[m_partitionSize + m_position] = left;
    m_channels[1].input[m_partitionSize + m_position] = right;
    left = m_channels[0].output[m_position];
    right = m_channels[1].output[m_position];
    if (++m_position == m_partitionSize) {
        m_position = 0;
        processPartition();
    }
}

} // namespace noteahead
