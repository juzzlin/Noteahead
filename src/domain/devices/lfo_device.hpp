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

#ifndef LFO_DEVICE_HPP
#define LFO_DEVICE_HPP

#include "../dsp/lfo.hpp"
#include "../tracker/parameter_automation.hpp"
#include "../tracker/parameter_modulation.hpp"
#include "device.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

//! Four LFOs, each moving one parameter anywhere in the project: a device's or an effect's, whatever
//! a parameter automation can name.
//!
//! It makes no sound. What it makes is modulation steps, on a fixed grid of the engine's timeline
//! rather than once per block, so that a sweep steps the same at every buffer size and a render
//! matches what was heard. The engine asks for them before anything renders -- see evaluate() --
//! and hands each to its target, which rides it on top of whatever the user or an automation set:
//! see Parameter::setModulatedValue().
//!
//! A device rather than an effect for what only a device has: a track. Notes on it restart the LFOs
//! that are set to, at the grid step they land on, and their velocity can scale the depth.
class LfoDevice : public Device
{
public:
    static constexpr size_t LfoCount { 4 };
    //! Steps per second. A millisecond is finer than a sweep can be heard to step, and is what the
    //! hardware this imitates refreshes its modulation at.
    static constexpr double ControlRateHz { 1000.0 };
    //! Rate at the bottom and the top of the free-running knob, before the divider: the synths' curve,
    //! ParameterMapper::mapLfoFrequency(), which ends a little short of 20 Hz.
    static constexpr double MinRateHz { 0.05 };
    static constexpr double MaxRateHz { 19.05 };
    //! The longest smoothing, at the top of its knob.
    static constexpr double MaxSmoothingSeconds { 0.25 };

    //! One modulation step for the engine to hand to a target.
    struct Step
    {
        size_t lfo {};
        //! Where on the engine's timeline it takes effect.
        uint64_t frame {};
        ParameterAutomation::Target target;
        //! Nothing means take the modulation off: the LFO's target changed or was cleared.
        std::optional<ParameterModulation::Mode> mode;
        double source {};
        double depth {};
    };

    explicit LfoDevice(std::string name);
    ~LfoDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    static std::string typeIdString();

    //! Its notes restart LFOs rather than play pitches
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
    void resetAudio() override;

    //! Every LFO's described controls, grouped by LFO.
    AutomatableParameterList automatableParameters() const override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    //! What LFO @p lfo moves. An empty owner id moves nothing.
    ParameterAutomation::Target target(size_t lfo) const;
    //! Points LFO @p lfo at @p target. The previous target is let go of at the next step.
    void setTarget(size_t lfo, ParameterAutomation::Target target);

    //! Writes LFO @p lfo's control @p key and announces a project edit. For the dialog.
    bool updateLfoParameter(size_t lfo, const QString & key, float value);
    //! What LFO @p lfo's control @p key reads, as a knob shows it, or zero where there is none.
    float lfoParameterValue(size_t lfo, const QString & key) const;

    //! The key LFO @p lfo's control @p key is stored under, "lfo2_rate".
    static std::string lfoKey(size_t lfo, const QString & key);

    //! Advances every LFO over [startFrame, startFrame + frameCount) and appends the steps that
    //! change anything to @p steps. For the engine, on the audio thread, before any device renders.
    //!
    //! The grid runs from the first frame evaluated after a reset, not from each block, so the same
    //! stretch gives the same steps however it is cut into blocks, and a render -- which resets
    //! every device as it starts -- steps the same each time it is run. Notes queued for this device are applied here, at the
    //! step they land on, rather than when the device renders.
    void evaluate(uint64_t startFrame, uint32_t frameCount, double sampleRate, double bpm, std::vector<Step> & steps);

protected:
    void syncParameters() override;

private:
    struct Slot
    {
        Lfo lfo;
        ParameterAutomation::Target target;
        //! A target swapped out since the last step, still carrying this LFO's modulation.
        std::optional<ParameterAutomation::Target> released;
        Lfo::Mode mode { Lfo::Mode::Normal };
        float rate { 0.5f };
        float depth { 0.5f };
        ParameterModulation::Mode modulationMode { ParameterModulation::Mode::Bipolar };
        float phase { 0.0f };
        float smoothing { 0.0f };
        bool retrigger { false };
        float velocitySensitivity { 0.0f };
        //! Of the note that last restarted it, 0..1. Full until a note says otherwise.
        float velocity { 1.0f };
        double smoothed { 0.0 };
        //! What was last handed to the target, to send nothing while nothing changes.
        std::optional<std::pair<double, double>> lastSent;
        std::optional<ParameterModulation::Mode> lastMode;
    };

    void addLfoParameters(size_t lfo);
    void describeLfoParameters(size_t lfo);
    void restart(Slot & slot) const;
    double effectiveDepth(const Slot & slot) const;

    std::string m_name;
    std::array<Slot, LfoCount> m_slots;
    double m_bpm { 120.0 };
    //! Where the grid started: the first frame evaluated since the last reset.
    std::optional<uint64_t> m_gridOrigin;
};

} // namespace noteahead

#endif // LFO_DEVICE_HPP
