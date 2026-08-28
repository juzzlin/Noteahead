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

#include "speech_sequencer.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Shortest a phoneme may be squeezed to, in seconds. Below this a formant transition has no room
//! to happen and the phoneme stops being audible as itself, so a fast tempo shortens a phrase by
//! dropping nothing and slurring everything rather than by turning it into clicks.
constexpr double MinimumPhonemeSeconds = 0.012;

} // namespace

void SpeechSequencer::setSampleRate(double sampleRate)
{
    m_sampleRate = std::max(1.0, sampleRate);
}

void SpeechSequencer::setBpm(double bpm)
{
    const double updated = std::max(1.0, bpm);
    if (std::abs(updated - m_bpm) < 1.0e-9) {
        return;
    }
    m_bpm = updated;

    // Durations are worked out when an utterance starts, so a tempo that moves after that -- or a
    // note that arrives before the first audio block has told the sequencer what the tempo is --
    // would otherwise leave the phrase running at whatever tempo happened to be in effect at the
    // trigger. Recomputing keeps Fit landing on the bar and Grid on the division either way.
    if (m_active && m_syncMode != SyncMode::Free) {
        computeDurations(m_start, m_end);
        m_duration = m_durations[m_index];
        m_utteranceFrames = 0;
        for (size_t i = m_start; i < m_end; i++) {
            m_utteranceFrames += m_durations[i];
        }
    }
}

void SpeechSequencer::setPhonemes(PhonemeEventList phonemes)
{
    m_phonemes = std::move(phonemes);
    m_durations.assign(m_phonemes.size(), 0);

    m_syllableStarts.clear();
    for (size_t i = 0; i < m_phonemes.size(); i++) {
        if (m_phonemes[i].syllableStart) {
            m_syllableStarts.push_back(i);
        }
    }
    // Anything before the first marked syllable still has to be spoken, so the first utterance
    // starts at the beginning of the phrase whether or not a mark landed there.
    if (!m_phonemes.empty() && (m_syllableStarts.empty() || m_syllableStarts.front())) {
        m_syllableStarts.insert(m_syllableStarts.begin(), 0);
    }

    reset();
}

const PhonemeEventList & SpeechSequencer::phonemes() const
{
    return m_phonemes;
}

void SpeechSequencer::setTriggerMode(TriggerMode mode)
{
    m_triggerMode = mode;
}

void SpeechSequencer::setSyncMode(SyncMode mode)
{
    m_syncMode = mode;
}

void SpeechSequencer::setRate(double rate)
{
    m_rate = std::clamp(rate, 0.1, 8.0);
}

void SpeechSequencer::setLengthBeats(double beats)
{
    m_lengthBeats = std::max(0.05, beats);
}

void SpeechSequencer::setDivisionBeats(double beats)
{
    m_divisionBeats = std::max(0.05, beats);
}

double SpeechSequencer::beatFrames() const
{
    return 60.0 / m_bpm * m_sampleRate;
}

size_t SpeechSequencer::nominalFrames(const PhonemeEvent & event) const
{
    return static_cast<size_t>(event.spec->nominalMs * event.lengthScale * m_sampleRate / 1000.0);
}

void SpeechSequencer::computeDurations(size_t from, size_t to)
{
    const auto minimum = static_cast<size_t>(MinimumPhonemeSeconds * m_sampleRate);
    const auto clampDuration = [minimum](double frames) {
        return std::max(minimum, static_cast<size_t>(frames));
    };

    switch (m_syncMode) {
    case SyncMode::Free:
        for (size_t i = from; i < to; i++) {
            m_durations[i] = clampDuration(nominalFrames(m_phonemes[i]) / m_rate);
        }
        break;

    case SyncMode::Fit: {
        // Natural durations scaled together, so the phrase lands on the grid with its own rhythm
        // intact rather than with every phoneme the same length.
        double nominalTotal = 0.0;
        for (size_t i = from; i < to; i++) {
            nominalTotal += static_cast<double>(nominalFrames(m_phonemes[i]));
        }
        const double target = m_lengthBeats * beatFrames();
        const double scale = nominalTotal > 0.0 ? target / nominalTotal : 1.0;
        for (size_t i = from; i < to; i++) {
            m_durations[i] = clampDuration(nominalFrames(m_phonemes[i]) * scale);
        }
        break;
    }

    case SyncMode::Grid: {
        // Each syllable gets exactly one division. Inside it the consonants keep their natural
        // length and the vowel absorbs the slack, because consonants are near enough constant in
        // real speech while vowels are what stretch. Sharing the slot out proportionally instead
        // makes every consonant stretch with the tempo, and it stops sounding like speech.
        const double slot = m_divisionBeats * beatFrames();
        size_t syllable = from;
        while (syllable < to) {
            size_t next = syllable + 1;
            while (next < to && !m_phonemes[next].syllableStart) {
                next++;
            }

            double consonantFrames = 0.0;
            size_t vowels = 0;
            for (size_t i = syllable; i < next; i++) {
                if (m_phonemes[i].spec->type == PhonemeType::Vowel) {
                    vowels++;
                } else {
                    consonantFrames += static_cast<double>(nominalFrames(m_phonemes[i]));
                }
            }

            if (vowels && consonantFrames < slot) {
                const double perVowel = (slot - consonantFrames) / static_cast<double>(vowels);
                for (size_t i = syllable; i < next; i++) {
                    const bool isVowel = m_phonemes[i].spec->type == PhonemeType::Vowel;
                    m_durations[i] = clampDuration(isVowel ? perVowel : nominalFrames(m_phonemes[i]));
                }
            } else {
                // No vowel to absorb the slack, or the consonants alone already overrun the slot.
                double nominalTotal = 0.0;
                for (size_t i = syllable; i < next; i++) {
                    nominalTotal += static_cast<double>(nominalFrames(m_phonemes[i]));
                }
                const double scale = nominalTotal > 0.0 ? slot / nominalTotal : 1.0;
                for (size_t i = syllable; i < next; i++) {
                    m_durations[i] = clampDuration(nominalFrames(m_phonemes[i]) * scale);
                }
            }

            syllable = next;
        }
        break;
    }
    }
}

