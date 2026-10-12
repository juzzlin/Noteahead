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

#ifndef DRUM_SYNTH_V2_DEVICE_HPP
#define DRUM_SYNTH_V2_DEVICE_HPP

#include "../dsp/drum/crash_engine.hpp"
#include "../dsp/drum/hihat_engine.hpp"
#include "../dsp/drum/kick_engine.hpp"
#include "../dsp/drum/ride_engine.hpp"
#include "../dsp/drum/rim_engine.hpp"
#include "../dsp/drum/snare_engine.hpp"
#include "../dsp/drum/tom_engine.hpp"
#include "drum_pad_device.hpp"

#include <string>

namespace noteahead {

//! The analogue-modelled kit: a bespoke engine on every pad, fitted drum by drum.
class DrumSynthV2Device : public DrumPadDevice
{
public:
    explicit DrumSynthV2Device(std::string name);
    virtual ~DrumSynthV2Device() override = default;

    std::string typeName() const override;
    std::string typeId() const override;

    static std::string typeIdString();

protected:
    void addEngineParameters(int index, const std::string & prefix) override;
    void syncEngineParameters(int index, const std::string & prefix) override;
    //! See the definition: what a kit saved before the sustain stage was voiced against.
    void restoreLegacyParameters() override;

private:
    void initializeEngines();
    void addKickParameters(const std::string & prefix);
    void addRimParameters(const std::string & prefix);
    void addSnareParameters(const std::string & prefix);
    void addTomParameters(const std::string & prefix);
    void addHiHatParameters(const std::string & prefix);
    void addCymbalParameters(const std::string & prefix);

    void syncCommonEngineParameters(int index, const std::string & prefix);
    void syncKickParameters(const std::string & prefix);
    void syncSnareParameters(const std::string & prefix);
    void syncClapParameters(const std::string & prefix);
    void syncTomParameters(int index, const std::string & prefix);
    void syncHiHatParameters(int index, const std::string & prefix);
    void syncCymbalParameters(int index, const std::string & prefix);
    void syncRimParameters(int index, const std::string & prefix);
};

} // namespace noteahead

#endif // DRUM_SYNTH_V2_DEVICE_HPP
