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

#ifndef STEREO_LEVEL_METER_HPP
#define STEREO_LEVEL_METER_HPP

#include <atomic>
#include <cstdint>

namespace noteahead {

//! Peak and RMS level tap that keeps its two channels apart.
//!
//! LevelMeter folds a stereo pair into one reading, which is what a mixer strip wants and what the
//! mixer is built on, so it is left alone. This is the same meter with the fold taken out: the same
//! atomic gate making write() a cheap no-op while nothing is on screen, the same fixed peak fallback
//! rate and RMS window so a reading does not depend on how often the UI polls, and no mutex, because
//! there is nothing to protect but scalars the audio thread stores and the UI thread loads.
//!
//! Both channels are folded in one pass over the buffer, so the audio thread walks it once.
class StereoLevelMeter
{
public:
    //! Level reported when nothing is sounding. Also the floor of the dB conversion.
    static constexpr float MinimumDb { -120.0f };

    void setActive(bool active);
    bool active() const;

    //! Audio-thread: fold one interleaved stereo buffer into the running levels. No-op when inactive.
    void write(const double * interleavedStereo, uint32_t frameCount, uint32_t sampleRate);

    //! Audio-thread: the same for a buffer of any channel count, so a mono capture can feed it.
    //!
    //! One channel goes to both sides rather than leaving the right dead: a mono input is not a
    //! silent right channel, and a meter that said so would read as a broken cable.
    void write(const double * interleaved, uint32_t frameCount, uint32_t sampleRate, uint32_t channelCount);

    //! UI-thread: current levels in dBFS, clamped at MinimumDb.
    float leftPeakDb() const;
    float leftRmsDb() const;
    float rightPeakDb() const;
    float rightRmsDb() const;

    void reset();

private:
    //! How fast the peak reading falls when the signal drops, in dB per second.
    static constexpr float PeakFallbackDbPerSecond { 20.0f };
    //! Averaging window of the RMS reading, in seconds.
    static constexpr float RmsWindowSeconds { 0.3f };

    //! One channel's two scalars, so the pair stays together and the maths is written once.
    struct Channel
    {
        std::atomic<float> peak { 0.0f };
        std::atomic<float> meanSquare { 0.0f };
    };

    static void fold(Channel & channel, float bufferPeak, float bufferMeanSquare, float seconds);

    std::atomic<bool> m_active { false };
    Channel m_left;
    Channel m_right;
};

} // namespace noteahead

#endif // STEREO_LEVEL_METER_HPP
