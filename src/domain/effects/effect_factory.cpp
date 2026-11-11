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

#include "effect_factory.hpp"

#include "../../common/constants.hpp"
#include "../utility/dbtp_meter.hpp"
#include "../utility/lufs_meter.hpp"
#include "../utility/rta.hpp"
#include "../utility/stereo_field_meter.hpp"

#include <map>

namespace noteahead {

namespace {
std::map<std::string, EffectFactory::Creator> & registry()
{
    static std::map<std::string, EffectFactory::Creator> instance;
    return instance;
}

std::map<std::string, EffectFactory::Creator> & legacyRegistry()
{
    static std::map<std::string, EffectFactory::Creator> instance;
    return instance;
}
} // namespace

void EffectFactory::registerEffect(const std::string & typeId, Creator creator)
{
    registry()[typeId] = std::move(creator);
}

std::vector<std::string> EffectFactory::typeIds()
{
    std::vector<std::string> ids;
    ids.reserve(registry().size());
    for (auto && [typeId, creator] : registry()) {
        ids.push_back(typeId);
    }
    return ids;
}

void EffectFactory::registerLegacyEffect(const std::string & legacyType, Creator creator)
{
    legacyRegistry()[legacyType] = std::move(creator);
}

std::shared_ptr<Effect> EffectFactory::createEffect(const std::string & typeId)
{
    return createEffect(typeId, "");
}

std::shared_ptr<Effect> EffectFactory::createEffect(const std::string & typeId, const std::string & legacyType)
{
    if (const auto it = registry().find(typeId); it != registry().end()) {
        return it->second();
    }

    if (!legacyType.empty()) {
        if (const auto itLegacy = legacyRegistry().find(legacyType); itLegacy != legacyRegistry().end()) {
            return itLegacy->second();
        }
    }

    return {};
}

void EffectFactory::init()
{
    registerPlugins();
    registerEffect(DbTpMeter::typeIdString(), []() { return std::make_shared<DbTpMeter>(); });
    registerEffect(LufsMeter::typeIdString(), []() { return std::make_shared<LufsMeter>(); });
    registerEffect(Rta::typeIdString(), []() { return std::make_shared<Rta>(); });
    registerEffect(StereoFieldMeter::typeIdString(), []() { return std::make_shared<StereoFieldMeter>(); });

    // Readable-string aliases so the gallery can create effects by their type() name
    registerEffect(Constants::RackEffectType::rta().toStdString(), []() { return std::make_shared<Rta>(); });
    registerEffect(Constants::RackEffectType::stereoFieldMeter().toStdString(), []() { return std::make_shared<StereoFieldMeter>(); });

    // Legacy support
    registerLegacyEffect("stereo_field_meter", []() { return std::make_shared<StereoFieldMeter>(); });
}

void EffectFactory::clear()
{
    registry().clear();
    legacyRegistry().clear();
}

} // namespace noteahead
