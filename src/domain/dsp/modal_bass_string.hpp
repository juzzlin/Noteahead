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

#ifndef MODAL_BASS_STRING_HPP
#define MODAL_BASS_STRING_HPP

#include <array>

namespace noteahead {

//! One string of an electric bass, as a bank of decaying phasors, one per partial.
//!
//! A phasor keeps its phase when its frequency moves, so a bend, a hammer-on or a pull-off only
//! turns the partials faster or slower: there is no delay line to retune and nothing to tear. What
//! the pluck and the pickups do to each partial is exact here too. The finger leaves a comb of
//! nulls at the harmonics that have a node where it plucked, and each pickup another where it
//! senses, both moving as a fret shortens the string.
//!
//! The constants are fitted to the partial levels of two CC0 recordings by Karoryfer: a 5-string on
//! its neck pickup with the tone rolled off (black-and-blue-basses, darkblack) and a Jazz Bass on
//! both pickups with every knob on 10 (growlybass). One model covers both to about 3.3 dB a partial.
class ModalBassString
{
public:
    //! What the device's knobs mean to a string
    struct Settings
    {
        double pluckMm { 120.0 }; //!< Where the finger plucks, from the bridge
        double fingerMm { 10.8 }; //!< How wide the finger is: a wider one rounds off the top
        double fundamentalT60 { 23.0 }; //!< Seconds the lowest partials ring
        double lossPerHz { 0.0021 }; //!< How much faster each hertz of a partial dies, 1/s
        double muteT60 { 0.15 }; //!< Seconds the fundamental takes to die under a lifted finger
        double neck { 1.0 }; //!< The neck pickup's volume
        double bridge { 1.0 }; //!< The bridge pickup's volume
    };

    static constexpr double ScaleMm = 864.0; //!< 34", the long-scale bass
    static constexpr int MaxPartials = 64;

    void setSampleRate(double sampleRate);
    void setSettings(const Settings & settings);

    //! Plucks the string stopped at @p fret, where it sounds at @p frequency. @p amplitude is the
    //! pluck's strength and @p tilt how fast its spectrum falls: a soft pluck rolls off the finger
    //! more slowly and has fewer highs.
    void pluck(double frequency, double fret, double inharmonicity, double amplitude, double tilt);

    //! A hammer-on or a pull-off: the string keeps ringing and only its length changes, with the
    //! little extra the finger landing or leaving puts in.
    void legato(double frequency, double fret, double inharmonicity, double amplitude, double tilt);

    //! The fretting hand lifts: the string dies at the mute rate.
    void mute();

    //! Bends every partial by @p ratio. Cheap to call every sample: the partials are only retuned
    //! when the ratio has moved.
    void setPitchRatio(double ratio);

    double nextSample();
    bool isActive() const;
    //! Whether the fretting hand is still on the string
    bool isHeld() const;
    void reset();

private:
    struct Partial
    {
        double re { 0.0 };
        double im { 0.0 };
        //! The rotation and decay of one sample, as a complex multiplier
        double rotRe { 1.0 };
        double rotIm { 0.0 };
        //! What is left of the last pluck, which the finger landing silences in a few ms
        double tailRe { 0.0 };
        double tailIm { 0.0 };
        double tailRotRe { 1.0 };
        double tailRotIm { 0.0 };
        double weight { 0.0 }; //!< What the pickups make of the partial
        double targetWeight { 0.0 };
        double frequency { 0.0 }; //!< Unbent
    };

    void setLength(double frequency, double fret, double inharmonicity);
    void excite(double amplitude, double tilt, double scale);
    void updatePickupWeights(bool immediate);
    void updateRotations();
    double decayRate(double frequency) const;

    std::array<Partial, MaxPartials> m_partials {};
    int m_partialCount { 0 };

    Settings m_settings {};
    double m_sampleRate { 44100.0 };
    double m_lengthMm { ScaleMm };
    double m_pitchRatio { 1.0 };
    double m_tailDecay { 0.0 };
    double m_weightSlew { 0.0 };
    bool m_held { false };
    bool m_active { false };
    int m_silenceCounter { 0 };
};

} // namespace noteahead

#endif // MODAL_BASS_STRING_HPP
