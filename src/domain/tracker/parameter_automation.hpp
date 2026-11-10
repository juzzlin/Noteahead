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

#ifndef PARAMETER_AUTOMATION_HPP
#define PARAMETER_AUTOMATION_HPP

#include "automation.hpp"
#include "automation_curve.hpp"
#include "interpolator.hpp"
#include "modulation.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace noteahead {

class Parameter;
class ProjectReader;
class ProjectWriter;

//! An automation that drives one of the application's own parameters directly, rather than through
//! a MIDI controller: it names the parameter, so it needs no controller number, and it is not held
//! to the 128 steps of a MIDI data byte.
//!
//! Values are knob positions, 0 to 1, which is how the parameter itself is saved: a project that
//! stores both the same way keeps them in step if a knob's curve is ever changed. Anything showing
//! one to a person converts it through the parameter's ParameterDisplay, so it reads in hertz or
//! milliseconds there. Interpolating positions rather than units is also what makes a sweep move
//! the way the knob does -- evenly in octaves on a cutoff, not crowded into its top.
class ParameterAutomation : public Automation
{
public:
    //! What the automation drives: a parameter of an effect, or of a device.
    struct Target
    {
        enum class Owner
        {
            Effect,
            Device,
            //! A MIDI controller of an instrument rather than a parameter: what a modulator moves
            //! on external gear, or through an internal device's own CC handling. Only modulators
            //! aim at one; a parameter automation never does.
            MidiCc
        };

        //! Where a MIDI CC target sends: the channel, 0 to 15, and the controller.
        struct MidiCcAddress
        {
            uint8_t channel {};
            uint8_t controller {};
        };

        Owner owner = Owner::Effect;
        //! The owner's id: Effect::id() for an effect, Device::uuid() for a device, the port's name
        //! for a MIDI CC.
        std::string ownerId;
        //! The parameter's key within its owner. For a MIDI CC, its channel and controller as
        //! midiCcKey() writes them.
        std::string parameter;
        //! The values the bottom and the top of a modulator's swing send, for a MIDI CC. Min above
        //! Max turns it around.
        uint8_t ccMin { 0 };
        uint8_t ccMax { 127 };

        bool operator==(const Target & other) const = default;

        //! Whether @p other is the same parameter or controller, whatever range it is moved over.
        bool isSameDestination(const Target & other) const;
        //! This target with @p previous's CC range when both are MIDI CCs, so that picking another
        //! controller keeps the range set for the last.
        Target withRangeOf(const Target & previous) const;

        //! "effect", "device" or "midiCc", as the project file and the UI both name an owner.
        //! Anything else reads as an effect, which is what an automation that does not say is
        //! aimed at.
        static QString ownerToString(Owner owner);
        static Owner ownerFromString(const QString & owner);

        //! The parameter key of a MIDI CC target: "2:74" for controller 74 on channel 2.
        static std::string midiCcKey(uint8_t channel, uint8_t controller);
        //! The channel and the controller of a MIDI CC target, or nothing for any other target, or
        //! one whose key does not parse.
        std::optional<MidiCcAddress> midiCcAddress() const;
        //! Where a MIDI CC target's controller goes for a modulator at @p position, 0 to 1.
        uint8_t midiCcValue(double position) const;
    };

    enum class Mode
    {
        //! The values are where the knob goes.
        Absolute,
        //! The values are how far towards the knob's own setting the control is let go, from its
        //! rest point at 0 to exactly the setting at 1. Works the way Expression does: the
        //! automation can be written without knowing what the knob is set to.
        Relative
    };

    struct InterpolationParameters
    {
        size_t line0 = 0;
        size_t line1 = 0;
        double value0 = 0.0;
        double value1 = 0.0;
        Interpolator::CurveType curve = Interpolator::CurveType::Linear;
        //! The nodes, when the curve is Drawn: value0 and value1 are then its ends.
        AutomationCurve drawnCurve {};

        bool operator==(const InterpolationParameters & other) const = default;
    };

    ParameterAutomation();
    //! Modulation amplitude and offset are in percent of the knob's travel, since a fixed number of
    //! hertz means something different at every point of a log scale.
    ParameterAutomation(size_t id, AutomationLocation location, Target target, Mode mode, InterpolationParameters interpolation, ModulationParameters modulation, QString comment, bool enabled);

    bool operator==(const ParameterAutomation & other) const;
    bool operator!=(const ParameterAutomation & other) const;
    bool operator<(const ParameterAutomation & other) const;

    const Target & target() const;
    void setTarget(const Target & target);

    Mode mode() const;
    void setMode(Mode mode);

    const InterpolationParameters & interpolation() const;
    void setInterpolation(const InterpolationParameters & interpolation);

    const ModulationParameters & modulation() const;
    void setModulation(const ModulationParameters & modulation);

    uint8_t eventsPerBeat() const;
    void setEventsPerBeat(uint8_t eventsPerBeat);

    uint8_t lineOffset() const;
    void setLineOffset(uint8_t lineOffset);

    //! The value @p parameter is to run at when an automation in @p mode reaches @p value.
    //!
    //! Relative scales the knob's own setting towards the control's rest point, so 1 is exactly the
    //! knob and 0 is the rest: the bottom of the travel for most controls, the centre for one that
    //! runs either way about off. Read from the authored value rather than the live one, so moving
    //! the knob while the song plays moves the top of the automation with it.
    static float effectiveValue(Mode mode, double value, const Parameter & parameter);

    void serializeToXml(ProjectWriter & writer) const;
    using ParameterAutomationU = std::unique_ptr<ParameterAutomation>;
    static ParameterAutomationU deserializeFromXml(ProjectReader & reader);

    QString toString() const;

private:
    Target m_target;
    Mode m_mode = Mode::Absolute;
    uint8_t m_eventsPerBeat = 0;
    uint8_t m_lineOffset = 0;
    InterpolationParameters m_interpolation;
    ModulationParameters m_modulation;
};

} // namespace noteahead

#endif // PARAMETER_AUTOMATION_HPP
