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

#ifndef SPEECH_SEQUENCER_V2_HPP
#define SPEECH_SEQUENCER_V2_HPP

#include "speech_sequencer.hpp"
#include "text_to_phonemes.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace noteahead {

//! Second-generation speech sequencer extending SpeechSequencer behavior with event indexing and
//! Step-mode diphthong nucleus sustain (holding on the nucleus until note-off releases the offglide).
class SpeechSequencerV2
{
public:
    using TriggerMode = SpeechSequencer::TriggerMode;
    using SyncMode = SpeechSequencer::SyncMode;

    void setSampleRate(double sampleRate);
    void setBpm(double bpm);

    void setPhonemes(PhonemeEventList phonemes);
    const PhonemeEventList & phonemes() const;

    void setTriggerMode(TriggerMode mode);
    void setSyncMode(SyncMode mode);
    void setRate(double rate);
    void setLengthBeats(double beats);
    void setDivisionBeats(double beats);
    void setNoteBeats(std::optional<double> beats);

    void setCursor(size_t noteIndex);

    void trigger();
    void release();
    void stop();
    void reset();

    bool advance();

    bool isActive() const;
    bool isSustaining() const;
    size_t currentIndex() const;
    const PhonemeSpec * phoneme() const;
    const PhonemeSpec * nextPhoneme() const;
    double phonemeSeconds() const;
    double phonemeElapsedSeconds() const;
    bool isStressed() const;
    double progress() const;
    double utteranceProgress() const;

    size_t syllableCursor() const;
    size_t syllableCount() const;

    size_t lineCursor() const;
    size_t lineCount() const;

private:
    void beginUtterance(size_t from, size_t to);
    void computeDurations(size_t from, size_t to);
    size_t nominalFrames(const PhonemeEvent & event) const;
    double beatFrames() const;
    bool sustainsCurrent() const;
    bool holdsDiphthongNucleus() const;
    size_t diphthongHoldFrame() const;
    size_t trimTrailingSilence(size_t from, size_t to) const;

    static constexpr double DiphthongHoldFraction { 0.25 };

    double m_sampleRate { 48000.0 };
    double m_bpm { 120.0 };

    PhonemeEventList m_phonemes {};
    std::vector<size_t> m_syllableStarts {};
    std::vector<size_t> m_lineStarts {};
    std::vector<size_t> m_durations {};

    TriggerMode m_triggerMode { TriggerMode::Phrase };
    SyncMode m_syncMode { SyncMode::Free };
    double m_rate { 1.0 };
    double m_lengthBeats { 4.0 };
    double m_divisionBeats { 0.5 };
    std::optional<double> m_noteBeats {};

    size_t m_start { 0 };
    size_t m_index { 0 };
    size_t m_end { 0 };
    size_t m_frame { 0 };
    size_t m_duration { 0 };
    size_t m_utteranceFrame { 0 };
    size_t m_utteranceFrames { 0 };
    size_t m_syllableCursor { 0 };
    size_t m_lineCursor { 0 };

    bool m_active { false };
    bool m_held { false };
    bool m_sustaining { false };
};

} // namespace noteahead

#endif // SPEECH_SEQUENCER_V2_HPP
