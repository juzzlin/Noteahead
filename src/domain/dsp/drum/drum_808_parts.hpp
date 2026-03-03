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

#ifndef DRUM_808_PARTS_HPP
#define DRUM_808_PARTS_HPP

#include <array>
#include <complex>
#include <cstdint>
#include <initializer_list>

//! What the TR-808 drums of Drum Synth 808 are built from. Every filter is the biquad their fits
//! were made with, so a fitted constant means the same thing here as it did there.
namespace noteahead::Drum808 {

//! A biquad of the audio cookbook's: a band pass whose peak is at unity, or a high or low pass
class Biquad
{
public:
    void setBandPass(double hz, double q, double sampleRate);
    void setHighPass(double hz, double q, double sampleRate);
    void setLowPass(double hz, double q, double sampleRate);
    //! Lifts or cuts what is below @p hz by @p gainDb, with the cookbook's Q for a shelf
    void setLowShelf(double hz, double gainDb, double q, double sampleRate);
    double process(double input);
    void reset();

private:
    double m_b0 { 1.0 };
    double m_b1 { 0.0 };
    double m_b2 { 0.0 };
    double m_a1 { 0.0 };
    double m_a2 { 0.0 };
    double m_x1 { 0.0 };
    double m_x2 { 0.0 };
    double m_y1 { 0.0 };
    double m_y2 { 0.0 };
};

//! White noise at unit deviation, the same sequence after every reseed
class Noise
{
public:
    explicit Noise(uint32_t seed);
    void reseed();
    double next();

private:
    uint32_t m_seed;
    uint32_t m_state;
};

//! A sound that rings and dies: amplitude * sin(2 pi hz t + phase) * exp(-t / seconds)
struct Mode
{
    double hz;
    double seconds;
    double amplitude;
    double phase;
};

//! One Mode played a sample at a time, as a phasor turned and shrunk by the same step each sample
class ModeOscillator
{
public:
    void start(const Mode & mode, double sampleRate);
    double next();
    void stop();

private:
    std::complex<double> m_state {};
    std::complex<double> m_step { 1.0, 0.0 };
};

//! The six square waves the 808 makes its hats, cymbal and cowbell of. Measured on the RD-8: odd
//! harmonics of these explain 94-96 % of the peak power of its hats and cymbal.
class SquareBank
{
public:
    static constexpr std::array<double, 6> FrequenciesHz { 334.15, 442.25, 518.1, 540.2, 799.0, 813.15 };

    //! The squares this bank sums, as bits of FrequenciesHz: the cowbell has only 540 and 799 Hz
    explicit SquareBank(uint8_t mask);
    void setSampleRate(double sampleRate);
    //! Where the free running oscillators happen to be when a drum is struck
    void scramble(uint32_t seed);
    //! The squares, each from -0.5 to 0.5, averaged
    double next();

private:
    uint8_t m_mask;
    double m_count { 0.0 };
    std::array<double, 6> m_phase {};
    std::array<double, 6> m_step {};
};

//! What one of the RD-8's knobs does, as measured at five of its positions: a smooth curve through
//! them that never overshoots between two of them, flat beyond the ends
struct KnobCurve
{
    std::array<double, 5> knobs;
    std::array<double, 5> values;

    double at(float knob) const;
    //! For a curve of ratios: interpolated between their logarithms, so the steps are relative
    double ratioAt(float knob) const;
};

//! How much white noise at unit deviation comes out of @p filters in a row, worked out on copies so
//! the filters themselves are left as they were
double impulseDeviation(std::initializer_list<const Biquad *> filters);

} // namespace noteahead::Drum808

#endif // DRUM_808_PARTS_HPP
