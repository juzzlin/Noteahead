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

#include "compressor_76_presets.hpp"

#include "../../../common/constants.hpp"
#include "compressor_76.hpp"

#include <map>
#include <utility>

namespace noteahead {

namespace {

struct PresetValues
{
    double inputDb;
    double outputDb;
    double attackMs;
    double releaseMs;
    Compressor76::Ratio ratio;
    Compressor76::Character character;
    double lowCutHz;
    float mix;
};

EffectPreset preset(std::string name, const PresetValues & values)
{
    namespace C = Constants::NahdXml;
    return EffectPreset {
        std::move(name),
        {
          { C::xmlKeyInputGain().toStdString(), Compressor76::inputGainPosition(values.inputDb) },
          { C::xmlKeyGain().toStdString(), Compressor76::outputGainPosition(values.outputDb) },
          { C::xmlKeyAttack().toStdString(), Compressor76::attackPosition(values.attackMs) },
          { C::xmlKeyRelease().toStdString(), Compressor76::releasePosition(values.releaseMs) },
          { C::xmlKeyRatio().toStdString(), static_cast<float>(values.ratio) },
          { C::xmlKeyMode().toStdString(), static_cast<float>(values.character) },
          { C::xmlKeyHpfCutoff().toStdString(), Compressor76::lowCutPosition(values.lowCutHz) },
          { C::xmlKeyMix().toStdString(), values.mix },
        }
    };
}

} // namespace

const EffectPresetList & Compressor76Presets::presets()
{
    using Ratio = Compressor76::Ratio;
    using Character = Compressor76::Character;

    // Init first, as the way back; the rest alphabetical
    static const EffectPresetList presets {
        { "Init", {} },
        preset("All Buttons Crush", { 10.0, -4.0, 0.05, 60.0, Ratio::All, Character::BlueStripe, 80.0, 0.4f }),
        preset("Bass Lock", { 6.0, -2.0, 0.25, 120.0, Ratio::EightToOne, Character::Blackface, 0.0, 1.0f }),
        preset("Drum Punch", { 6.0, -2.0, 0.70, 80.0, Ratio::FourToOne, Character::BlueStripe, 100.0, 1.0f }),
        preset("Mix Bus Glue", { -4.0, 2.0, 0.60, 200.0, Ratio::FourToOne, Character::Blackface, 90.0, 1.0f }),
        preset("Peak Limiter", { 4.0, -1.0, 0.02, 100.0, Ratio::TwentyToOne, Character::Blackface, 0.0, 1.0f }),
        preset("Snare Snap", { 8.0, -3.0, 0.50, 70.0, Ratio::EightToOne, Character::BlueStripe, 120.0, 1.0f }),
        preset("Vocal Presence", { 6.0, -2.0, 0.15, 100.0, Ratio::FourToOne, Character::BlueStripe, 80.0, 1.0f }),
    };
    return presets;
}

} // namespace noteahead
