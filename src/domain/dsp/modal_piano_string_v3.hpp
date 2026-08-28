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

#ifndef MODAL_PIANO_STRING_V3_HPP
#define MODAL_PIANO_STRING_V3_HPP

#include "dsp_component.hpp"

#include <array>
#include <cstdint>

namespace noteahead {

// Modal piano string, third revision: the note is a bank of two-pole resonators, one per
// partial, each given its own frequency, starting amplitude and decay time at strike time.
//
// Kept apart from ModalPianoString rather than replacing it. The corrections below change
// what a note sounds like, and the constants around them were fitted with the faults in
// place, so a song written against the older string has to keep playing the older string.
// V2 is frozen; this is where the model moves on.
//
// What changed:
//
//  - The pickup's sign is kept. A CP-80 has no soundboard: it is a rigid harp with a piezo
//    under each string, and a piezo at a rigid termination senses force, whose partials
//    alternate in sign as (-1)^n. The strike comb keeps its own sign too, instead of being
//    taken through fabs(). Neither changes any partial's level -- only the phases they
//    arrive in, which is what the strike itself is made of.
//
//  - A partial that stands in for a unison pair carries the pair's weight. Below
//    UnisonPartials a pair is emitted at prompt + slow amplitude; above it one mode used to
//    be emitted at full amplitude, which stepped the spectrum by about six decibels in the
//    middle of the series.
//
//  - The strings come in twos, never threes, and the single-strung range reaches to F#1.
//    A CP-80 is bichord; the earlier bound at MIDI 28 came from a grand.
//
// A waveguide loop is cheaper but it decides all of those together, through one loop
// filter, which is why it cannot follow a real piano: the reference recording's partials
// decay at rates that differ by an order of magnitude within one note, its bass
// fundamental is radiated weaker than its own second partial, and every note decays in
// two distinct stages. Here each of those is simply a number per mode.
//
// The cost is one multiply-add pair per mode per sample, so the mode count is capped and
// modes are dropped from the top as they fall below hearing.
class ModalPianoStringV3 : public DspComponent
{
public:
    // What the device's controls mean to a string, passed whole at strike time so that
    // adding a control does not change the signature.
    struct Settings
    {
        float brightness { 0.5f }; // How fast the partials decay relative to the fundamental
        float decay { 0.5f }; // Scales the fundamental's decay time
        float inharmonicity { 0.5f }; // Scales the stiffness coefficient B
        float hardness { 0.5f }; // Hammer spectrum corner, i.e. strike brightness
        float detune { 0.3f }; // Unison spread, in cents, between the paired modes
        float stretch { 0.0f }; // How much of the Railsback curve to apply
        float richness { 0.6f }; // Upper bound on the partial count
        float doubleDecay { 0.5f }; // How much faster the in-phase component decays
        // Scales the ramp the strike opens with. Half is the ramp the model was fitted with, so a
        // patch that leaves it alone is unchanged; below that the note arrives more abruptly, above
        // it the hammer is felt rather than heard.
        float attack { 0.5f };
        // How much of the strike's level follows velocity. One is the whole range, which is what
        // the reference was measured at; nothing holds every note at its loudest. Only the level
        // is blended -- the hammer still hardens with velocity, so a note played softly stays
        // soft in tone while coming up in level, which is the point of the control.
        //
        // Half by default rather than the reference's own law. The measured law is right about the
        // instrument and hard to play: a treble note rings a second or two against the bass's
        // seventeen, so at equal peak the ear hears it as much the quieter and asks to be played
        // harder. Half keeps most of the dynamic range and takes that away.
        float velocitySensitivity { 0.5f };
    };

    // Partials of the note itself, before the unison pairs are added. The reference has
    // close to sixty of them still audible at the bottom of the keyboard.
    static constexpr int MaxPartials = 64;
    // How many of the lowest partials are split into a fast and a slow component. The
    // two-stage decay is only measurable where the partials are strong and long-lived.
    static constexpr int UnisonPartials = 12;
    static constexpr int MaxModes = MaxPartials + UnisonPartials;

    void setSampleRate(double sampleRate) override;

    void trigger(uint8_t note, float velocity, const Settings & settings);
    void release(float releaseTime);

    double nextSample();
    bool isActive() const;
    void reset();

