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

#include "../dsp/chorus_effect.hpp"
#include "../dsp/clipper_effect.hpp"
#include "../dsp/compressor_effect.hpp"
#include "../dsp/eq_8_band_parametric_effect.hpp"
#include "../dsp/reverb_effect.hpp"
#include "auto_panner_effect.hpp"
#include "panner_effect.hpp"

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
}

void EffectFactory::registerEffect(const std::string & typeId, Creator creator)
{
    registry()[typeId] = std::move(creator);
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
    registerEffect(AutoPannerEffect::typeIdString(), []() { return std::make_shared<AutoPannerEffect>(); });
    registerEffect(ChorusEffect::typeIdString(), []() { return std::make_shared<ChorusEffect>(); });
    registerEffect(ClipperEffect::typeIdString(), []() { return std::make_shared<ClipperEffect>(); });
    registerEffect(CompressorEffect::typeIdString(), []() { return std::make_shared<CompressorEffect>(); });
    registerEffect(Eq8BandParametricEffect::typeIdString(), []() { return std::make_shared<Eq8BandParametricEffect>(); });
    registerEffect(PannerEffect::typeIdString(), []() { return std::make_shared<PannerEffect>(); });
    registerEffect(ReverbEffect::typeIdString(), []() { return std::make_shared<ReverbEffect>(); });

    // Legacy support
    registerLegacyEffect("auto_panner", []() { return std::make_shared<AutoPannerEffect>(); });
    registerLegacyEffect("chorus", []() { return std::make_shared<ChorusEffect>(); });
    registerLegacyEffect("clipper", []() { return std::make_shared<ClipperEffect>(); });
    registerLegacyEffect("compressor", []() { return std::make_shared<CompressorEffect>(); });
    registerLegacyEffect("eq_8_band_parametric", []() { return std::make_shared<Eq8BandParametricEffect>(); });
    registerLegacyEffect("panner", []() { return std::make_shared<PannerEffect>(); });
    registerLegacyEffect("reverb", []() { return std::make_shared<ReverbEffect>(); });
}

void EffectFactory::clear()
{
    registry().clear();
    legacyRegistry().clear();
}

} // namespace noteahead
