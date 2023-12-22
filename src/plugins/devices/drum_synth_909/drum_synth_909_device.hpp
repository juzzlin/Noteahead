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

#ifndef DRUM_SYNTH_909_DEVICE_HPP
#define DRUM_SYNTH_909_DEVICE_HPP

#include "../../../domain/devices/drum_pad_device.hpp"

#include <string>
#include <vector>

namespace noteahead {

//! A TR-909, each drum modelled on recordings of an RD-9 with its knobs swept.
//!
//! The pads, their notes and everything after the drums are DrumPadDevice's, as Drum Synth V2 and
//! FM Drums have them, so a beat moves between the three unchanged; the pad V2 plays a reversed
//! crash on plays the 909's crash reversed. Each drum has the knobs an RD-9 has for it and no more.
class DrumSynth909Device : public DrumPadDevice
{
public:
    explicit DrumSynth909Device(std::string name);
    virtual ~DrumSynth909Device() override = default;

    std::string typeName() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;

    static std::string typeIdString();

    std::unique_ptr<DrumPadDevice> newInstance(std::string name) const override;

    //! The factory kits, which differ only in the mix: Init as the device starts, Mono with every
    //! pad in the centre at the same levels, Equal with every pad peaking where the kick does, and
    //! Mono Equal with both
    enum class Kit
    {
        Init,
        Mono,
        Equal,
        MonoEqual
    };
    static std::vector<std::string> kitNames();
    //! Puts the whole device back to its defaults and mixes it as @p kit does
    void loadKit(Kit kit);

    //! One knob of a drum's panel
    struct Knob
    {
        //! Its key, without the pad's prefix
        std::string key;
        //! What the panel calls it, untranslated: translated where it is shown, in the
        //! "ParameterDisplay" context
        const char * label;
    };

    //! The knobs of the drum on pad @p index, as the RD-9's panel has them, in its order
    static std::vector<Knob> voiceKnobs(int index);

protected:
    void addEngineParameters(int index, const std::string & prefix) override;
    void syncEngineParameters(int index, const std::string & prefix) override;
    std::pair<float, float> defaultVoiceMix(int index) const override;
};

} // namespace noteahead

#endif // DRUM_SYNTH_909_DEVICE_HPP
