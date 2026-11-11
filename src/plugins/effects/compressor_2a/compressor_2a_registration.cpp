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

#include "compressor_2a.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect_factory.hpp"

namespace noteahead {

void registerCompressor2aEffect()
{
    const auto create = [] { return std::make_shared<Compressor2A>(); };
    EffectFactory::registerEffect(Compressor2A::typeIdString(), create);
    // Type strings and legacy names are what projects saved before type ids still carry
    EffectFactory::registerEffect(Constants::RackEffectType::compressor2A().toStdString(), create);
}

} // namespace noteahead
