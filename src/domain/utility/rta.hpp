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

#ifndef RTA_HPP
#define RTA_HPP

#include "../effects/effect.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

namespace noteahead {

//! FFT-based real-time spectrum analyzer, passthrough insert effect.
//!
//! Analysis is multi-resolution: six windows run at once, each half the length of the one above it,
//! and every band is measured by the shortest of them that still resolves it. What that buys is the
//! responsiveness the display is read for. A band's width grows with its centre frequency, so a bar
//! at 5 kHz needs a hundredth of the frequency resolution a bar at 30 Hz does, and making it wait
//! for the window the bottom octave needs is what makes an analyzer feel slow. One window for
//! everything would have to be the longest one; here the top of the spectrum answers in twenty
//! milliseconds while the bottom keeps the resolution it cannot do without.
//!
//! Halving rather than quartering, six deep rather than three. Both follow from what a bar's
//! steadiness costs: a band spanning half as many bins is twice as restless on anything noise-like,
//! and where two neighbouring bars are measured by different windows that difference draws a step
//! at the crossover which belongs to the analyzer rather than to the music. Closely spaced
//! resolutions keep that difference small enough not to read as one.
//!
//! The longest window scales with band count, and the rest follow from it:
//!   32 bands       → 8192 down to 256
//!   64 bands       → 16384 down to 512
//!   96, 128 bands  → 32768 down to 1024
class Rta : public Effect
{
public:
    Rta();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;

    void reset() override;
    void sync() override;

    void setAnalysisEnabled(bool enabled);

    std::vector<float> bandMagnitudesDb() const;
    std::vector<std::pair<float, float>> bandLogPositions() const;

    //! Bumped every time the bands are rebuilt, which is the only thing that moves them.
    //!
    //! The positions are a few hundred bytes that change when the band count or the sample rate
    //! does and never otherwise, while the levels change sixty times a second. Handing the drawing
    //! side a number to compare lets it copy the layout on the frames that need it rather than on
    //! every one.
    uint32_t layoutGeneration() const;

private:
    //! One analysis resolution: its window, the samples waiting to fill it, and the bands read off
    //! it. Buffers are allocated once at their largest and used from the front, so a rebuild on the
    //! audio thread only changes lengths.
    struct Tier
    {
        std::vector<double> window;
        std::vector<double> inBuf; //!< Circular, so a hop costs no shifting of what is already in.
        std::vector<double> timeBuf; //!< The window applied to it, in order, ready to transform.
        std::vector<double> fftRe;
        std::vector<double> fftIm;
        std::vector<int> bands; //!< Indices into m_bandBins measured at this resolution.
        int fftN = 0;
        int specBins = 0;
        int hopSize = 0;
        int hopFill = 0;
        int writePos = 0;
    };

    //! Six resolutions span a factor of 32 in window length, which covers the ratio between the
    //! widest and narrowest band of a log display without leaving a gap a band can fall into.
    static constexpr int TierCount = 6;
    //! Longest window the 96- and 128-band modes ask for; every buffer is allocated for it.
    static constexpr int MaxTierFftSize = 32768;
    //! Each tier resolves half as finely as the one above, and answers twice as soon.
    static constexpr int TierSizeRatio = 2;
    //! Shortest window any tier is allowed, so the smallest band count keeps a usable spectrum.
    static constexpr int MinTierFftSize = 256;
    //! Room for the per-bin weights of every band. Generous: a band takes a handful of bins, and
    //! this is reserved once so that a rebuild on the audio thread cannot allocate.
    static constexpr int MaxBandWeights = 8192;
    //! Bins a band must span before a shorter window is considered to resolve it. One bin would
    //! technically fill the bar, but a single bin per band is a noisy estimate: asking for two keeps
    //! the shorter windows from making the display restless in exchange for the speed they buy.
    static constexpr double MinBinsPerBand = 2.0;
    //! Bounds how often a tier can run, so the shortest window cannot spend the audio thread on
    //! analyses far past the rate anything reads them at.
    static constexpr int MinHopSize = 16;

    static constexpr double FreqLo = 20.0;
    static constexpr double FreqHi = 20000.0;

    void syncParameters();
    void buildTierWindow(Tier & tier);
    void buildBands();
    void runTierAnalysis(Tier & tier);
    //! Attack and release coefficients for one tier's hop, from the Speed setting.
    void smoothingCoefficients(const Tier & tier, double & attackCoeff, double & releaseCoeff) const;

    std::array<Tier, TierCount> m_tiers;

    // Audio-thread only:
    std::vector<double> m_smoothedPow;
    std::vector<std::pair<int, int>> m_bandBins;

    //! How much of each bin in a band's range belongs to it, run together for all bands.
    //!
    //! A band edge falls between bins, and counting the bins whose centre happens to land inside it
    //! makes a band's level depend on where its edges rounded to. That rounding is worth a decibel
    //! or two when a band spans only a couple of bins, which is exactly the case either side of a
    //! change of resolution, so the two would draw a step at the crossover that is in the analyzer
    //! rather than in the music. Weighting the edge bins by the fraction that falls inside the band
    //! removes it.
    std::vector<double> m_bandWeights;
    std::vector<int> m_bandWeightOffsets;
    std::vector<std::pair<float, float>> m_bandLogX;

    //! Staging for the band levels an analysis pass produced, so that m_bandMutex is only held for
    //! the copy into m_bandDb. Reused between passes: this runs on the audio thread, which must not
    //! allocate. Sized by buildBands().
    std::vector<std::pair<int, float>> m_bandUpdates;

    // Shared between audio and UI thread (protected by m_bandMutex):
    mutable std::mutex m_bandMutex;
    std::vector<float> m_bandDb;
    std::vector<std::pair<float, float>> m_bandLogXPublic;

    std::atomic<uint32_t> m_layoutGeneration { 0 };

    double m_sampleRateCached = 44100.0;
    uint32_t m_lastSampleRate = 0;

    int m_bandCountMode = 0; // 0=32, 1=64, 2=128, 3=96
    int m_dbRangeMode = 2; // 0=-30, 1=-45, 2=-60, 3=-80
    bool m_showPinkNoise = true;
    float m_pinkNoiseLevel = -18.0f;
    int m_speedMode = 1; // 0=Fast, 1=Normal, 2=Slow
    int m_fftRateMode = 1; // 0=Fast(32x overlap), 1=Normal(16x), 2=Slow(8x)

    std::atomic<bool> m_analysisEnabled { false };
    bool m_shouldSync = false;
};

} // namespace noteahead

#endif // RTA_HPP
