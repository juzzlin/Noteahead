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

#ifndef SPECTRAL_PEAK_DETECTOR_HPP
#define SPECTRAL_PEAK_DETECTOR_HPP

#include <cstddef>
#include <span>
#include <vector>

namespace noteahead {

//! Finds the narrow peaks that stand out of a signal's spectrum: a resonance, a ringing mode, a
//! harsh band. Every hop it takes a windowed FFT of the last few tens of milliseconds, smooths it a
//! little over time and frequency so that noise does not look like a peak, and compares each bin
//! with the spectrum's outline, the same spectrum averaged over an octave around the bin with the
//! bin's own neighbourhood left out. Whatever is above the outline by more than the selectivity is
//! a reduction, capped at the depth, and the strongest local maxima of the reductions are the peaks.
//!
//! Only a detector: what is done about the peaks is up to the caller. It adds no latency, but it
//! hears a window behind the signal.
class SpectralPeakDetector
{
public:
    struct Settings
    {
        //! How far a bin must stand above the outline before anything is taken out.
        double selectivityDb = 6.0;
        //! The most taken out of any one peak.
        double depthDb = 6.0;
        //! Only peaks between these, faded in and out over a third of an octave outside them.
        double lowHz = 20.0;
        double highHz = 20000.0;
        //! -1..1: lowers the selectivity towards the top and raises it towards the bottom, or the
        //! other way round.
        double focus = 0.0;
        //! Two peaks closer than this are one.
        double minSeparationOctaves = 0.25;
    };

    struct Peak
    {
        double frequencyHz = 0.0;
        //! Positive: how far the peak is to be pulled down.
        double reductionDb = 0.0;
    };

    static constexpr size_t MaxPeaks { 24 };
    static constexpr size_t MaxChannels { 2 };

    //! Allocates. Not on the audio thread while it is running.
    void configure(double sampleRate);
    //! The channels push() takes, at most MaxChannels; their powers are summed, so that linked
    //! channels share one set of peaks. Clears the history.
    void setChannelCount(size_t channelCount);
    void setSettings(const Settings & settings);

    //! One frame of setChannelCount() samples. True when an analysis has just finished and
    //! peaks() is new.
    bool push(const double * frame);

    //! Strongest first.
    std::span<const Peak> peaks() const;
    //! The reduction asked of each bin by the last analysis, in dB, positive.
    std::span<const double> reductionsDb() const;
    double binFrequencyHz(size_t bin) const;
    size_t fftSize() const;
    size_t hopSize() const;

    void reset();

private:
    void analyse();
    void rebuildBinCache();
    double rangeWeight(double frequencyHz) const;

    struct BinCache
    {
        double weight { 0.0 };
        double selectivityDb { 1.0 };
        double invCount { 0.0 };
        size_t from { 0 };
        size_t to { 0 };
        size_t excludedFrom { 0 };
        size_t excludedTo { 0 };
    };

    Settings m_settings;
    double m_sampleRate { 48000.0 };
    size_t m_fftSize { 0 };
    size_t m_fftMask { 0 };
    size_t m_hopSize { 0 };
    size_t m_channelCount { 1 };
    size_t m_writeIndex { 0 };
    size_t m_framesSinceHop { 0 };
    double m_timeCoefficient { 0.0 };
    double m_powerNormalization { 1.0 };

    std::vector<double> m_ring;
    std::vector<double> m_window;
    std::vector<double> m_input;
    std::vector<double> m_re;
    std::vector<double> m_im;
    std::vector<double> m_smoothedPower;
    std::vector<double> m_levelDb;
    std::vector<double> m_prefixDb;
    std::vector<double> m_reductionDb;
    std::vector<BinCache> m_binCache;

    struct Candidate
    {
        size_t bin = 0;
        double reductionDb = 0.0;
    };

    std::vector<Candidate> m_candidates;
    std::vector<Peak> m_peaks;
};

} // namespace noteahead

#endif // SPECTRAL_PEAK_DETECTOR_HPP
