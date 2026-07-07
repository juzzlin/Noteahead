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

#include "ensemble_chorus.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

EnsembleChorus::EnsembleChorus()
{
    m_slowLfo1.setWaveform(Lfo::Waveform::Sine);
    m_slowLfo2.setWaveform(Lfo::Waveform::Sine);
    m_slowLfo3.setWaveform(Lfo::Waveform::Sine);

    m_slowLfo1.setFrequency(0.6);
    m_slowLfo2.setFrequency(0.6);
    m_slowLfo3.setFrequency(0.6);

    m_slowLfo1.setPhase(0.0);
    m_slowLfo2.setPhase(1.0 / 3.0);
    m_slowLfo3.setPhase(2.0 / 3.0);

    m_fastLfo1.setWaveform(Lfo::Waveform::Sine);
    m_fastLfo2.setWaveform(Lfo::Waveform::Sine);
    m_fastLfo3.setWaveform(Lfo::Waveform::Sine);

    m_fastLfo1.setFrequency(6.0);
    m_fastLfo2.setFrequency(6.0);
    m_fastLfo3.setFrequency(6.0);

    m_fastLfo1.setPhase(0.0);
    m_fastLfo2.setPhase(1.0 / 3.0);
    m_fastLfo3.setPhase(2.0 / 3.0);
}

void EnsembleChorus::setSampleRate(double sampleRate)
{
    if (!m_bufferL.empty() && std::abs(m_sampleRate - sampleRate) < 0.1) {
        // Sample rate unchanged and buffers already sized: skip re-init so the
        // delay write position keeps advancing across successive audio callbacks
        // instead of restarting at 0 every buffer.
        return;
    }

    m_sampleRate = sampleRate;

    m_slowLfo1.setSampleRate(sampleRate);
    m_slowLfo2.setSampleRate(sampleRate);
    m_slowLfo3.setSampleRate(sampleRate);

    m_fastLfo1.setSampleRate(sampleRate);
    m_fastLfo2.setSampleRate(sampleRate);
    m_fastLfo3.setSampleRate(sampleRate);

    updateBuffers();
}

void EnsembleChorus::process(double & left, double & right)
{
    if (m_sampleRate <= 0.0 || m_bufferL.empty()) {
        return;
    }

    const double dryL { left };
    const double dryR { right };

    // Write input to circular buffer
    m_bufferL[m_writePos] = dryL;
    m_bufferR[m_writePos] = dryR;

    // LFO modulations
    const double slow1 { m_slowLfo1.nextSample() };
    const double slow2 { m_slowLfo2.nextSample() };
    const double slow3 { m_slowLfo3.nextSample() };

    const double fast1 { m_fastLfo1.nextSample() };
    const double fast2 { m_fastLfo2.nextSample() };
    const double fast3 { m_fastLfo3.nextSample() };

    // Base delay: 25ms
    const double baseDelaySamples { 0.025 * m_sampleRate };

    // Modulation depth
    const double slowDepthSamples { 0.0015 * m_sampleRate };
    const double fastDepthSamples { 0.0002 * m_sampleRate };

    auto calculateDelay = [&](double slowVal, double fastVal) {
        double delay { baseDelaySamples + slowVal * slowDepthSamples };
        if (m_mode == 1 || m_mode == 2) { // Chorus II and Chorus I+II include the fast LFO
            delay += fastVal * fastDepthSamples;
        }
        if (m_mode == 2) { // Chorus I+II: both engaged for a deeper, fuller ensemble
            delay += slowVal * slowDepthSamples * 0.5;
        }
        return delay;
    };

    const double delay1 { calculateDelay(slow1, fast1) };
    const double delay2 { calculateDelay(slow2, fast2) };
    const double delay3 { calculateDelay(slow3, fast3) };

    auto readFromBuffer = [&](const std::vector<double> & buffer, double delaySamples) {
        delaySamples = std::max(1.0, delaySamples);
        const double bufSize { static_cast<double>(buffer.size()) };
        double readPos { static_cast<double>(m_writePos) - delaySamples };
        while (readPos < 0.0) {
            readPos += bufSize;
        }
        while (readPos >= bufSize) {
            readPos -= bufSize;
        }

        const size_t i0 { static_cast<size_t>(readPos) };
        const size_t i1 { (i0 + 1) % buffer.size() };
        const double frac { readPos - static_cast<double>(i0) };

        return buffer[i0] * (1.0 - frac) + buffer[i1] * frac;
    };

    // Read taps (cross-fed between channels for stereo width: delay2 into the
    // left output, delay3 into the right output).
    const double tap1L { readFromBuffer(m_bufferL, delay1) };
    const double tap1R { readFromBuffer(m_bufferR, delay1) };

    const double tap2R { readFromBuffer(m_bufferR, delay2) };

    const double tap3L { readFromBuffer(m_bufferL, delay3) };

    // Mix taps
    const double wetL { (tap1L + tap2R) * 0.5 };
    const double wetR { (tap1R + tap3L) * 0.5 };

    if (m_enabled) {
        left = wetL;
        right = wetR;
    }

    m_writePos = (m_writePos + 1) % static_cast<uint32_t>(m_bufferL.size());
}

void EnsembleChorus::reset()
{
    std::fill(m_bufferL.begin(), m_bufferL.end(), 0.0);
    std::fill(m_bufferR.begin(), m_bufferR.end(), 0.0);
    m_writePos = 0;

    m_slowLfo1.reset();
    m_slowLfo2.reset();
    m_slowLfo3.reset();

    m_slowLfo1.setPhase(0.0);
    m_slowLfo2.setPhase(1.0 / 3.0);
    m_slowLfo3.setPhase(2.0 / 3.0);

    m_fastLfo1.reset();
    m_fastLfo2.reset();
    m_fastLfo3.reset();

    m_fastLfo1.setPhase(0.0);
    m_fastLfo2.setPhase(1.0 / 3.0);
    m_fastLfo3.setPhase(2.0 / 3.0);
}

void EnsembleChorus::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool EnsembleChorus::enabled() const
{
    return m_enabled;
}

void EnsembleChorus::setMode(int mode)
{
    m_mode = std::clamp(mode, 0, 2);
}

int EnsembleChorus::mode() const
{
    return m_mode;
}

void EnsembleChorus::updateBuffers()
{
    if (m_sampleRate <= 0.0) {
        return;
    }

    // Allocate buffer size for maximum delay of 100ms
    const size_t size { static_cast<size_t>(0.1 * m_sampleRate) };
    m_bufferL.resize(size, 0.0);
    m_bufferR.resize(size, 0.0);
    m_writePos = 0;
}

} // namespace noteahead