void SpeechSequencer::beginUtterance(size_t from, size_t to)
{
    if (from >= to || to > m_phonemes.size()) {
        m_active = false;
        return;
    }

    computeDurations(from, to);

    m_start = from;
    m_index = from;
    m_end = to;
    m_frame = 0;
    m_duration = m_durations[from];
    m_utteranceFrame = 0;
    m_utteranceFrames = 0;
    for (size_t i = from; i < to; i++) {
        m_utteranceFrames += m_durations[i];
    }
    m_active = true;
}

void SpeechSequencer::trigger()
{
    m_held = true;

    if (m_phonemes.empty()) {
        m_active = false;
        return;
    }

    if (m_triggerMode == TriggerMode::Phrase) {
        beginUtterance(0, m_phonemes.size());
        return;
    }

    if (m_syllableStarts.empty()) {
        m_active = false;
        return;
    }

    const size_t cursor = m_syllableCursor % m_syllableStarts.size();
    const size_t from = m_syllableStarts[cursor];
    const size_t to = cursor + 1 < m_syllableStarts.size() ? m_syllableStarts[cursor + 1] : m_phonemes.size();
    m_syllableCursor = (cursor + 1) % m_syllableStarts.size();
    beginUtterance(from, to);
}

void SpeechSequencer::release()
{
    m_held = false;
}

void SpeechSequencer::stop()
{
    m_active = false;
    m_held = false;
}

void SpeechSequencer::reset()
{
    stop();
    m_start = 0;
    m_index = 0;
    m_end = 0;
    m_frame = 0;
    m_duration = 0;
    m_utteranceFrame = 0;
    m_utteranceFrames = 0;
    m_syllableCursor = 0;
}

bool SpeechSequencer::sustainsCurrent() const
{
    // Only one thing sustains: the vowel a Step-mode syllable ends on, and only while the note is
    // held and the tempo is not already deciding the length. A phrase is an event and runs to its
    // end; a syllable on a note lasts as long as the note.
    return m_triggerMode == TriggerMode::Step
      && m_syncMode == SyncMode::Free
      && m_held
      && m_index + 1 == m_end
      && m_phonemes[m_index].spec->type == PhonemeType::Vowel;
}

bool SpeechSequencer::advance()
{
    if (!m_active) {
        return false;
    }

    m_frame++;
    m_utteranceFrame++;

    if (m_frame >= m_duration) {
        if (sustainsCurrent()) {
            m_frame = m_duration;
            return true;
        }
        m_index++;
        if (m_index >= m_end) {
            m_active = false;
            return false;
        }
        m_frame = 0;
        m_duration = m_durations[m_index];
    }

    return true;
}

bool SpeechSequencer::isActive() const
{
    return m_active;
}

const PhonemeSpec * SpeechSequencer::phoneme() const
{
    return m_active && m_index < m_phonemes.size() ? m_phonemes[m_index].spec : nullptr;
}

const PhonemeSpec * SpeechSequencer::nextPhoneme() const
{
    return m_active && m_index + 1 < m_end ? m_phonemes[m_index + 1].spec : nullptr;
}

double SpeechSequencer::phonemeSeconds() const
{
    return m_active && m_sampleRate > 0.0 ? static_cast<double>(m_duration) / m_sampleRate : 0.0;
}

bool SpeechSequencer::isStressed() const
{
    return m_active && m_index < m_phonemes.size() && m_phonemes[m_index].stressed;
}

double SpeechSequencer::progress() const
{
    if (!m_duration) {
        return 0.0;
    }
    return std::min(1.0, static_cast<double>(m_frame) / static_cast<double>(m_duration));
}

double SpeechSequencer::utteranceProgress() const
{
    if (!m_utteranceFrames) {
        return 0.0;
    }
    return std::min(1.0, static_cast<double>(m_utteranceFrame) / static_cast<double>(m_utteranceFrames));
}

size_t SpeechSequencer::syllableCursor() const
{
    return m_syllableCursor;
}

size_t SpeechSequencer::syllableCount() const
{
    return m_syllableStarts.size();
}

} // namespace noteahead
