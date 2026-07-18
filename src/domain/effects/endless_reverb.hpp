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

#ifndef ENDLESS_REVERB_HPP
#define ENDLESS_REVERB_HPP

#include "../dsp/cascaded_svf.hpp"
#include "effect.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace noteahead {

// A large, otherworldly ambient reverb: an 8-line modulated Householder FDN with input diffusion, a high
// feedback control, and a Freeze switch for infinite ("endless") tails.
class EndlessReverb : public Effect
{
public:
    EndlessReverb();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void process(AudioContext & context) override;
    void reset() override;
    void sync() override;

private:
    void syncParameters();
    void updateBuffers();
    void updateFilters();
    void processSample(double & left, double & right);

    float m_size { 0.7f };
    float m_feedback { 0.85f };
    float m_damping { 0.3f };
    float m_preDelayMs { 20.0f };
    float m_modDepth { 0.4f };
    float m_modRateHz { 0.3f };
    float m_width { 1.0f };
    float m_lpfCutoff { 0.8f };
    float m_hpfCutoff { 0.2f };
    float m_mix { 0.0f };
    bool m_freeze { false };

    bool m_shouldSyncParameters { false };
    bool m_shouldUpdateBuffers { false };

    static constexpr int NumDelays = 8;
    static constexpr int NumDiffusers = 4;

    struct DelayLine
    {
        std::vector<double> buffer;
        uint32_t writePos { 0 };
        uint32_t bufferLen { 0 };
        double nominalDelay { 0.0 };
        double modDepthSamples { 0.0 };
        double lpState { 0.0 };
        double lfoPhase { 0.0 };
        double lfoInc { 0.0 };
        CascadedSvf fbLpf;
        CascadedSvf fbHpf;

        // Linearly interpolated read, delaySamples behind the write pointer.
        double read(double delaySamples) const
        {
            double readPos = static_cast<double>(writePos) - delaySamples;
            while (readPos < 0.0) {
                readPos += static_cast<double>(bufferLen);
            }
            while (readPos >= static_cast<double>(bufferLen)) {
                readPos -= static_cast<double>(bufferLen);
            }
            const size_t i0 = static_cast<size_t>(readPos);
            const size_t i1 = (i0 + 1) % bufferLen;
            const double frac = readPos - static_cast<double>(i0);
            return buffer[i0] * (1.0 - frac) + buffer[i1] * frac;
        }

        void write(double value)
        {
            buffer[writePos] = value;
            writePos = (writePos + 1) % bufferLen;
        }

        void reset()
        {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            writePos = 0;
            lpState = 0.0;
            fbLpf.reset();
            fbHpf.reset();
        }
    };

    // Schroeder all-pass diffuser (Freeverb form) used to smear the input before the tank.
    struct Allpass
    {
        std::vector<double> buffer;
        uint32_t writePos { 0 };
        uint32_t size { 0 };
        double coeff { 0.7 };

        double process(double in)
        {
            const double buffered = buffer[writePos];
            const double out = -in + buffered;
            buffer[writePos] = in + buffered * coeff;
            writePos = (writePos + 1) % size;
            return out;
        }

        void reset()
        {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            writePos = 0;
        }
    };

    std::array<DelayLine, NumDelays> m_delays;
    std::array<Allpass, NumDiffusers> m_diffusers;
    std::vector<double> m_preDelayBuffer;
    uint32_t m_preDelayWritePos { 0 };
    uint32_t m_lastSampleRate { 0 };

    CascadedSvf m_wetLpfL;
    CascadedSvf m_wetLpfR;
    CascadedSvf m_wetHpfL;
    CascadedSvf m_wetHpfR;
};

} // namespace noteahead

#endif // ENDLESS_REVERB_HPP
