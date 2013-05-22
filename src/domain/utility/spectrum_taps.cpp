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

#include "spectrum_taps.hpp"

#include "../dsp/audio_context.hpp"
#include "rta_core.hpp"

#include <algorithm>

namespace noteahead {

namespace {

constexpr int LongestFftSize = 16384;
constexpr int BandCountMode = 1; // 64 bands
constexpr int SpeedMode = 1; // Normal
constexpr int FftRateMode = 2; // Slow

size_t index(SpectrumTaps::Tap tap)
{
    return tap == SpectrumTaps::Tap::Pre ? 0 : 1;
}

} // namespace

SpectrumTaps::SpectrumTaps(size_t maxPreDelayFrames)
  : m_maxPreDelayFrames { maxPreDelayFrames }
{
}

SpectrumTaps::~SpectrumTaps() = default;

void SpectrumTaps::setEnabled(bool pre, bool post)
{
    for (const auto tap : { Tap::Pre, Tap::Post }) {
        auto & channel = m_channels.at(index(tap));
        const bool enable = tap == Tap::Pre ? pre : post;
        if (enable && !channel.owner) {
            if (tap == Tap::Pre) {
                m_preDelay.assign(m_maxPreDelayFrames, 0.0);
            }
            channel.owner = std::make_unique<RtaCore>(LongestFftSize);
            channel.owner->configure(BandCountMode, SpeedMode, FftRateMode);
            channel.core.store(channel.owner.get(), std::memory_order_release);
        }
        if (enable && !channel.enabled.load(std::memory_order_relaxed)) {
            channel.resetPending.store(true, std::memory_order_relaxed);
        }
        channel.enabled.store(enable, std::memory_order_release);
    }
}

bool SpectrumTaps::isEnabled() const
{
    return std::ranges::any_of(m_channels, [](auto && channel) { return channel.enabled.load(std::memory_order_relaxed); });
}

RtaCore * SpectrumTaps::runningCore(Channel & channel)
{
    if (!channel.enabled.load(std::memory_order_acquire)) {
        return nullptr;
    }
    const auto core = channel.core.load(std::memory_order_acquire);
    if (core && channel.resetPending.exchange(false, std::memory_order_relaxed)) {
        core->reset();
        if (&channel == &m_channels.at(index(Tap::Pre))) {
            std::ranges::fill(m_preDelay, 0.0);
            m_preDelayIndex = 0;
        }
    }
    return core;
}

void SpectrumTaps::processPre(const AudioContext & context, size_t delayFrames)
{
    const auto core = runningCore(m_channels.at(index(Tap::Pre)));
    if (!core) {
        return;
    }

    const size_t frames = std::min(delayFrames, m_preDelay.size());
    if (frames != m_preDelayFrames) {
        m_preDelayFrames = frames;
        m_preDelayIndex = 0;
        std::fill_n(m_preDelay.begin(), frames, 0.0);
    }
    if (!frames) {
        core->process(context);
        return;
    }

    core->setSampleRate(context.sampleRate);
    for (uint32_t i = 0; i < context.frameCount; i++) {
        const double delayed = m_preDelay[m_preDelayIndex];
        m_preDelay[m_preDelayIndex] = (context.buffer[i * 2] + context.buffer[i * 2 + 1]) * 0.5;
        if (++m_preDelayIndex == frames) {
            m_preDelayIndex = 0;
        }
        core->addSample(delayed);
    }
}

void SpectrumTaps::processPost(const AudioContext & context)
{
    if (const auto core = runningCore(m_channels.at(index(Tap::Post))); core) {
        core->process(context);
    }
}

void SpectrumTaps::processPost(std::span<const double> stereo, uint32_t frameCount, uint32_t sampleRate)
{
    const auto core = runningCore(m_channels.at(index(Tap::Post)));
    if (!core) {
        return;
    }
    core->setSampleRate(sampleRate);
    const bool present = stereo.size() >= static_cast<size_t>(frameCount) * 2;
    for (uint32_t i = 0; i < frameCount; i++) {
        core->addSample(present ? (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5 : 0.0);
    }
}

const RtaCore * SpectrumTaps::core(Tap tap) const
{
    return m_channels.at(index(tap)).owner.get();
}

} // namespace noteahead
