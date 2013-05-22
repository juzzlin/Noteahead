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

#ifndef SPECTRUM_TAPS_HPP
#define SPECTRUM_TAPS_HPP

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace noteahead {

class RtaCore;
struct AudioContext;

//! The spectrum an effect draws behind its curve: one RtaCore on its input, one on its output, run
//! only while something is looking at them.
//!
//! Costs nothing until a tap is first enabled, apart from an atomic load per block: neither core
//! exists before then. Cores are created on the UI thread and handed to the audio thread through an
//! atomic pointer, and once created a core stays until the effect goes, because freeing one under
//! the audio thread is the alternative. Disabling a tap only stops it running.
//!
//! The analysis is the economy setting of the RTA's: 64 bands on windows of 16384 down to 512, at
//! the slowest FFT rate. A curve behind an equalizer's response needs neither more points nor more
//! updates than that, and it runs in the audio path of an effect that is there for something else.
class SpectrumTaps
{
public:
    enum class Tap
    {
        Pre,
        Post
    };

    //! @p maxPreDelayFrames is the longest the input ever has to be held back to stay in time with
    //! an output the effect delays. Allocated with the Pre core, so only once it is first asked for.
    explicit SpectrumTaps(size_t maxPreDelayFrames = 0);
    ~SpectrumTaps();

    SpectrumTaps(const SpectrumTaps &) = delete;
    SpectrumTaps & operator=(const SpectrumTaps &) = delete;

    //! UI thread. A tap that comes back on starts from silence rather than from whatever it held
    //! when it went off.
    void setEnabled(bool pre, bool post);
    //! Whether either tap is running. Any thread.
    bool isEnabled() const;

    //! Audio thread, before the effect has processed the block. @p delayFrames holds the input back
    //! by the effect's own latency, so that the two taps compare the same moment.
    void processPre(const AudioContext & context, size_t delayFrames = 0);
    //! Audio thread, after the effect has processed the block.
    void processPost(const AudioContext & context);

    //! The Post tap fed from another signal than the block's own, @p frameCount stereo frames of
    //! it, or silence while @p stereo is empty: a side-chain, which can come and go.
    void processPost(std::span<const double> stereo, uint32_t frameCount, uint32_t sampleRate);

    //! UI thread. Nullptr until @p tap has first been enabled.
    const RtaCore * core(Tap tap) const;

private:
    struct Channel
    {
        //! Touched by the UI thread only. The audio thread goes through core.
        std::unique_ptr<RtaCore> owner;
        std::atomic<RtaCore *> core { nullptr };
        std::atomic<bool> enabled { false };
        std::atomic<bool> resetPending { false };
    };

    //! The core of @p channel if it is to run this block, reset first if that was asked for.
    RtaCore * runningCore(Channel & channel);

    std::array<Channel, 2> m_channels;

    size_t m_maxPreDelayFrames { 0 };
    //! Allocated on the UI thread before the Pre core is published, and never resized after.
    std::vector<double> m_preDelay;
    // Audio thread only:
    size_t m_preDelayFrames { 0 };
    size_t m_preDelayIndex { 0 };
};

} // namespace noteahead

#endif // SPECTRUM_TAPS_HPP
