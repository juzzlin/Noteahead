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

#ifndef STEREO_FIELD_METER_HPP
#define STEREO_FIELD_METER_HPP

#include "../dsp/linkwitz_riley_crossover.hpp"
#include "../effects/effect.hpp"
#include "audio_scope.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>

namespace noteahead {

//! Passthrough meter for everything about a mix that a level meter cannot show: how wide it is, how
//! much of it survives a fold-down to mono, and where in the spectrum the width actually lives.
//!
//! Width is the one thing in a mix that is routinely set by ear alone, and the ear is the wrong
//! instrument for it: a pair of speakers in one room flatters a stereo field that a phone, a club
//! system or a lone laptop speaker will collapse. What collapses is whatever is out of phase
//! between the channels, and nothing about that is audible until it is gone.
//!
//! Three readings, from the coarsest to the most specific:
//!
//! Correlation is the single number: 1 is mono, 0 is fully decorrelated, and anything sustained
//! below 0 cancels when the two channels are summed. Per-band correlation says where that is
//! happening, since a mix is usually only in trouble at one end of the spectrum -- most often the
//! bottom, where width costs the most and buys the least. Mid and Side levels say how much of the
//! signal is actually off-centre, which is what decides whether width is worth spending headroom
//! on at all.
//!
//! The goniometer trace is the same information with nothing averaged away: it shows the shape of
//! the field rather than a number describing it, which is what catches a single wide element in an
//! otherwise narrow mix.
class StereoFieldMeter : public Effect
{
public:
    StereoFieldMeter();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    //! Analysis costs nothing while no dialog is showing it, so it is gated rather than always run.
    void setAnalysisEnabled(bool enabled);

    static constexpr size_t NumBands = 3;

    //! One frame's worth of everything the dialog draws, taken together so that the numbers it
    //! shows all describe the same moment.
    struct Reading
    {
        //! Broadband, from -1 to 1.
        float correlation { 1.0f };
        //! The same, per band, low to high.
        std::array<float, NumBands> bandCorrelation { 1.0f, 1.0f, 1.0f };
        float midDb { -100.0f };
        float sideDb { -100.0f };
        //! Where the energy sits between the channels: -1 is hard left, 1 is hard right.
        float balance { 0.0f };
    };

    Reading reading() const;

    //! Recent sample pairs for the goniometer, decimated to at most maxPoints per channel.
    AudioScope::Snapshot trace(size_t maxPoints) const;

private:
    //! Smoothed running statistics of a pair of signals: how alike they are, and how big each of
    //! them is. Deliberately not named for left and right -- the same arithmetic answers the
    //! question for mid and side, which is where the level readings come from.
    struct PairMeter
    {
        double productAb { 0.0 };
        double squareA { 0.0 };
        double squareB { 0.0 };

        void update(double a, double b, double coefficient);
        float correlation() const;
        double rmsA() const;
        double rmsB() const;
        void reset();
    };

    //! One channel's split into the three bands the per-band correlations are measured over.
    //!
    //! Unlike the Stereo Widener's splitter this one has no all-pass to compensate with: the bands
    //! are never summed back, only measured, and both channels meet identical filters, so whatever
    //! phase the crossovers introduce is common to the pair and cancels out of the correlation.
    struct Splitter
    {
        LinkwitzRileyCrossover lowSplit;
        LinkwitzRileyCrossover highSplit;

        void setCutoffs(double lowerFrequency, double upperFrequency, double sampleRate);
        void split(double input, std::array<double, NumBands> & bands);
        void reset();
    };

    void updateState();
    void syncParameters();
    void analyse(double left, double right);

    std::atomic<bool> m_analysisEnabled { false };

    AudioScope m_scope;

    //! Left against right, which answers both correlation and balance.
    PairMeter m_broadband;
    std::array<PairMeter, NumBands> m_bands;
    //! Mid against side, which is where the two level readings come from.
    PairMeter m_midSide;

    Splitter m_splitterL;
    Splitter m_splitterR;

    mutable std::mutex m_readingMutex;
    Reading m_reading;

    double m_meterCoefficient { 0.0 };
    bool m_shouldSyncParameters { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // STEREO_FIELD_METER_HPP
