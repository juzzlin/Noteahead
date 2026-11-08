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

#ifndef MODAL_GRAND_STRING_HPP
#define MODAL_GRAND_STRING_HPP

#include <array>
#include <cstdint>

namespace noteahead {

//! One key of an acoustic grand: its one, two or three strings as a bank of two-pole resonators,
//! one per partial and string, heard in stereo.
//!
//! Every constant here is fitted to a recording of a concert grand, the Yamaha CP4's CFX sample,
//! played from A0 to C8 at velocity 100. What the hammer, the strike point and the soundboard do to
//! each partial is one formula over the whole keyboard, which matches the recording's attack spectra
//! to 6.0 dB a partial, against 5.2 dB for a free curve fitted to each note on its own:
//!
//!  - The hammer: a low-pass of 2.6 poles at 860 Hz, the same on every key. It is why the attack's
//!    centroid sits at 400-600 Hz from the bass to the middle, nineteen harmonics on the lowest note
//!    and the fundamental alone at the top.
//!  - The strike point, 1/8 of the string, leaves shallow nulls at the partials with a node there.
//!  - The soundboard radiates the lowest partials weakly, below a resonance at 132 Hz.
//!
//! A key's strings are tuned a cent or so apart. Coupled at the bridge, the part of their motion
//! they share reaches the soundboard and dies fast, and the rest rings on: the two-stage decay of a
//! piano. Above middle C the fast part is most of the note, below it almost none of it.
class ModalGrandString
{
public:
    //! What the device's knobs mean to a string, at strike time
    struct Settings
    {
        float hardness { 0.5f }; //!< The hammer's corner, an octave and a half either way
        float decay { 0.5f }; //!< Scales every decay time, half to double
        float brightness { 0.5f }; //!< How fast the upper partials die, against the lower
        float inharmonicity { 0.5f }; //!< Scales the strings' stiffness, none to double
        float detune { 0.3f }; //!< How far apart a key's strings are tuned
        float stretch { 1.0f }; //!< How much of the recording's octave stretch to tune to
        float panSpread { 0.6f }; //!< How far the bass is to the left of the treble, up to hard left and right
        float velocitySensitivity { 0.6f }; //!< How much quieter soft notes are; the tone follows anyway
        float release { 0.3f }; //!< How long the damper takes
    };

    static constexpr int MaxPartials = 80;
    //! The partials split into a key's strings and a fast part. Above these the strings are heard as
    //! one, and the fast part's share only shortens their decay.
    static constexpr int UnisonPartials = 16;
    static constexpr int MaxStrings = 3;
    static constexpr int MaxModes = MaxPartials + UnisonPartials * MaxStrings;

    //! The first key without a damper: from here up a string rings whether the key is held or not
    static constexpr uint8_t LowestUndampedNote = 90;

    void setSampleRate(double sampleRate);

    //! Strikes the key. @p softPedal shifts the hammer onto fewer strings and softer felt.
    void strike(uint8_t note, float velocity, const Settings & settings, bool softPedal);
    //! The damper lands on the strings, unless the key has none
    void damp();

    //! Adds one sample of the key to @p left and @p right
    void process(double & left, double & right);

    bool isActive() const;
    uint8_t note() const;
    //! How loud the key still is, for choosing which to cut when there are too many
    double energy() const;
    int modeCount() const;
    void reset();

    //! The tuning, in cents from equal temperament at A4 = 440 Hz, that the recording's tuner
    //! stretched each key to
    static double stretchCents(int note);
    //! The strings' stiffness coefficient B, from the recording's partial frequencies
    static double inharmonicity(int note);
    //! How many strings the key has: one in the low bass, two in the tenor, three above
    static int stringCount(int note);
    //! The slow and fast decay times of the fundamental, and the fast part's share of the strike
    static double slowDecaySeconds(double frequency);
    static double fastDecaySeconds(double frequency);
    static double fastShare(double frequency);
    //! What the hammer, the strike point and the soundboard make of a partial at @p frequency
    static double partialLevel(int note, int partial, double frequency, double hammerHz);
    //! Where the key sits between the speakers, 0..1, at @p panSpread: the bass left, the treble right
    static double keyPan(int note, double panSpread);

private:
    struct Modes
    {
        std::array<double, MaxModes> a1 {};
        std::array<double, MaxModes> a2 {};
        std::array<double, MaxModes> y1 {};
        std::array<double, MaxModes> y2 {};
        std::array<double, MaxModes> left {};
        std::array<double, MaxModes> right {};
        std::array<double, MaxModes> omega {};
        std::array<double, MaxModes> sigma {};
    };

    void addMode(double frequency, double sigma, double amplitude, double pan, double spread);
    void setPole(int mode, double sigma);

    Modes m_modes;
    int m_modeCount { 0 };

    double m_sampleRate { 44100.0 };
    uint8_t m_note { 0 };
    bool m_active { false };
    bool m_damped { false };
    //! The decay rate the damper adds, per second, and how long it takes to land
    double m_damperSigma { 0.0 };
    int m_damperCountdown { 0 };
    //! Raised-cosine onset, as long as the hammer is in contact with the strings
    double m_attackPhase { 1.0 };
    double m_attackStep { 0.0 };
    double m_energy { 0.0 };
    int m_pruneCounter { 0 };
};

} // namespace noteahead

#endif // MODAL_GRAND_STRING_HPP
