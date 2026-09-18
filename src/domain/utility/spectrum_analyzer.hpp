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

#ifndef SPECTRUM_ANALYZER_HPP
#define SPECTRUM_ANALYZER_HPP

#include <cstddef>
#include <vector>

namespace noteahead {

//! Long-term average spectrum of a finished mix, in third-octave bands.
//!
//! Not a spectrum analyzer in the Rta sense: that one answers "what is playing now", which is the
//! wrong question about balance. A mix is judged on where its energy sits averaged over the whole
//! piece, and that is what this measures -- one number per band for the entire file.
//!
//! Levels come out relative to the mix's own average across the midrange rather than as absolute
//! dBFS, so that two tracks mastered to different loudnesses can be compared directly: what is left
//! after that normalisation is the balance, which is the thing a mixing decision is about.
class SpectrumAnalyzer
{
public:
    struct Band
    {
        double centerHz { 0.0 };
        //! Level relative to this mix's own 100 Hz - 8 kHz average, in dB.
        float levelDb { 0.0f };
    };

    //! Where the energy sits, summarised. All four are relative to the same midrange average, so
    //! they say how a mix is balanced rather than how loud it is.
    struct Result
    {
        std::vector<Band> bands;
        //! Weight: what gives a mix its body.
        float lowMidDb { 0.0f };
        //! Between weight and presence, and where "mud" is blamed whether or not it lives there.
        float midDb { 0.0f };
        //! Presence: where a lead's core sits, and what a mix reads as hollow without.
        float upperMidDb { 0.0f };
        //! Brightness.
        float highDb { 0.0f };
        //! Presence against brightness, which is the single number a hollow mix gives itself away
        //! by: the two move in opposite directions and the ear reads the difference, not either one.
        float upperMidToHighDb { 0.0f };
        //! False when the file held too little above the silence gate to average anything.
        bool isValid { false };
    };

    //! @p channels is the interleaving stride of the buffers handed to process().
    explicit SpectrumAnalyzer(double sampleRate, int channels = 2);

    //! Interleaved samples, @p numSamples counting every channel.
    void process(const float * data, size_t numSamples);

    Result calculate() const;

    //! Third-octave centres this reports, 25 Hz to 16 kHz.
    static std::vector<double> bandCenters();

private:
    void pushSample(double mono);
    void analyzeFrame();

    double m_sampleRate;
    int m_channels;

    std::vector<double> m_window;
    std::vector<double> m_frame;
    size_t m_frameFill { 0 };

    std::vector<double> m_powerSum;
    size_t m_frameCount { 0 };
};

} // namespace noteahead

#endif // SPECTRUM_ANALYZER_HPP
