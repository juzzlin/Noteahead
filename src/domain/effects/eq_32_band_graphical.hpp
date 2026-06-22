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

#ifndef EQ_32_BAND_GRAPHICAL_HPP
#define EQ_32_BAND_GRAPHICAL_HPP

#include "../dsp/graphic_eq_designer.hpp"
#include "../dsp/linear_phase_fir.hpp"
#include "../dsp/matched_bell_filter.hpp"
#include "effect.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace noteahead {

//! A third-octave graphic equalizer for mastering: 32 sliders from 16 Hz to 20 kHz, +-12 dB each,
//! whose response actually passes through the slider positions. See GraphicEqDesigner.
class Eq32BandGraphical : public Effect
{
public:
    static constexpr size_t BandCount { GraphicEqDesigner::BandCount };
    static constexpr double MaxGainDb { 12.0 };

    Eq32BandGraphical();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    //! Holds on to a tail as long as the linear-phase filter does, which is about a second: the
    //! engine would otherwise stop a silent device with the end of its last note still inside, and
    //! play it at the start of the next one.
    bool isSettled() const override;
    //! Half the linear-phase kernel and one partition while Linear Phase is on, nothing otherwise.
    size_t latencyFrames(double sampleRate) const override;
    //! What latencyFrames() comes to at @p sampleRate with Linear Phase on, for the dialog to show
    //! before the mode is engaged.
    static size_t linearPhaseLatencyFrames(double sampleRate);

    const EffectPresetList & factoryPresets() const override;

    enum class StereoMode
    {
        //! Both channels, which comes to the same thing as left and right each on their own.
        Stereo,
        //! Only the centre of the image; the side passes through.
        Mid,
        //! Only the side; the centre passes through.
        Side
    };

    //! What the equalizer does to whichever path it shapes at each of @p frequencies, in dB,
    //! the output gain included. Asked for all at once because each call designs the bands from
    //! the parameters afresh -- they belong to the audio thread, and are only brought up to date
    //! when a block is processed -- and the design is the expensive part.
    std::vector<double> magnitudesDbAt(std::span<const double> frequencies) const;
    double magnitudeDbAt(double frequency) const;

    //! The ISO nominal frequency @p band is labelled with, "31.5 Hz" or "1.25 kHz".
    static std::string nominalLabel(size_t band);
    //! A slider's gain in dB, from its parameter value.
    static double sliderGainDb(float parameterValue);
    //! The parameter value that puts a slider at @p gainDb.
    static float sliderParameterValue(double gainDb);

private:
    struct Settings
    {
        GraphicEqDesigner::Gains filterGainsDb {};
        StereoMode stereoMode { StereoMode::Stereo };
        double outputGain { 1.0 };
        bool linearPhase { false };
    };

    Settings settingsFromParameters() const;

    void updateBuffers();
    void syncParameters();
    void processFrame(double & left, double & right);
    void processFrameLinearPhase(double & left, double & right);
    void startLinearPhaseDesign();

    //! The bands of one path through the equalizer.
    using Bands = std::array<MatchedBellFilter, BandCount>;
    static double processPath(Bands & bands, double input);

    //! Left and right in Stereo mode, mid and side otherwise.
    std::array<Bands, 2> m_paths;
    StereoMode m_stereoMode { StereoMode::Stereo };
    double m_outputGain { 1.0 };

    bool m_linearPhase { false };
    LinearPhaseFir m_fir;
    //! True from a reset, or from switching Linear Phase on, until audio has gone through. Engaging
    //! starts the filter empty, so nothing of the minimum-phase path's signal is replayed late.
    bool m_firIsIdle { true };
    //! The path Mid or Side mode leaves alone, delayed to stay in time with the filtered one.
    std::vector<double> m_bypassDelay;
    size_t m_bypassIndex { 0 };
    //! Frames since the input last carried anything, against the filter's memory.
    size_t m_quietFrames { 0 };

    uint32_t m_lastSampleRate { 0 };
    bool m_shouldUpdateBuffers { true };
    bool m_shouldSyncParameters { true };
};

} // namespace noteahead

#endif // EQ_32_BAND_GRAPHICAL_HPP
