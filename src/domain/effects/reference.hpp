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

#ifndef REFERENCE_HPP
#define REFERENCE_HPP

#include "../dsp/compressor_core.hpp"
#include "../dsp/one_pole_filter.hpp"
#include "../dsp/svf_filter.hpp"
#include "effect.hpp"
#include "reference_environments.hpp"

#include <array>
#include <vector>

namespace noteahead {

//! Plays the mix the way another system would: a car, a phone, a club.
//!
//! A mix is signed off in one room and then heard everywhere else. This is the cheap way of visiting
//! those places: band limits, voicing, the box or room ringing, the dynamics that system applies,
//! and how much of the stereo image it keeps.
//!
//! Indicative, not exact. A parametric model of a car is not a car, and the point is not to fake one
//! but to make the decisions a car would force -- whether the bass balance survives losing an octave,
//! whether the vocal carries once the midrange is scooped -- without leaving the room.
//!
//! Monitoring only, like Monitor: an offline render passes through untouched, so the check can never
//! print into an export.
class Reference : public Effect
{
public:
    Reference();

    static std::string typeIdString();

    std::string type() const override;
    std::string typeId() const override;

    void sync() override;
    void reset() override;

    const EffectPresetList & factoryPresets() const override;

    //! The environment currently selected, for the dialog and for tests.
    const ReferenceEnvironment & environment() const;

protected:
    void processSample(double & left, double & right) override;

    void processBlock(AudioContext & context) override;

private:
    void syncParameters();
    void updateState();
    void updateFilters();

    //! The reflections of one channel, as a delay buffer read at the environment's taps.
    struct Reflections
    {
        std::vector<double> buffer;
        size_t writePos { 0 };
        std::vector<OnePoleFilter> damping;

        void reset();
        double process(double input, const ReferenceEnvironment & environment, double sampleRate, double dampingHz);
    };

    //! One channel's shaping: the band limits and up to four voicing bells.
    struct Voicing
    {
        SvfFilter highPass;
        SvfFilter lowPass;
        std::array<SvfFilter, 4> bands;

        void reset();
        double process(double input, size_t bandCount);
    };

    size_t m_environmentIndex { 0 };
    float m_amount { 1.0f };
    float m_room { 1.0f };
    float m_dynamics { 1.0f };
    float m_outputDb { 0.0f };

    Voicing m_voicingL;
    Voicing m_voicingR;
    Reflections m_reflectionsL;
    Reflections m_reflectionsR;
    CompressorCore m_compressor;

    double m_lastSampleRate { -1.0 };
    size_t m_lastEnvironmentIndex { std::numeric_limits<size_t>::max() };
    bool m_shouldSyncParameters { true };
};

} // namespace noteahead

#endif // REFERENCE_HPP
