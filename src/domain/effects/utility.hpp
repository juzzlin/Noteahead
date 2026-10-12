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

#ifndef UTILITY_HPP
#define UTILITY_HPP

#include "../dsp/dc_blocker.hpp"
#include "../dsp/linkwitz_riley_crossover.hpp"
#include "effect.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace noteahead {

//! The plain tools for putting a signal right against another one: polarity, channels, width, mono
//! below a frequency, a few milliseconds of delay, a trim and a DC block -- and the Phase Scope that
//! shows whether it is right yet.
//!
//! What it is for is the low end. A kick and a bass that hit together either add up or take each
//! other away, depending on how their waves line up, and which of the two it is cannot be heard
//! from either on its own. The scope captures this signal against a reference -- the device the
//! side-chain selector names, usually the kick -- on every hit of the reference, so the two waves
//! can be compared standing still, and reads out how alike they are and what summing them does.
//! Polarity, Delay, or an All-Pass Filter ahead of this effect, are then the levers that move it.
//!
//! The delay is the signal moving, not latency: it is never reported as latency, so a render does
//! not take it back out the way it does a linear-phase filter's.
class Utility : public Effect
{
public:
    Utility();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    std::optional<size_t> sidechainSourceDeviceIndex() const override;
    //! Unsettled while the delay still holds signal, and while the scope is open: a device skipped
    //! for being silent does not run its racks, and the scope would freeze on the last thing it saw.
    bool isSettled() const override;

    enum class ChannelMode
    {
        Stereo,
        Left,
        Right,
        Swap,
        Mono
    };

    static constexpr double MaxDelayMs { 20.0 };
    static constexpr double MaxGainDb { 24.0 };

    //! The scope costs nothing while no dialog is showing it, so it is gated rather than always run.
    void setAnalysisEnabled(bool enabled);

    //! How this signal stands against the reference, smoothed over a few hundred milliseconds.
    struct Reading
    {
        //! Whether there is a reference to compare with at all.
        bool hasReference { false };
        //! From -1 to 1: alike at 1, opposed below 0, where summing the two takes level away.
        float correlation { 0.0f };
        float referenceDb { -100.0f };
        float signalDb { -100.0f };
        //! Level of the two summed, against the level they would sum to if they were unrelated:
        //! about +3 dB when they reinforce, 0 when unrelated, and below 0 when they cancel.
        float sumGainDb { 0.0f };
    };

    Reading reading() const;

    //! The latest hit, captured from where the reference rose: both waves over the same span, so
    //! that the one can be read against the other. Empty reference without a side-chain source.
    struct Capture
    {
        std::vector<float> reference;
        std::vector<float> signal;
    };

    Capture capture() const;

    //! The delay in whole samples at @p sampleRate, which is how it is applied.
    static uint32_t delaySamples(float parameterValue, double sampleRate);

private:
    void updateBuffers();
    void syncParameters();
    void processFrame(double & left, double & right);
    void analyse(double referenceSample, double signalSample, bool hasReference);

    // Parameters as the audio thread uses them
    double m_gain { 1.0 };
    bool m_invertLeft { false };
    bool m_invertRight { false };
    ChannelMode m_channelMode { ChannelMode::Stereo };
    double m_width { 1.0 };
    bool m_monoBass { false };
    bool m_dcBlock { false };
    uint32_t m_delaySamples { 0 };
    std::optional<size_t> m_sidechainSourceDevice;

    std::array<DcBlocker, 2> m_dcBlockers;
    LinkwitzRileyCrossover m_sideSplit;
    std::array<std::vector<double>, 2> m_delayLines;
    size_t m_delayIndex { 0 };
    size_t m_quietFrames { 0 };

    // The scope
    std::atomic<bool> m_analysisEnabled { false };
    double m_productSum { 0.0 };
    double m_referenceSquare { 0.0 };
    double m_signalSquare { 0.0 };
    double m_sumSquare { 0.0 };
    double m_smoothing { 0.0 };
    double m_fastRelease { 0.0 };
    double m_slowCoefficient { 0.0 };
    size_t m_captureLength { 0 };
    size_t m_holdOffLength { 0 };
    bool m_captureReady { false };
    double m_fastEnvelope { 0.0 };
    double m_slowEnvelope { 0.0 };
    size_t m_holdOff { 0 };
    size_t m_captured { 0 };
    bool m_capturing { false };
    Capture m_pending;
    mutable std::mutex m_publishMutex;
    Reading m_reading;
    Capture m_capture;

    uint32_t m_lastSampleRate { 0 };
    bool m_shouldUpdateBuffers { true };
};

} // namespace noteahead

#endif // UTILITY_HPP
