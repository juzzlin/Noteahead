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

#include "speech_sequencer_v2.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr double MinimumPhonemeSeconds = 0.012;

} // namespace

void SpeechSequencerV2::setSampleRate(double sampleRate)
{
    m_sampleRate = std::max(1.0, sampleRate);
}

void SpeechSequencerV2::setBpm(double bpm)
{
    const double updated = std::max(1.0, bpm);
    if (std::abs(updated - m_bpm) < 1.0e-9) {
        return;
    }
    m_bpm = updated;

    if (m_active && m_syncMode != SyncMode::Free) {
        computeDurations(m_start, m_end);
        m_duration = m_durations[m_index];
        m_utteranceFrames = 0;
        for (size_t i = m_start; i < m_end; i++) {
            m_utteranceFrames += m_durations[i];
        }
    }
}

void SpeechSequencerV2::setPhonemes(PhonemeEventList phonemes)
{
    m_phonemes = std::move(phonemes);
    m_durations.assign(m_phonemes.size(), 0);

    m_syllableStarts.clear();
    m_lineStarts.clear();
    for (size_t i = 0; i < m_phonemes.size(); i++) {
        if (m_phonemes[i].syllableStart) {
            m_syllableStarts.push_back(i);
        }
        if (m_phonemes[i].lineStart) {
            m_lineStarts.push_back(i);
        }
    }
    if (!m_phonemes.empty() && (m_syllableStarts.empty() || m_syllableStarts.front())) {
        m_syllableStarts.insert(m_syllableStarts.begin(), 0);
    }
    if (!m_phonemes.empty() && (m_lineStarts.empty() || m_lineStarts.front())) {
        m_lineStarts.insert(m_lineStarts.begin(), 0);
    }

    reset();
}

const PhonemeEventList & SpeechSequencerV2::phonemes() const
{
    return m_phonemes;
}

void SpeechSequencerV2::setTriggerMode(TriggerMode mode)
{
    m_triggerMode = mode;
}

void SpeechSequencerV2::setSyncMode(SyncMode mode)
{
    m_syncMode = mode;
}

void SpeechSequencerV2::setRate(double rate)
{
    m_rate = std::clamp(rate, 0.1, 8.0);
}

void SpeechSequencerV2::setLengthBeats(double beats)
{
    m_lengthBeats = std::max(0.05, beats);
}

void SpeechSequencerV2::setDivisionBeats(double beats)
{
    m_divisionBeats = std::max(0.05, beats);
}

void SpeechSequencerV2::setNoteBeats(std::optional<double> beats)
{
    m_noteBeats = beats.has_value() ? std::optional { std::max(0.05, *beats) } : std::nullopt;
}

void SpeechSequencerV2::setCursor(size_t noteIndex)
{
    m_syllableCursor = m_syllableStarts.empty() ? 0 : noteIndex % m_syllableStarts.size();
    m_lineCursor = m_lineStarts.empty() ? 0 : noteIndex % m_lineStarts.size();
}

double SpeechSequencerV2::beatFrames() const
{
    return 60.0 / m_bpm * m_sampleRate;
}

size_t SpeechSequencerV2::nominalFrames(const PhonemeEvent & event) const
{
    return static_cast<size_t>(event.spec->nominalMs * event.lengthScale * m_sampleRate / 1000.0);
}

