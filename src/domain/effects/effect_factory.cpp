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
#include "../dsp/all_pass_filter.hpp"
#include "../dsp/chorus.hpp"
#include "../dsp/clipper.hpp"
#include "../dsp/compressor.hpp"
#include "../dsp/dbtp_meter.hpp"
#include "../dsp/eq_8_band_parametric.hpp"
#include "../dsp/lufs_meter.hpp"
#include "../dsp/reverb.hpp"
#include "../dsp/rta.hpp"
#include "../dsp/saturator.hpp"
#include "auto_panner.hpp"
#include "delay.hpp"
#include "panner.hpp"

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
    registerEffect(AllPassFilter::typeIdString(), []() { return std::make_shared<AllPassFilter>(); });
    registerEffect(AutoPanner::typeIdString(), []() { return std::make_shared<AutoPanner>(); });
    registerEffect(Chorus::typeIdString(), []() { return std::make_shared<Chorus>(); });
    registerEffect(Clipper::typeIdString(), []() { return std::make_shared<Clipper>(); });
    registerEffect(Compressor::typeIdString(), []() { return std::make_shared<Compressor>(); });
    registerEffect(DbTpMeter::typeIdString(), []() { return std::make_shared<DbTpMeter>(); });
    registerEffect(Delay::typeIdString(), []() { return std::make_shared<Delay>(); });
    registerEffect(Eq8BandParametric::typeIdString(), []() { return std::make_shared<Eq8BandParametric>(); });
    registerEffect(LufsMeter::typeIdString(), []() { return std::make_shared<LufsMeter>(); });
    registerEffect(Panner::typeIdString(), []() { return std::make_shared<Panner>(); });
    registerEffect(Rta::typeIdString(), []() { return std::make_shared<Rta>(); });
    registerEffect(Reverb::typeIdString(), []() { return std::make_shared<Reverb>(); });
    registerEffect(Saturator::typeIdString(), []() { return std::make_shared<Saturator>(); });

    // Readable-string aliases so the gallery can create effects by their type() name
    registerEffect(Constants::RackEffectType::autoPanner().toStdString(), []() { return std::make_shared<AutoPanner>(); });
    registerEffect(Constants::RackEffectType::clipper().toStdString(), []() { return std::make_shared<Clipper>(); });
    registerEffect(Constants::RackEffectType::compressor().toStdString(), []() { return std::make_shared<Compressor>(); });
    registerEffect(Constants::RackEffectType::delay().toStdString(), []() { return std::make_shared<Delay>(); });
    registerEffect(Constants::RackEffectType::eq8BandParametric().toStdString(), []() { return std::make_shared<Eq8BandParametric>(); });
    registerEffect(Constants::RackEffectType::panner().toStdString(), []() { return std::make_shared<Panner>(); });
    registerEffect(Constants::RackEffectType::reverb().toStdString(), []() { return std::make_shared<Reverb>(); });
    registerEffect(Constants::RackEffectType::rta().toStdString(), []() { return std::make_shared<Rta>(); });
    registerEffect(Constants::RackEffectType::saturator().toStdString(), []() { return std::make_shared<Saturator>(); });

    // Legacy support
    registerLegacyEffect("auto_panner", []() { return std::make_shared<AutoPanner>(); });
    registerLegacyEffect("chorus", []() { return std::make_shared<Chorus>(); });
    registerLegacyEffect("clipper", []() { return std::make_shared<Clipper>(); });
    registerLegacyEffect("compressor", []() { return std::make_shared<Compressor>(); });
    registerLegacyEffect("delay", []() { return std::make_shared<Delay>(); });
    registerLegacyEffect("eq_8_band_parametric", []() { return std::make_shared<Eq8BandParametric>(); });
    registerLegacyEffect("panner", []() { return std::make_shared<Panner>(); });
    registerLegacyEffect("reverb", []() { return std::make_shared<Reverb>(); });
}

void EffectFactory::clear()
{
    registry().clear();
    legacyRegistry().clear();
}

} // namespace noteahead
