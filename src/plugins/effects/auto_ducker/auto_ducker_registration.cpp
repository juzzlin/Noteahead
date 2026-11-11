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

#include "auto_ducker.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect_factory.hpp"

namespace noteahead {

void registerAutoDuckerEffect()
{
    const auto create = [] { return std::make_shared<AutoDucker>(); };
    EffectFactory::registerEffect(AutoDucker::typeIdString(), create);
    // Type strings and legacy names are what projects saved before type ids still carry
    EffectFactory::registerEffect(Constants::RackEffectType::autoDucker().toStdString(), create);
    EffectFactory::registerLegacyEffect("auto_ducker", create);
}

} // namespace noteahead
