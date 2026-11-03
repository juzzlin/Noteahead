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

#ifndef MODULATION_SOURCE_HPP
#define MODULATION_SOURCE_HPP

#include "../tracker/parameter_automation.hpp"
#include "../tracker/parameter_modulation.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace noteahead {

//! A device that moves other devices' and effects' parameters: the LFO device, the XY pad.
//!
//! The engine asks every source for its steps before anything renders -- see evaluate() -- and hands
//! each to its target, which rides it on top of whatever the user or an automation set: see
//! Parameter::setModulatedValue(). A source has numbered slots, one per thing it can aim at a
//! target, and each slot's offsets add up with any other source's on the same parameter.
class ModulationSource
{
public:
    //! One modulation step for the engine to hand to a target.
    struct Step
    {
        //! Which of the source's slots it comes from: an LFO, an axis of a pad.
        size_t slot {};
        //! Where on the engine's timeline it takes effect.
        uint64_t frame {};
        ParameterAutomation::Target target;
        //! Nothing means take the modulation off: the slot's target changed or was cleared.
        std::optional<ParameterModulation::Mode> mode;
        double source {};
        double depth {};
    };

    virtual ~ModulationSource();

    //! Appends the steps that change anything over [startFrame, startFrame + frameCount) to
    //! @p steps. For the engine, on the audio thread, before any device renders.
    virtual void evaluate(uint64_t startFrame, uint32_t frameCount, double sampleRate, double bpm, std::vector<Step> & steps) = 0;

    //! Every target a slot is aimed at, one entry per slot that has one. For the UI thread, to
    //! tell the user where two sources meet on the same parameter.
    virtual std::vector<ParameterAutomation::Target> modulationTargets() const = 0;
};

} // namespace noteahead

#endif // MODULATION_SOURCE_HPP
