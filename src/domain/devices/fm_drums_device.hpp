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

#ifndef FM_DRUMS_DEVICE_HPP
#define FM_DRUMS_DEVICE_HPP

#include "../dsp/drum/fm_drum_engine.hpp"
#include "drum_pad_device.hpp"
#include "fm_drums_presets.hpp"

#include <string>

namespace noteahead {

//! A drum machine with the same four-operator FM voice on all twelve pads.
//!
//! Unlike Drum Synth V2, whose pads each play an engine built for one drum, every pad here is an
//! FmDrumEngine and differs only in its settings: the kit a new device starts with is a set of
//! settings, and any pad can be made into any drum. The pads, their notes and everything after the
//! engine are DrumPadDevice's, and so the same as V2's, so a beat moves between the two unchanged.
class FmDrumsDevice : public DrumPadDevice
{
public:
    // Ranges of the controls, what a knob's travel maps onto. The dialog reads the same ones.
    static constexpr double MinFrequency { 20.0 };
    static constexpr double MaxFrequency { 5000.0 };
    static constexpr double MinRatio { 0.25 };
    static constexpr double MaxRatio { 16.0 };
    static constexpr double MaxPitchSemitones { 48.0 };
    static constexpr double MinPitchDecaySeconds { 0.001 };
    static constexpr double MaxPitchDecaySeconds { 1.0 };
    static constexpr double MaxAttackSeconds { 1.0 };
    static constexpr double MinDecaySeconds { 0.005 };
    static constexpr double MaxDecaySeconds { 8.0 };

    explicit FmDrumsDevice(std::string name);
    virtual ~FmDrumsDevice() override = default;

    std::string typeName() const override;
    std::string typeId() const override;

    static std::string typeIdString();

    std::unique_ptr<DrumPadDevice> newInstance(std::string name) const override;

    //! Puts the whole device back to its defaults and voices every pad from factory kit @p index.
    void loadKit(size_t index);
    //! Gives @p pad the sound of FmDrumsPresets::pads() entry @p index: its voice and filters, and
    //! not its level, pan, envelope or effects.
    void loadPadPreset(int pad, size_t index);

protected:
    void addEngineParameters(int index, const std::string & prefix) override;
    void syncEngineParameters(int index, const std::string & prefix) override;
    std::pair<float, float> defaultFilterCutoffs(int index) const override;

private:
    //! Units and labels for every continuous control of one pad, so that automation offers them.
    void describeEngineParameters(const std::string & prefix);

    FmDrumEngine & engine(int index);
    //! Writes @p voicing into the engine controls of pad @p index without syncing.
    void applyVoicing(int index, const FmDrumsPadVoicing & voicing);
};

} // namespace noteahead

#endif // FM_DRUMS_DEVICE_HPP
