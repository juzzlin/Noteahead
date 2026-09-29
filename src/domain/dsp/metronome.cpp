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

#include "metronome.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

void Metronome::start(int countInBeats)
{
    m_countInBeatsRemaining.store(std::max(0, countInBeats));
    m_countInFinished.store(countInBeats <= 0);
    // Reset before running rather than after stopping, so a metronome started again lands its first
    // click immediately instead of finishing the interval the previous run was part way through.
    reset();
    m_running.store(true);
}

void Metronome::stop()
{
    m_running.store(false);
    m_countInBeatsRemaining.store(0);
    m_countInFinished.store(false);
}

bool Metronome::running() const
{
    return m_running.load();
}

bool Metronome::countInFinished() const
{
    return m_countInFinished.load();
}

int Metronome::countInBeatsRemaining() const
{
    return m_countInBeatsRemaining.load();
}

void Metronome::setBpm(double bpm)
{
    // A tempo of zero would put the beats infinitely far apart and divide by it below.
    m_bpm.store(bpm > 0.0 ? bpm : 120.0);
}

void Metronome::setBeatsPerBar(int beatsPerBar)
{
    m_beatsPerBar.store(std::max(1, beatsPerBar));
}

void Metronome::setLevel(double level)
{
    m_level.store(std::clamp(level, 0.0, 1.0));
}

void Metronome::trigger(bool accent)
{
    const auto rate = m_sampleRate > 0.0 ? m_sampleRate : 48000.0;
    m_voicePhase = 0.0;
    m_voicePhaseStep = 2.0 * std::numbers::pi * (accent ? AccentHz : ClickHz) / rate;
    m_voiceGain = m_level.load() * (accent ? AccentGain : 1.0);
    // Falls to about -60 dB over the decay time, which is where it stops being audible.
    m_voiceDecayPerFrame = std::pow(0.001, 1.0 / (ClickDecaySeconds * rate));
}

void Metronome::render(std::span<double> interleavedStereo, uint32_t frameCount, uint32_t sampleRate)
{
    if (!m_running.load() || !frameCount || !sampleRate) {
        return;
    }

    // The rate the backend is actually running at wins over whatever was set: the click has to stay
    // in time with the clock that is really counting, not the one it was told about.
    if (m_sampleRate != static_cast<double>(sampleRate)) {
        setSampleRate(static_cast<double>(sampleRate));
    }

    const auto framesPerBeat = m_sampleRate * 60.0 / m_bpm.load();
    const auto beatsPerBar = m_beatsPerBar.load();

    for (uint32_t frame = 0; frame < frameCount; frame++) {
        if (m_framesToNextBeat <= 0.0) {
            trigger(m_beatInBar == 0);
            m_beatInBar = (m_beatInBar + 1) % beatsPerBar;
            // Counted as the beat sounds, so a pre-count of four is through on the fourth click and
            // the downbeat that follows it belongs to the take.
            if (const auto remaining = m_countInBeatsRemaining.load(); remaining > 0) {
                m_countInBeatsRemaining.store(remaining - 1);
                if (remaining == 1) {
                    m_countInFinished.store(true);
                }
            }
            m_framesToNextBeat += framesPerBeat;
        }
        m_framesToNextBeat -= 1.0;

        if (m_voiceGain > 0.0001) {
            const auto sample = std::sin(m_voicePhase) * m_voiceGain;
            m_voicePhase += m_voicePhaseStep;
            m_voiceGain *= m_voiceDecayPerFrame;
            const auto index = static_cast<size_t>(frame) * 2;
            if (index + 1 < interleavedStereo.size()) {
                interleavedStereo[index] += sample;
                interleavedStereo[index + 1] += sample;
            }
        } else {
            m_voiceGain = 0.0;
        }
    }
}

void Metronome::reset()
{
    m_framesToNextBeat = 0.0;
    m_beatInBar = 0;
    m_voicePhase = 0.0;
    m_voiceGain = 0.0;
}

} // namespace noteahead
