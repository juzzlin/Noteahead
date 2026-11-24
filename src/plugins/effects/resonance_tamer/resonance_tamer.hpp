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

#ifndef RESONANCE_TAMER_HPP
#define RESONANCE_TAMER_HPP

#include "../../../domain/dsp/spectral_peak_detector.hpp"
#include "../../../domain/dsp/tracking_bell_bank.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../domain/effects/spectrum_source.hpp"
#include "../../../domain/utility/spectrum_taps.hpp"

#include <array>
#include <cstdint>
#include <mutex>
#include <span>
#include <vector>

namespace noteahead {

//! Finds the narrow peaks that stick out of the spectrum as the music plays -- a ringing room mode,
//! a harsh band of a vocal, the fizz of a guitar -- and pulls just those down, only while they stick
//! out. The broad balance is left alone: that is the difference from a dynamic EQ, whose bands are
//! placed by hand.
//!
//! A SpectralPeakDetector listens and a TrackingBellBank cuts, with bells in the time domain, so it
//! adds no latency and can go on any insert. The detector hears a window behind the signal, which a
//! resonance, ringing on, does not mind; a short spike gets through.
//!
//! Solo plays what it takes out.
class ResonanceTamer : public Effect, public SpectrumSource
{
public:
    ResonanceTamer();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;
    bool isSettled() const override;

    //! Pre is the input, Post the output: Pre - Post is what is being taken out.
    SpectrumTaps & spectrumTaps() override;

    const EffectPresetList & factoryPresets() const override;

    enum class StereoMode
    {
        //! One detector hears both sides, and the same cuts go on both.
        Linked,
        Stereo,
        MidSide
    };

    static constexpr double MaxDepthDb { 24.0 };
    static constexpr double MinSelectivityDb { 1.0 };
    static constexpr double MaxSelectivityDb { 18.0 };
    //! The choices of the Quality control: how many bells each detector may cut with at once.
    static constexpr std::array<int, 3> BellCounts { 6, 12, 24 };

    //! What the bells on @p path (0: the linked, left or mid one; 1: right or side) are doing at
    //! each of @p frequencies now, in dB.
    std::vector<double> reductionDbAt(std::span<const double> frequencies, size_t path) const;
    //! The deepest cut any bell is making now, in dB, negative.
    double deepestCutDb() const;
    StereoMode stereoMode() const;

    // Parameter conversions, shared with the presets, the summary and the dialog
    static double depthDb(float position);
    static float depthPosition(double db);
    static double selectivityDb(float position);
    static float selectivityPosition(double db);
    static double sharpnessQ(float position);
    static float sharpnessPosition(double q);
    static double frequencyHz(float position);
    static float frequencyPosition(double hz);
    static float attackPosition(double ms);
    static float releasePosition(double ms);

private:
    void updateBuffers();
    void syncParameters();
    void processFrame(double & left, double & right);
    void publish();

    std::array<SpectralPeakDetector, 2> m_detectors;
    std::array<TrackingBellBank, 2> m_banks;
    StereoMode m_stereoMode { StereoMode::Linked };

    uint32_t m_lastSampleRate { 0 };
    bool m_shouldUpdateBuffers { true };

    mutable std::mutex m_publishedMutex;
    std::array<std::array<TrackingBellBank::Bell, TrackingBellBank::MaxBells>, 2> m_publishedBells {};
    std::array<size_t, 2> m_publishedCounts {};
    double m_publishedQ { 4.0 };

    SpectrumTaps m_spectrumTaps;
};

} // namespace noteahead

#endif // RESONANCE_TAMER_HPP
