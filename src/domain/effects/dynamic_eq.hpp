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

#ifndef DYNAMIC_EQ_HPP
#define DYNAMIC_EQ_HPP

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/compressor_core.hpp"
#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

namespace noteahead {

//! One EQ band whose gain is not set but earned: it stays flat until a detector goes over the
//! threshold, and then moves towards Range by as much as a compressor would pull the level down,
//! never further. A negative Range cuts what gets too loud -- a sibilant, a resonance, the bass
//! under every kick -- and a positive one brings something up only while it is playing hard.
//!
//! The detector listens to this signal or to another device's, through a band-pass at the band's
//! own frequency or to the whole of it. The Flat type moves the whole signal instead of a band,
//! which makes it a ducker driven by a band of whatever it listens to.
class DynamicEq : public Effect
{
public:
    DynamicEq();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    std::optional<size_t> sidechainSourceDeviceIndex() const override;
    //! Settled once the band is back at rest: a device skipped for being silent would otherwise
    //! freeze it part way through its release.
    bool isSettled() const override;

    const EffectPresetList & factoryPresets() const override;

    enum class BandType
    {
        Bell,
        LowShelf,
        HighShelf,
        //! The whole signal, not a band of it.
        Flat
    };
    enum class DetectorSource
    {
        //! This signal.
        Self,
        //! The device the side-chain selector names.
        External
    };
    enum class DetectorFilter
    {
        //! Through a band-pass at the band's frequency and Q: reacting only to its own range.
        Band,
        Wide
    };
    enum class StereoMode
    {
        Stereo,
        Mid,
        Side
    };

    static constexpr double MaxRangeDb { 24.0 };
    static constexpr double MaxOutputGainDb { 12.0 };

    //! The gain the band is at now, in dB, for the dialog.
    float currentGainDb() const;
    //! The peak of what the detector hears, after its filters, in dB, falling back slowly enough to
    //! be read against the threshold. -120 dB for silence.
    float detectorLevelDb() const;
    //! The band's gain over the last couple of seconds, oldest first, one point every few
    //! milliseconds: the attack and release as they happen.
    std::vector<float> gainHistory() const;
    //! The history costs a little, so it is kept only while a dialog is showing it.
    void setAnalysisEnabled(bool enabled);

    //! What the band does at each of @p frequencies with its gain at @p gainDb, in dB, the output
    //! gain included. Drawn twice: at the full Range as the outline, and at the current gain.
    std::vector<double> magnitudesDbAt(std::span<const double> frequencies, double gainDb) const;
    //! Range in dB, from the parameter.
    double rangeDb() const;

    // Parameter conversions, shared with the presets and the dialog
    static double frequencyHz(float position);
    static float frequencyPosition(double hz);
    static double qValue(float position);
    static float qPosition(double q);

private:
    void updateBuffers();
    void syncParameters();
    void updateBand(double gainDb);
    void processFrame(double & left, double & right, const double * external);

    BandType m_bandType { BandType::Bell };
    double m_frequency { 1000.0 };
    double m_q { 1.4142 };
    double m_rangeDb { -6.0 };
    DetectorSource m_detectorSource { DetectorSource::Self };
    DetectorFilter m_detectorFilter { DetectorFilter::Band };
    StereoMode m_stereoMode { StereoMode::Stereo };
    bool m_listen { false };
    double m_outputGain { 1.0 };
    std::optional<size_t> m_sidechainSourceDevice;

    CompressorCore m_core;
    std::array<SvfFilter, 2> m_band;
    std::array<SvfFilter, 2> m_detectorBandPass;
    //! The Compressor's side-chain low-pass, ahead of the band-pass, off at the top of its travel.
    std::array<CascadedSvf, 2> m_detectorLpf;
    float m_detectorLpfCutoff { 1.0f };
    double m_gainDb { 0.0 };
    double m_appliedGainDb { 0.0 };
    uint32_t m_framesSinceUpdate { 0 };
    std::atomic<float> m_publishedGainDb { 0.0f };
    double m_detectorPeak { 0.0 };
    double m_detectorPeakDecay { 0.0 };
    std::atomic<float> m_publishedDetectorDb { -120.0f };

    std::atomic<bool> m_analysisEnabled { false };
    std::vector<float> m_history;
    size_t m_historyIndex { 0 };
    uint32_t m_historyCountdown { 0 };
    uint32_t m_historyInterval { 1 };
    mutable std::mutex m_historyMutex;
    std::vector<float> m_publishedHistory;

    uint32_t m_lastSampleRate { 0 };
    bool m_shouldUpdateBuffers { true };
};

} // namespace noteahead

#endif // DYNAMIC_EQ_HPP
