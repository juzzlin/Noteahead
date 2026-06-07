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

#ifndef EFFECT_FACTORY_HPP
#define EFFECT_FACTORY_HPP

#include <functional>
#include <memory>
#include <string>

namespace noteahead {

class Effect;

class EffectFactory
{
public:
    using Creator = std::function<std::shared_ptr<Effect>()>;

    static void registerEffect(const std::string & typeId, Creator creator);
    static void registerLegacyEffect(const std::string & legacyType, Creator creator);
    static std::shared_ptr<Effect> createEffect(const std::string & typeId);
    static std::shared_ptr<Effect> createEffect(const std::string & typeId, const std::string & legacyType);
    static void init();
    static void clear();
};

} // namespace noteahead

#endif // EFFECT_FACTORY_HPP
