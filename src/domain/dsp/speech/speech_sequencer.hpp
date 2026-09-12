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

#ifndef SPEECH_SEQUENCER_HPP
#define SPEECH_SEQUENCER_HPP

#include "text_to_phonemes.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace noteahead {

//! Decides which phoneme is being spoken and for how long. Everything about a phrase that is a
//! question of time rather than of sound.
//!
//! Kept apart from FormantVoice because the two answer different questions: the voice knows how a
//! phoneme sounds, this knows when. It is also the whole of what makes the device musical rather
//! than merely a speech synthesizer, so it is worth being able to test on its own.
class SpeechSequencer
{
public:
    enum class TriggerMode
    {
        //! A note speaks the whole phrase, at that note's pitch.
        Phrase = 0,
        //! A note speaks the next syllable. The tracker-native one: write a melody and the words
        //! land on its notes.
        Step = 1,
        //! A note speaks the next sentence, so one device carries a whole lyric.
        //!
        //! Appended rather than slotted in beside Phrase, where it belongs by size: the value is
        //! written into every project that uses the device, so the order is not free to change.
        Line = 2
    };

    enum class SyncMode
    {
        //! Natural durations, scaled by the rate. Ignores the tempo.
        Free = 0,
        //! The phrase is stretched to span a set number of beats.
        Fit = 1,
        //! Each syllable occupies exactly one division.
        Grid = 2
    };

    void setSampleRate(double sampleRate);
    void setBpm(double bpm);

    //! The compiled phrase. Allocates, so it is called when the phrase changes and never from the
    //! audio thread.
    void setPhonemes(PhonemeEventList phonemes);
    const PhonemeEventList & phonemes() const;

    void setTriggerMode(TriggerMode mode);
    void setSyncMode(SyncMode mode);
    //! Speaking speed in Free mode, as a multiplier. Above 1 is faster.
    void setRate(double rate);
    //! Beats the whole phrase spans in Fit mode.
    void setLengthBeats(double beats);
    //! Beats one syllable occupies in Grid mode.
    void setDivisionBeats(double beats);

    //! How long the note that fired this utterance lasts, in beats, when the player knows.
    //!
    //! What Fit spans in Line mode, in place of the one Length every utterance would otherwise
    //! share. A lyric's lines are not the same length as each other, so a single setting either
    //! races the long ones or drawls the short ones; the note is already the right length, and it
    //! is the thing the pattern shows.
    //!
    //! In beats rather than seconds so that a tempo change mid-line still lands: beatFrames() reads
    //! the tempo that is in effect, which is what the recompute in setBpm() relies on.
    //!
    //! Nothing without it: Length is what Fit falls back to when a note carries no length. Every
    //! note that reaches a device through a render has one, because the render closes even the
    //! notes nothing ends, so what falls back is live play -- a key held on a keyboard has no
    //! length until it is let go of, by which time the durations have long been decided.
    void setNoteBeats(std::optional<double> beats);

    //! Places the cursor at the utterance a given number of notes into the phrase.
    //!
    //! A cursor is state the song does not carry, so starting from the middle of a song would
    //! otherwise speak the wrong line under the right note. The player counts the notes that come
    //! before where it is starting and says so here.
    void setCursor(size_t noteIndex);

    //! Note on. Phrase mode rewinds to the start; Step mode speaks the syllable under the cursor
    //! and moves the cursor on, wrapping at the end of the phrase.
    void trigger();
    //! Note off. Only Step mode in Free sync is holding anything to let go of.
    void release();
    //! Stops mid-utterance, leaving the cursor where it is.
    void stop();
    //! Stops and rewinds the cursor.
    void reset();

    //! Advances by one frame. Returns whether anything is still being spoken.
    bool advance();

    bool isActive() const;
    //! The phoneme now being spoken, or nullptr when nothing is.
    const PhonemeSpec * phoneme() const;
    //! The phoneme after the current one, or nullptr at the end of the utterance. The voice needs
    //! it to aspirate a stop into what follows.
    const PhonemeSpec * nextPhoneme() const;
    //! How long the current phoneme lasts, in seconds. The voice needs it because what follows a
    //! stop's release takes a fixed time however long the phoneme has been stretched to.
    double phonemeSeconds() const;
    //! Whether the phoneme being spoken belongs to a stressed syllable.
    bool isStressed() const;
    //! How far through that phoneme, 0..1.
    double progress() const;
    //! How far through the whole utterance, 0..1. Drives the intonation contour.
    double utteranceProgress() const;

    //! Which syllable a Step-mode note would speak next.
    size_t syllableCursor() const;
    size_t syllableCount() const;

    //! Which line a Line-mode note would speak next.
    size_t lineCursor() const;
    size_t lineCount() const;

private:
    void beginUtterance(size_t from, size_t to);
    void computeDurations(size_t from, size_t to);
    //! Natural duration of one event, with its prosodic length scale applied.
    size_t nominalFrames(const PhonemeEvent & event) const;
    double beatFrames() const;
    //! Whether the current phoneme is the one a held note sustains.
    bool sustainsCurrent() const;
    //! The utterance starting at @p from, with any trailing silence left out of it.
    //!
    //! The pause a full stop leaves behind belongs between the lines rather than inside one. Left
    //! in, Grid spends a whole division on it -- a lone silence has no vowel to absorb the slack,
    //! so it takes a slot of its own -- and Fit squeezes the words to make room for it. The gap
    //! before the next line is the note's own, which the instrument's note-off offset already sets.
    size_t trimTrailingSilence(size_t from, size_t to) const;

    double m_sampleRate { 48000.0 };
    double m_bpm { 120.0 };

    PhonemeEventList m_phonemes;
    //! Indices of the syllable starts, plus nothing else. Step mode walks this.
    std::vector<size_t> m_syllableStarts;
    //! The same for the sentence starts, which Line mode walks.
    std::vector<size_t> m_lineStarts;
    //! Frames each phoneme lasts. Sized with the phrase so that starting an utterance, which
    //! happens on the audio thread, never allocates.
    std::vector<size_t> m_durations;

    TriggerMode m_triggerMode { TriggerMode::Phrase };
    SyncMode m_syncMode { SyncMode::Free };
    double m_rate { 1.0 };
    double m_lengthBeats { 4.0 };
    double m_divisionBeats { 0.5 };
    std::optional<double> m_noteBeats;

    size_t m_start { 0 };
    size_t m_index { 0 };
    size_t m_end { 0 };
    size_t m_frame { 0 };
    size_t m_duration { 0 };
    size_t m_utteranceFrame { 0 };
    size_t m_utteranceFrames { 0 };
    size_t m_syllableCursor { 0 };
    //! Kept apart from the syllable cursor rather than shared, so that nothing about Step mode
    //! moves when a phrase is written with sentences in it.
    size_t m_lineCursor { 0 };

    bool m_active { false };
    bool m_held { false };
};

} // namespace noteahead

#endif // SPEECH_SEQUENCER_HPP
