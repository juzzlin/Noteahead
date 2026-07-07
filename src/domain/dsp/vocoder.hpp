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

#ifndef VOCODER_HPP
#define VOCODER_HPP

#include "cascaded_svf.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

class Vocoder
{
public:
    Vocoder()
    {
        for (size_t i { 0 }; i < 10; ++i) {
            m_analysisL[i].setMode(CascadedSvf::Mode::BandPass);
            m_analysisL[i].setOrder(2);
            m_analysisR[i].setMode(CascadedSvf::Mode::BandPass);
            m_analysisR[i].setOrder(2);

            m_synthesisL[i].setMode(CascadedSvf::Mode::BandPass);
            m_synthesisL[i].setOrder(2);
            m_synthesisR[i].setMode(CascadedSvf::Mode::BandPass);
            m_synthesisR[i].setOrder(2);
        }
    }

    void setSampleRate(double sampleRate)
    {
        m_sampleRate = sampleRate;
        updateFilters();
    }

    void process(double carrierL, double carrierR, double modulatorL, double modulatorR, double & outL, double & outR)
    {
        if (m_sampleRate <= 0.0) {
            outL = carrierL;
            outR = carrierR;
            return;
        }

        double sumL { 0.0 };
        double sumR { 0.0 };

        for (size_t i { 0 }; i < 10; ++i) {
            // Modulator band signals
            const double modBandL { m_analysisL[i].process(modulatorL) };
            const double modBandR { m_analysisR[i].process(modulatorR) };

            // Envelope followers (rectifier + simple 1-pole lowpass)
            const double rectL { std::abs(modBandL) };
            const double rectR { std::abs(modBandR) };

            // Lowpass coefficient for ~50 Hz cutoff
            const double coeff { 0.01 }; // simple smoothing
            m_envL[i] += (rectL - m_envL[i]) * coeff;
            m_envR[i] += (rectR - m_envR[i]) * coeff;

            // Synthesis band signals
            const double synBandL { m_synthesisL[i].process(carrierL) };
            const double synBandR { m_synthesisR[i].process(carrierR) };

            // Modulate synthesis bands by modulator envelope
            sumL += synBandL * m_envL[i] * 12.0; // gain boost for synthesis filter scaling
            sumR += synBandR * m_envR[i] * 12.0;
        }

        outL = sumL;
        outR = sumR;
    }

    void reset()
    {
        for (size_t i { 0 }; i < 10; ++i) {
            m_analysisL[i].reset();
            m_analysisR[i].reset();
            m_synthesisL[i].reset();
            m_synthesisR[i].reset();
            m_envL[i] = 0.0;
            m_envR[i] = 0.0;
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

        const std::array<double, 10> freqs { 150.0, 250.0, 400.0, 600.0, 900.0, 1300.0, 1900.0, 2800.0, 4000.0, 5600.0 };
        for (size_t i { 0 }; i < 10; ++i) {
            m_analysisL[i].setSampleRate(m_sampleRate);
            m_analysisR[i].setSampleRate(m_sampleRate);
            m_synthesisL[i].setSampleRate(m_sampleRate);
            m_synthesisR[i].setSampleRate(m_sampleRate);

            setFilterFrequency(m_analysisL[i], freqs[i]);
            setFilterFrequency(m_analysisR[i], freqs[i]);
            setFilterFrequency(m_synthesisL[i], freqs[i]);
            setFilterFrequency(m_synthesisR[i], freqs[i]);

            setFilterQ(m_analysisL[i], 8.0);
            setFilterQ(m_analysisR[i], 8.0);
            setFilterQ(m_synthesisL[i], 8.0);
            setFilterQ(m_synthesisR[i], 8.0);
        }
    }

    double m_sampleRate { 0.0 };
    std::array<CascadedSvf, 10> m_analysisL;
    std::array<CascadedSvf, 10> m_analysisR;
    std::array<CascadedSvf, 10> m_synthesisL;
    std::array<CascadedSvf, 10> m_synthesisR;
    std::array<double, 10> m_envL { 0.0 };
    std::array<double, 10> m_envR { 0.0 };
};

} // namespace noteahead

#endif // VOCODER_HPP
