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

#ifndef ROOM_MICS_HPP
#define ROOM_MICS_HPP

#include "../../../domain/effects/effect.hpp"
#include "room_acoustics.hpp"
#include "room_convolver.hpp"
#include "room_tail.hpp"

#include <array>
#include <vector>

namespace noteahead {

//! Records the signal again in a room: plays it from a source placed in the room and picks it up
//! with three pairs of mics, Close, Room and Far, each with its own position, pattern and level.
//!
//! A sense of space that a reverb does not give: how far the source is from each mic, heard as
//! level, as the top the air took and as the time the sound took to arrive, and the room's own
//! tone, from what its walls, floor and ceiling are made of. The early part of what each mic hears
//! is worked out from the geometry, reflection by reflection (RoomAcoustics::ImpulseDesigner), and
//! played by convolution; the late tail is a feedback network matched to the room's decay
//! (RoomTail). Moving anything redesigns the responses over the next few blocks and crossfades to
//! them.
//!
//! The input's two channels are two sources, Spread apart, like a pair of speakers. The Close mics
//! sit on them unless moved, so at their defaults they hear the input itself and a little of the
//! room. Each pair is brought to the level of a mic on its source, so that the levels mix three
//! sounds of the same loudness, however far a pair stands.
class RoomMics : public Effect
{
public:
    RoomMics();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;
    bool isSettled() const override;

    const EffectPresetList & factoryPresets() const override;

    // Parameter conversions, shared with the presets, the dialog and the tests
    static constexpr double MinSizeM { 2.0 };
    static constexpr double MaxSizeM { 50.0 };
    static constexpr double MaxHeightM { 20.0 };
    static constexpr double MaxSpreadM { 10.0 };
    static constexpr double MaxCloseOffsetM { 10.0 };
    static constexpr double MaxSpacingM { 10.0 };
    static constexpr double MaxAngleDeg { 90.0 };

    static double sizeMetres(float position, double max = MaxSizeM);
    static float sizePosition(double metres, double max = MaxSizeM);
    static double closeOffsetMetres(float position);
    static float closeOffsetPosition(double metres);

    //! The room as the parameters set it.
    RoomAcoustics::RoomSettings settings() const;
    //! The parameter values that set @p settings, with @p mix.
    static std::map<std::string, float> parametersFor(const RoomAcoustics::RoomSettings & settings, float mix);

    //! Runs any design and load still under way to completion. For the tests.
    void finishDesign();

private:
    void configure();
    void syncParameters();
    //! Moves the design and the load on by what @p frames of audio pays for, or all the way.
    void advanceJobs(size_t frames, bool complete);
    void combineAndLoad();
    void renderFrame(double & left, double & right);

    RoomAcoustics::ImpulseDesigner m_designer;
    RoomConvolver m_convolver;
    RoomTail m_tail;

    //! What the parameters ask for, and what the responses playing were made of.
    RoomAcoustics::RoomSettings m_requested;
    RoomAcoustics::RoomSettings m_combined;
    //! The designer holds a finished design that is not loaded yet.
    bool m_designReady { false };
    bool m_hasDesign { false };
    bool m_hasCombined { false };

    std::array<std::vector<double>, RoomConvolver::KernelCount> m_kernels;
    double m_lateGain { 0.0 };

    bool m_shouldSyncParameters { true };
    double m_configuredSampleRate { 0.0 };
};

} // namespace noteahead

#endif // ROOM_MICS_HPP