    // The strike's own note, so a voice can be matched to a note-off.
    uint8_t note() const;
    // How many modes are still being computed. Falls as the note decays.
    int activeModeCount() const;

    // The topmost notes of a piano have no dampers, so lifting the key there does nothing.
    static constexpr uint8_t LowestUndampedNote = 93;

private:
    // One partial, as a two-pole resonator y[n] = a1*y[n-1] + a2*y[n-2] running free
    // after the strike set its two initial samples.
    struct Mode
    {
        double a1 { 0.0 };
        double a2 { 0.0 };
        double y1 { 0.0 };
        double y2 { 0.0 };
        // Kept so that the damper can shorten the decay without re-deriving the pitch.
        double omega { 0.0 };
        double radius { 0.0 };
    };

    // Pitch the decay time is quoted at, and how the decay shortens as the pitch rises.
    // Both measured off the reference recording; the fall is far slower than the square
    // root a plucked-string model would take.
    static constexpr double ReferenceFrequency = 261.63; // C4
    static constexpr double ReferenceDecayTime = 17.0; // Seconds, at the reference pitch
    static constexpr double DecayPitchExponent = 1.15;
    // The pitch law keeps falling past the top of the keyboard, but the reference does
    // not: its topmost two octaves all ring for about the same couple of seconds. Without
    // this the top octave dies a second too soon.
    static constexpr double MinPitchDecayTime = 1.6;
    static constexpr double MinDecayTime = 0.05;
    static constexpr double MaxDecayTime = 40.0;
    // Below this note the strings are single, and at or above it they come in twos. A CP-80 is
    // bichord over its whole compass -- it never reaches three -- and F#1 is the last note it
    // strings singly. A single string has nothing to beat or trade energy with, so it does
    // not split.
    static constexpr uint8_t LowestDoubleStrungNote = 43;
    // Corner of the second-order high-pass standing in for how weakly a soundboard
    // radiates the lowest partials. It is what leaves the bass fundamental quieter than
    // its own second partial, as the reference has it.
    static constexpr double RadiationCorner = 100.0;
    // Slope of the hammer's own spectrum above its corner, in poles.
    static constexpr double HammerSlope = 3.0;
    // Below this the mode has nothing left to say and is dropped from the loop.
    static constexpr double ModeSilenceThreshold = 1e-7;
    static constexpr double SilenceThreshold = 1e-9;
    // How often the dead modes at the top are pruned, in samples. Often enough to save
    // the work, rarely enough that the check itself costs nothing.
    static constexpr int PruneInterval = 1024;

    static double midiNoteToFreq(uint8_t note);
    // Stiffness coefficient B of the note's string, from the reference recording.
    static double inharmonicityCoefficient(uint8_t note);
    // Level of the key relative to the rest of the keyboard, as a linear factor.
    static double keyLevel(uint8_t note);
    // Offset from equal temperament, in cents, that a tuner's octave stretching produces.
    static double railsbackCents(uint8_t note);
    // Where along the string the hammer lands, as a fraction of its length.
    static double strikePosition(uint8_t note);

    // Sets a mode's coefficients from its pitch and decay time.
    void setModePole(Mode & mode, double frequency, double decayTime) const;

    // Shortens every mode's decay to what the damper allows. Called once the damper has had time
    // to land, which is not the moment the key was let go.
    void applyDamper(float releaseTime);

    std::array<Mode, MaxModes> m_modes;
    int m_modeCount { 0 };

    uint8_t m_note { 0 };
    bool m_releasing { false };

    double m_gain { 0.0 };
    // Raised-cosine ramp over the first samples of the note, so that the modes do not all
    // arrive on one sample. Its length is what the attack time is measured as.
    double m_attackPhase { 0.0 };
    double m_attackStep { 0.0 };

    // A damper has mass, and its felt has to travel and then absorb: it cannot stop a string that
    // has not finished its first cycles. Letting it land the instant the key came up truncated the
    // strike itself -- at a five millisecond note in the bass it cost 5 dB of peak and 22 dB of
    // energy, which is the difference between a staccato note and nothing much at all.
    int m_damperCountdown { 0 };
    float m_pendingReleaseTime { 0.0f };

    int m_pruneCounter { 0 };
    double m_energy { 0.0 };
};

} // namespace noteahead

#endif // MODAL_PIANO_STRING_V3_HPP
