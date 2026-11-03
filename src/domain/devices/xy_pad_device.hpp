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

#ifndef XY_PAD_DEVICE_HPP
#define XY_PAD_DEVICE_HPP

#include "../tracker/parameter_automation.hpp"
#include "device.hpp"
#include "modulation_source.hpp"

#include <array>
#include <atomic>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

//! Four XY pads, played by hand, each axis moving one parameter anywhere in the project: a device's
//! or an effect's, whatever a parameter automation can name.
//!
//! The centre of a pad is where every target is set, so a pad at rest moves nothing. Letting go
//! springs it back to the centre over the pad's return time, or leaves it where it is when that
//! is zero.
//!
//! The pad is written from the UI thread, through atomics, and read on the audio thread in
//! evaluate(), which slews it on the same fixed grid as the LFO device so that a mouse's coarse
//! steps do not zip, and eases it home along a half cosine that arrives exactly in the return time. Where the pad stands is not saved: it is played, not set.
class XyPadDevice : public Device, public ModulationSource
{
public:
    static constexpr size_t PadCount { 4 };
    static constexpr size_t AxisCount { 2 };
    static constexpr size_t SlotCount { PadCount * AxisCount };
    //! Steps per second, as the LFO device takes them.
    static constexpr double ControlRateHz { 1000.0 };
    //! How long a pad takes to follow the hand. Short enough to feel immediate, long enough to
    //! smooth the steps a mouse moves in.
    static constexpr double SlewSeconds { 0.01 };
    //! The longest spring back, at the top of the return knob.
    static constexpr double MaxReturnSeconds { 5.0 };

    enum class Axis
    {
        X = 0,
        Y = 1
    };

    explicit XyPadDevice(std::string name);
    ~XyPadDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    static std::string typeIdString();

    bool supportsOctave() const override;
    bool producesAudio() const override;
    void processDeviceMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processDeviceMidiNoteOff(uint8_t note) override;
    void processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    //! Silence: everything this device does happens in evaluate()
    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void reset() override;
    void resetDeviceAudio() override;

    //! Every pad's described controls, grouped by pad.
    AutomatableParameterList automatableParameters() const override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    //! The slot a pad's axis is: what its steps and its offsets on a parameter are numbered by.
    static size_t slot(size_t pad, Axis axis);

    //! What @p axis of @p pad moves. An empty owner id moves nothing.
    ParameterAutomation::Target target(size_t pad, Axis axis) const;
    //! Points @p axis of @p pad at @p target. The previous target is let go of at the next step.
    void setTarget(size_t pad, Axis axis, ParameterAutomation::Target target);

    //! Writes pad @p pad's control @p key and announces a project edit. For the dialog.
    bool updatePadParameter(size_t pad, const QString & key, float value);
    //! What pad @p pad's control @p key reads, as a knob shows it, or zero where there is none.
    float padParameterValue(size_t pad, const QString & key) const;
    //! The same for a control of one axis of a pad, its depth.
    bool updateAxisParameter(size_t pad, Axis axis, const QString & key, float value);
    float axisParameterValue(size_t pad, Axis axis, const QString & key) const;

    //! The key pad @p pad's control @p key is stored under, "pad2_returnTime".
    static std::string padKey(size_t pad, const QString & key);
    //! The key an axis's own control is stored under, "pad2_y_intensity".
    static std::string axisKey(size_t pad, Axis axis, const QString & key);

    //! Puts the hand on @p pad at @p x, @p y, each from -1 to 1 with 0 at the centre and y up.
    //! Lock-free; for the UI thread.
    void press(size_t pad, float x, float y);
    //! Moves the hand on @p pad. Lock-free; for the UI thread.
    void move(size_t pad, float x, float y);
    //! Lifts the hand off @p pad: it springs back, or holds when its return time is zero.
    //! Lock-free; for the UI thread.
    void release(size_t pad);

    //! Where @p pad stands now, as the audio thread last moved it. What the crosshair shows.
    float positionX(size_t pad) const;
    float positionY(size_t pad) const;
    bool isPressed(size_t pad) const;
    //! Whether @p pad is still on its way back to the centre, or to where the hand left it.
    bool isMoving(size_t pad) const;

    //! Advances every pad over [startFrame, startFrame + frameCount) and appends the steps that
    //! change anything to @p steps. For the engine, on the audio thread, before any device renders.
    void evaluate(uint64_t startFrame, uint32_t frameCount, double sampleRate, double bpm, std::vector<Step> & steps) override;
    std::vector<ParameterAutomation::Target> modulationTargets() const override;

protected:
    void syncParameters() override;

private:
    struct AxisSlot
    {
        ParameterAutomation::Target target;
        //! A target swapped out since the last step, still carrying this axis's modulation.
        std::optional<ParameterAutomation::Target> released;
        float depth { 1.0f };
        //! What was last handed to the target, to send nothing while nothing changes.
        std::optional<std::pair<double, double>> lastSent;
    };

    struct Pad
    {
        //! Written by the UI thread: where the hand is, and whether it is on the pad.
        std::atomic<float> handX { 0.0f };
        std::atomic<float> handY { 0.0f };
        std::atomic<bool> pressed { false };
        //! Written by the audio thread: where the pad stands, for the crosshair.
        std::atomic<float> shownX { 0.0f };
        std::atomic<float> shownY { 0.0f };
        std::atomic<bool> moving { false };
        //! The audio thread's own: where the pad stands, and where it is heading once let go.
        double x { 0.0 };
        double y { 0.0 };
        double restX { 0.0 };
        double restY { 0.0 };
        bool wasPressed { false };
        //! Where the spring back started, and how many grid steps into it the pad is. Nothing while
        //! it is not springing back.
        double returnFromX { 0.0 };
        double returnFromY { 0.0 };
        std::optional<uint64_t> returnStep;
        float returnTime { 0.0f };
        std::array<AxisSlot, AxisCount> axes;
    };

    bool updateOwnParameter(const std::string & key, float value);
    float ownParameterValue(const std::string & key) const;
    void addPadParameters(size_t pad);
    void describePadParameters(size_t pad);

    std::string m_name;
    std::array<Pad, PadCount> m_pads;
    //! Where the grid started: the first frame evaluated since the last reset.
    std::optional<uint64_t> m_gridOrigin;
};

} // namespace noteahead

#endif // XY_PAD_DEVICE_HPP
