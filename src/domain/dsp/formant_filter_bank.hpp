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

#ifndef FORMANT_FILTER_BANK_HPP
#define FORMANT_FILTER_BANK_HPP

#include "cascaded_svf.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

//! Vowel colouring for the voice registers: a glottal source tilt followed by three formant
//! resonances, one bank per register.
//!
//! The tilt is what makes the section sing rather than saw. A vocal tract is excited by a glottal
//! pulse train falling at some 12 dB per octave, while the oscillators feeding this bank are the
//! same sawtooths the strings section uses, at 6 dB. Published formant amplitudes are measured on
//! radiated speech, so they already contain that rolloff: reproducing them by weighting the
//! resonances of a bank fed at 6 dB per octave, as this did, leaves the upper formants roughly
//! 10 dB too strong and the result reads as a filtered string rather than a voice. Tilting the
//! source instead lets the resonances sit at nearly unit gain and puts the balance where speech
//! measurements put it.
//!
//! The registers sing different vowels, which is what the hardware's Male and Female voices do:
//! male an /o/ ("oh"), female an /a/ ("aah"). The female is at the usual sung-vowel values; the
//! small gain trims correct what the tilt over- or under-does at each peak.
//!
//! The male register is measured rather than tabulated. A VC340 recording of its Male 8' and 4'
//! alone, no ensemble, holding every C, puts that machine's formant region at 650 - 1050 Hz: on the
//! partials of the lowest note, which are the only ones no other note contributes to, it peaks at
//! 654 and 916 Hz and falls away either side. A textbook /u/ sits an octave below that, and against
//! the recording it left this register 20 - 32 dB short right where the hardware sings, which is
//! most of what made it read as a filtered string rather than a voice.
//!
//! Adjacent resonances are summed with alternating polarity. Summed in phase they cancel between
//! the peaks, which measured as 15 - 28 dB notches where a real vowel has valleys of 5 - 10 dB.
class FormantFilterBank
{
public:
    FormantFilterBank()
    {
        for (auto & f : m_filtersMale) {
            f.setMode(CascadedSvf::Mode::BandPass);
            f.setOrder(2);
        }
        for (auto & f : m_filtersFemale) {
            f.setMode(CascadedSvf::Mode::BandPass);
            f.setOrder(2);
        }
    }

    void setSampleRate(double sampleRate)
    {
        m_sampleRate = sampleRate;
        updateFilters();
    }

    // Male and female registers are filtered independently through their own
    // formant band-pass banks so that, e.g., a Male-only signal never picks up
    // female formant coloration (and vice versa).
    void process(double maleIn, double femaleIn, double & maleOut, double & femaleOut)
    {
        if (m_sampleRate <= 0.0) {
            maleOut = maleIn;
            femaleOut = femaleIn;
            return;
        }

        m_maleTilt += (maleIn - m_maleTilt) * m_tiltRate;
        m_femaleTilt += (femaleIn - m_femaleTilt) * m_tiltRate;

        maleOut = filterRegister(m_filtersMale, MaleFormants, m_maleTilt, MaleOutputGain);
        femaleOut = filterRegister(m_filtersFemale, FemaleFormants, m_femaleTilt, FemaleOutputGain);
    }

    void reset()
    {
        for (auto & f : m_filtersMale) {
            f.reset();
        }
        for (auto & f : m_filtersFemale) {
            f.reset();
        }
        m_maleTilt = 0.0;
        m_femaleTilt = 0.0;
    }

private:
    struct Formant
    {
        double frequency;
        double bandwidth;
        double gain;
    };

    using FormantList = std::array<Formant, 3>;

    //! Sung /u/ and /a/. Bandwidths set the Q, which is why they are given rather than a Q: a
    //! formant's bandwidth stays roughly put as its frequency moves, so the two are not the same
    //! thing to specify.
    static constexpr FormantList MaleFormants { { { 700.0, 90.0, 1.0 }, { 900.0, 90.0, 1.4 }, { 2530.0, 160.0, 1.8 } } };
    static constexpr FormantList FemaleFormants { { { 850.0, 80.0, 1.0 }, { 1220.0, 110.0, 0.9 }, { 2810.0, 180.0, 1.35 } } };

    //! Corner of the glottal tilt. A property of the pulse the folds make rather than of the note
    //! being sung, so it does not track pitch.
    static constexpr double TiltFrequency { 200.0 };

    //! Makeup, per register, measured against the equal-gain bank this replaced so that a project
    //! already using the device keeps its balance.
    //!
    //! The two differ by some 12 dB because the female register's formants sit more than an octave
    //! further up, where both the tilt and the sawtooth feeding it have already given away that
    //! much. Deriving it rather than measuring it would mean modelling the source, which is only
    //! a sawtooth by convenience.
    static constexpr double MaleOutputGain { 12.1 };
    static constexpr double FemaleOutputGain { 22.7 };

    double filterRegister(std::array<CascadedSvf, 3> & filters, const FormantList & formants, double input, double outputGain)
    {
        double sum { 0.0 };
        for (size_t i { 0 }; i < filters.size(); ++i) {
            const double polarity { (i % 2 == 0) ? 1.0 : -1.0 };
            // Each band peaks at its own Q, so dividing by that leaves the trims in charge.
            const double normalization { formants[i].bandwidth / formants[i].frequency };
            sum += polarity * formants[i].gain * normalization * filters[i].process(input);
        }
        return sum * outputGain;
    }

    void setFilterFrequency(CascadedSvf & filter, double freqHz)
    {
        const double maxFreq { std::min(20000.0, m_sampleRate * 0.49) };
        const double cutoff { std::log2(freqHz / 20.0) / std::log2(maxFreq / 20.0) };
        filter.setCutoff(cutoff);
    }

    void setFilterQ(CascadedSvf & filter, double q)
    {
        const double resonance { std::clamp(1.0 - 1.0 / (2.0 * q), 0.0, 0.99) };
        filter.setResonance(resonance);
    }

    void updateFilters()
    {
        if (m_sampleRate <= 0.0) {
            return;
        }

        m_tiltRate = 1.0 - std::exp(-2.0 * std::numbers::pi * std::min(TiltFrequency, m_sampleRate * 0.45) / m_sampleRate);

        for (size_t i { 0 }; i < 3; ++i) {
            m_filtersMale[i].setSampleRate(m_sampleRate);
            setFilterFrequency(m_filtersMale[i], MaleFormants[i].frequency);
            setFilterQ(m_filtersMale[i], MaleFormants[i].frequency / MaleFormants[i].bandwidth);

            m_filtersFemale[i].setSampleRate(m_sampleRate);
            setFilterFrequency(m_filtersFemale[i], FemaleFormants[i].frequency);
            setFilterQ(m_filtersFemale[i], FemaleFormants[i].frequency / FemaleFormants[i].bandwidth);
        }
    }

    double m_sampleRate { 0.0 };
    double m_tiltRate { 0.0 };
    double m_maleTilt { 0.0 };
    double m_femaleTilt { 0.0 };
    std::array<CascadedSvf, 3> m_filtersMale;
    std::array<CascadedSvf, 3> m_filtersFemale;
};

} // namespace noteahead

#endif // FORMANT_FILTER_BANK_HPP
