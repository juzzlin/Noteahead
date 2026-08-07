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
#include "air_band_eq.hpp"
#include "all_pass_filter.hpp"
#include "auto_ducker.hpp"
#include "auto_panner.hpp"
#include "chorus.hpp"
#include "clipper.hpp"
#include "compressor.hpp"
#include "delay.hpp"
#include "drive.hpp"
#include "endless_reverb.hpp"
#include "eq_8_band_parametric.hpp"
#include "limiter.hpp"
#include "panner.hpp"
#include "reverb.hpp"
#include "saturator.hpp"
#include "simple_eq.hpp"
#include "stereo_enhancer.hpp"
#include "tube_stage.hpp"
#include "vintage_passive_eq.hpp"
#include "wave_designer.hpp"

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
    registerEffect(AirBandEq::typeIdString(), []() { return std::make_shared<AirBandEq>(); });
    registerEffect(AllPassFilter::typeIdString(), []() { return std::make_shared<AllPassFilter>(); });
    registerEffect(AutoDucker::typeIdString(), []() { return std::make_shared<AutoDucker>(); });
    registerEffect(AutoPanner::typeIdString(), []() { return std::make_shared<AutoPanner>(); });
    registerEffect(EndlessReverb::typeIdString(), []() { return std::make_shared<EndlessReverb>(); });
    registerEffect(Chorus::typeIdString(), []() { return std::make_shared<Chorus>(); });
    registerEffect(Clipper::typeIdString(), []() { return std::make_shared<Clipper>(); });
    registerEffect(Compressor::typeIdString(), []() { return std::make_shared<Compressor>(); });
    registerEffect(DbTpMeter::typeIdString(), []() { return std::make_shared<DbTpMeter>(); });
    registerEffect(Delay::typeIdString(), []() { return std::make_shared<Delay>(); });
    registerEffect(Drive::typeIdString(), []() { return std::make_shared<Drive>(); });
    registerEffect(Eq8BandParametric::typeIdString(), []() { return std::make_shared<Eq8BandParametric>(); });
    registerEffect(Limiter::typeIdString(), []() { return std::make_shared<Limiter>(); });
    registerEffect(LufsMeter::typeIdString(), []() { return std::make_shared<LufsMeter>(); });
    registerEffect(Panner::typeIdString(), []() { return std::make_shared<Panner>(); });
    registerEffect(Rta::typeIdString(), []() { return std::make_shared<Rta>(); });
    registerEffect(Reverb::typeIdString(), []() { return std::make_shared<Reverb>(); });
    registerEffect(Saturator::typeIdString(), []() { return std::make_shared<Saturator>(); });
    registerEffect(TubeStage::typeIdString(), []() { return std::make_shared<TubeStage>(); });
    registerEffect(VintagePassiveEq::typeIdString(), []() { return std::make_shared<VintagePassiveEq>(); });
    registerEffect(SimpleEq::typeIdString(), []() { return std::make_shared<SimpleEq>(); });

    // Readable-string aliases so the gallery can create effects by their type() name
    registerEffect(Constants::RackEffectType::autoDucker().toStdString(), []() { return std::make_shared<AutoDucker>(); });
    registerEffect(Constants::RackEffectType::autoPanner().toStdString(), []() { return std::make_shared<AutoPanner>(); });
    registerEffect(Constants::RackEffectType::endless().toStdString(), []() { return std::make_shared<EndlessReverb>(); });
    registerEffect(Constants::RackEffectType::clipper().toStdString(), []() { return std::make_shared<Clipper>(); });
    registerEffect(Constants::RackEffectType::compressor().toStdString(), []() { return std::make_shared<Compressor>(); });
    registerEffect(Constants::RackEffectType::delay().toStdString(), []() { return std::make_shared<Delay>(); });
    registerEffect(Constants::RackEffectType::drive().toStdString(), []() { return std::make_shared<Drive>(); });
    registerEffect(Constants::RackEffectType::eq8BandParametric().toStdString(), []() { return std::make_shared<Eq8BandParametric>(); });
    registerEffect(Constants::RackEffectType::limiter().toStdString(), []() { return std::make_shared<Limiter>(); });
    registerEffect(Constants::RackEffectType::panner().toStdString(), []() { return std::make_shared<Panner>(); });
    registerEffect(Constants::RackEffectType::reverb().toStdString(), []() { return std::make_shared<Reverb>(); });
    registerEffect(Constants::RackEffectType::rta().toStdString(), []() { return std::make_shared<Rta>(); });
    registerEffect(Constants::RackEffectType::saturator().toStdString(), []() { return std::make_shared<Saturator>(); });
    registerEffect(Constants::RackEffectType::vintagePassiveEq().toStdString(), []() { return std::make_shared<VintagePassiveEq>(); });
    registerEffect(Constants::RackEffectType::airBandEq().toStdString(), []() { return std::make_shared<AirBandEq>(); });
    registerEffect(Constants::RackEffectType::simpleEq().toStdString(), []() { return std::make_shared<SimpleEq>(); });
    registerEffect(Constants::RackEffectType::tubeStage().toStdString(), []() { return std::make_shared<TubeStage>(); });
    registerEffect(StereoEnhancer::typeIdString(), []() { return std::make_shared<StereoEnhancer>(); });
    registerEffect(Constants::RackEffectType::stereoEnhancer().toStdString(), []() { return std::make_shared<StereoEnhancer>(); });
    registerEffect(WaveDesigner::typeIdString(), []() { return std::make_shared<WaveDesigner>(); });
    registerEffect(Constants::RackEffectType::waveDesigner().toStdString(), []() { return std::make_shared<WaveDesigner>(); });

    // Legacy support
    registerLegacyEffect("auto_ducker", []() { return std::make_shared<AutoDucker>(); });
    registerLegacyEffect("tube_stage", []() { return std::make_shared<TubeStage>(); });
    registerLegacyEffect("wave_designer", []() { return std::make_shared<WaveDesigner>(); });
    registerLegacyEffect("stereo_enhancer", []() { return std::make_shared<StereoEnhancer>(); });
    registerLegacyEffect("auto_panner", []() { return std::make_shared<AutoPanner>(); });
    registerLegacyEffect("endless", []() { return std::make_shared<EndlessReverb>(); });
    registerLegacyEffect("chorus", []() { return std::make_shared<Chorus>(); });
    registerLegacyEffect("clipper", []() { return std::make_shared<Clipper>(); });
    registerLegacyEffect("compressor", []() { return std::make_shared<Compressor>(); });
    registerLegacyEffect("delay", []() { return std::make_shared<Delay>(); });
    registerLegacyEffect("eq_8_band_parametric", []() { return std::make_shared<Eq8BandParametric>(); });
    registerLegacyEffect("limiter", []() { return std::make_shared<Limiter>(); });
    registerLegacyEffect("panner", []() { return std::make_shared<Panner>(); });
    registerLegacyEffect("reverb", []() { return std::make_shared<Reverb>(); });
    registerLegacyEffect("vintage_passive_eq", []() { return std::make_shared<VintagePassiveEq>(); });
    registerLegacyEffect("air_band_eq", []() { return std::make_shared<AirBandEq>(); });
    registerLegacyEffect("simple_eq", []() { return std::make_shared<SimpleEq>(); });
}

void EffectFactory::clear()
{
    registry().clear();
    legacyRegistry().clear();
}

} // namespace noteahead
