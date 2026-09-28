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

#include "reference_presets.hpp"

#include "../../common/constants.hpp"
#include "reference_environments.hpp"

namespace noteahead {

const EffectPresetList & ReferencePresets::presets()
{
    // Built from the environment table rather than written out again here: a preset is the
    // environment's number and the controls wide open, so a system added to the table cannot be
    // missing from the list that selects it.
    static const EffectPresetList presets = [] {
        namespace C = Constants::NahdXml;
        EffectPresetList result;
        const auto & environments = referenceEnvironments();
        for (size_t i = 0; i < environments.size(); i++) {
            result.push_back(EffectPreset { environments.at(i).name,
                                            { { C::xmlKeyEnvironment().toStdString(), static_cast<float>(i) },
                                              { C::xmlKeyAmount().toStdString(), 1.0f },
                                              { C::xmlKeyRoom().toStdString(), 1.0f },
                                              { C::xmlKeyDynamics().toStdString(), 1.0f } } });
        }
        return result;
    }();
    return presets;
}

} // namespace noteahead
