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

#ifndef METRONOME_HPP
#define METRONOME_HPP

#include "dsp_component.hpp"

#include <atomic>
#include <cstdint>
#include <span>

namespace noteahead {

//! A click on every beat, with the first beat of a bar accented, and an optional pre-count.
//!
//! Counts samples rather than following the player's ticks, because the thing it exists for happens
//! with the transport stopped: recording a Sampler pad has no song playing to take a beat from. The
//! only thing it needs from the outside is the tempo.
//!
//! Started and stopped from the UI thread and rendered from the audio thread, so the handful of
//! values that cross between them are atomics, the way LevelMeter does it. Nothing is signalled out
//! of the audio thread -- countInFinished() is polled.
class Metronome : public DspComponent
{
public:
    //! Beats counted before countInFinished() turns true. Zero starts the clicking straight away.
    void start(int countInBeats);
    void stop();
    bool running() const;

    //! True once the pre-count has been clicked through. False while it runs, and while stopped.
    bool countInFinished() const;

    //! Beats of the pre-count still to come, for a UI counting them down. Zero once it is through.
    int countInBeatsRemaining() const;

    void setBpm(double bpm);
    void setBeatsPerBar(int beatsPerBar);
    //! Linear, 0..1, applied to the click. The accent is a little louder within it.
    void setLevel(double level);

    //! Audio-thread: adds the click to an interleaved stereo buffer. A no-op while stopped.
    //!
    //! Centred rather than panned: a click is a reference, not part of the mix.
    void render(std::span<double> interleavedStereo, uint32_t frameCount, uint32_t sampleRate);

    void reset();

private:
    //! Where the click sits, in Hz. The accent is a fifth or so above the rest, which is enough to
    //! hear which beat is the downbeat without the two sounding like different instruments.
    static constexpr double ClickHz { 1000.0 };
    static constexpr double AccentHz { 1500.0 };
    //! How long a click lasts. Long enough to hear at any tempo, short enough not to smear into the
    //! next beat at the fastest one anybody counts in at.
    static constexpr double ClickDecaySeconds { 0.035 };
    static constexpr double AccentGain { 1.3 };

    //! Starts a click voice on the beat that has just come round.
    void trigger(bool accent);

    std::atomic<bool> m_running { false };
    std::atomic<double> m_bpm { 120.0 };
    std::atomic<int> m_beatsPerBar { 4 };
    std::atomic<double> m_level { 0.5 };
    std::atomic<int> m_countInBeatsRemaining { 0 };
    std::atomic<bool> m_countInFinished { false };

    //! Audio-thread only, from here down.
    double m_framesToNextBeat { 0.0 };
    int m_beatInBar { 0 };
    //! The click voice: a sine at m_voicePhaseStep, faded out over ClickDecaySeconds.
    double m_voicePhase { 0.0 };
    double m_voicePhaseStep { 0.0 };
    double m_voiceGain { 0.0 };
    double m_voiceDecayPerFrame { 0.0 };
};

} // namespace noteahead

#endif // METRONOME_HPP
