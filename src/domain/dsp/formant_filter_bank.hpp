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

namespace noteahead {

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

        double maleSum { 0.0 };
        for (auto & f : m_filtersMale) {
            maleSum += f.process(maleIn);
        }
        maleOut = maleSum * 0.45;

        double femaleSum { 0.0 };
        for (auto & f : m_filtersFemale) {
            femaleSum += f.process(femaleIn);
        }
        femaleOut = femaleSum * 0.45;
    }

    void reset()
    {
        for (auto & f : m_filtersMale) {
            f.reset();
        }
        for (auto & f : m_filtersFemale) {
            f.reset();
        }
    }

private:
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

        // Standard formant frequency filters (vowels like "aah"/"ooh")
        const std::array<double, 3> freqsMale { 350.0, 750.0, 1300.0 };
        const std::array<double, 3> qsMale { 8.0, 6.0, 5.0 };

        const std::array<double, 3> freqsFemale { 650.0, 1700.0, 2500.0 };
        const std::array<double, 3> qsFemale { 8.0, 6.0, 5.0 };

        for (size_t i { 0 }; i < 3; ++i) {
            m_filtersMale[i].setSampleRate(m_sampleRate);
            setFilterFrequency(m_filtersMale[i], freqsMale[i]);
            setFilterQ(m_filtersMale[i], qsMale[i]);

            m_filtersFemale[i].setSampleRate(m_sampleRate);
            setFilterFrequency(m_filtersFemale[i], freqsFemale[i]);
            setFilterQ(m_filtersFemale[i], qsFemale[i]);
        }
    }

    double m_sampleRate { 0.0 };
    std::array<CascadedSvf, 3> m_filtersMale;
    std::array<CascadedSvf, 3> m_filtersFemale;
};

} // namespace noteahead

#endif // FORMANT_FILTER_BANK_HPP