void SpeechSequencerV2::computeDurations(size_t from, size_t to)
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
        double nominalTotal = 0.0;
        for (size_t i = from; i < to; i++) {
            nominalTotal += static_cast<double>(nominalFrames(m_phonemes[i]));
        }
        const bool followsNote = m_triggerMode == TriggerMode::Line && m_noteBeats.has_value();
        const double target = (followsNote ? *m_noteBeats : m_lengthBeats) * beatFrames();
        const double scale = nominalTotal > 0.0 ? target / nominalTotal : 1.0;
        for (size_t i = from; i < to; i++) {
            m_durations[i] = clampDuration(nominalFrames(m_phonemes[i]) * scale);
        }
        break;
    }

    case SyncMode::Grid: {
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

size_t SpeechSequencerV2::trimTrailingSilence(size_t from, size_t to) const
{
    while (to > from && m_phonemes[to - 1].spec->type == PhonemeType::Silence) {
        to--;
    }
    return to > from ? to : to + 1;
}

void SpeechSequencerV2::beginUtterance(size_t from, size_t to)
{
    if (from >= to || to > m_phonemes.size()) {
        m_active = false;
        m_sustaining = false;
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
    m_sustaining = false;
    m_active = true;
}

void SpeechSequencerV2::trigger()
{
    m_held = true;

    if (m_phonemes.empty()) {
        m_active = false;
        m_sustaining = false;
        return;
    }

    if (m_triggerMode == TriggerMode::Phrase) {
        beginUtterance(0, m_phonemes.size());
        return;
    }

    if (m_triggerMode == TriggerMode::Line) {
        if (m_lineStarts.empty()) {
            m_active = false;
            m_sustaining = false;
            return;
        }
        const size_t cursor = m_lineCursor % m_lineStarts.size();
        const size_t from = m_lineStarts[cursor];
        const size_t to = cursor + 1 < m_lineStarts.size() ? m_lineStarts[cursor + 1] : m_phonemes.size();
        m_lineCursor = (cursor + 1) % m_lineStarts.size();
        beginUtterance(from, trimTrailingSilence(from, to));
        return;
    }

    if (m_syllableStarts.empty()) {
        m_active = false;
        m_sustaining = false;
        return;
    }

    const size_t cursor = m_syllableCursor % m_syllableStarts.size();
    const size_t from = m_syllableStarts[cursor];
    const size_t to = cursor + 1 < m_syllableStarts.size() ? m_syllableStarts[cursor + 1] : m_phonemes.size();
    m_syllableCursor = (cursor + 1) % m_syllableStarts.size();
    beginUtterance(from, to);
}

void SpeechSequencerV2::release()
{
    m_held = false;
    m_sustaining = false;
}

void SpeechSequencerV2::stop()
{
    m_active = false;
    m_held = false;
    m_sustaining = false;
}

void SpeechSequencerV2::reset()
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
    m_lineCursor = 0;
}

bool SpeechSequencerV2::sustainsCurrent() const
{
    return m_triggerMode == TriggerMode::Step
      && m_syncMode == SyncMode::Free
      && m_held
      && m_index + 1 == m_end
      && m_phonemes[m_index].spec->type == PhonemeType::Vowel
      && !m_phonemes[m_index].spec->glideTo.has_value();
}

bool SpeechSequencerV2::holdsDiphthongNucleus() const
{
    return m_triggerMode == TriggerMode::Step
      && m_syncMode == SyncMode::Free
      && m_held
      && m_index + 1 == m_end
      && m_phonemes[m_index].spec->type == PhonemeType::Vowel
      && m_phonemes[m_index].spec->glideTo.has_value();
}

size_t SpeechSequencerV2::diphthongHoldFrame() const
{
    return std::max<size_t>(1, static_cast<size_t>(std::round(static_cast<double>(m_duration) * DiphthongHoldFraction)));
}

bool SpeechSequencerV2::advance()
{
    if (!m_active) {
        return false;
    }

    if (holdsDiphthongNucleus()) {
        const size_t holdFrame = diphthongHoldFrame();
        if (m_frame + 1 >= holdFrame) {
            m_frame = holdFrame;
            m_sustaining = true;
            return true;
        }
    }

    m_sustaining = false;
    m_frame++;
    m_utteranceFrame++;

    if (m_frame >= m_duration) {
        if (sustainsCurrent()) {
            m_frame = m_duration;
            m_sustaining = true;
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

bool SpeechSequencerV2::isActive() const
{
    return m_active;
}

bool SpeechSequencerV2::isSustaining() const
{
    return m_active && m_sustaining;
}

size_t SpeechSequencerV2::currentIndex() const
{
    return m_index;
}

const PhonemeSpec * SpeechSequencerV2::phoneme() const
{
    return m_active && m_index < m_phonemes.size() ? m_phonemes[m_index].spec : nullptr;
}

const PhonemeSpec * SpeechSequencerV2::nextPhoneme() const
{
    return m_active && m_index + 1 < m_end ? m_phonemes[m_index + 1].spec : nullptr;
}

double SpeechSequencerV2::phonemeSeconds() const
{
    return m_active && m_sampleRate > 0.0 ? static_cast<double>(m_duration) / m_sampleRate : 0.0;
}

double SpeechSequencerV2::phonemeElapsedSeconds() const
{
    return m_active && m_sampleRate > 0.0 ? static_cast<double>(m_frame) / m_sampleRate : 0.0;
}

bool SpeechSequencerV2::isStressed() const
{
    return m_active && m_index < m_phonemes.size() && m_phonemes[m_index].stressed;
}

double SpeechSequencerV2::progress() const
{
    if (!m_duration) {
        return 0.0;
    }
    return std::min(1.0, static_cast<double>(m_frame) / static_cast<double>(m_duration));
}

double SpeechSequencerV2::utteranceProgress() const
{
    if (!m_utteranceFrames) {
        return 0.0;
    }
    return std::min(1.0, static_cast<double>(m_utteranceFrame) / static_cast<double>(m_utteranceFrames));
}

size_t SpeechSequencerV2::syllableCursor() const
{
    return m_syllableCursor;
}

size_t SpeechSequencerV2::syllableCount() const
{
    return m_syllableStarts.size();
}

size_t SpeechSequencerV2::lineCursor() const
{
    return m_lineCursor;
}

size_t SpeechSequencerV2::lineCount() const
{
    return m_lineStarts.size();
}

} // namespace noteahead
