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
#include "../../plugins/effects/air_band_eq/air_band_eq.hpp"
#include "../../plugins/effects/all_pass_filter/all_pass_filter.hpp"
#include "../../plugins/effects/ambience/ambience.hpp"
#include "../../plugins/effects/analog_fuzz/analog_fuzz.hpp"
#include "../../plugins/effects/auto_ducker/auto_ducker.hpp"
#include "../../plugins/effects/auto_filter/auto_filter.hpp"
#include "../../plugins/effects/auto_panner/auto_panner.hpp"
#include "../../plugins/effects/bass_grinder/bass_grinder.hpp"
#include "../../plugins/effects/bit_crusher/bit_crusher.hpp"
#include "../../plugins/effects/chorus/chorus.hpp"
#include "../../plugins/effects/clipper/clipper.hpp"
#include "../../plugins/effects/compressor/compressor.hpp"
#include "../../plugins/effects/compressor_2a/compressor_2a.hpp"
#include "../../plugins/effects/crossfeed/crossfeed.hpp"
#include "../../plugins/effects/delay/delay.hpp"
#include "../../plugins/effects/dimension/dimension.hpp"
#include "../../plugins/effects/doubler/doubler.hpp"
#include "../../plugins/effects/drive/drive.hpp"
#include "../../plugins/effects/dynamic_eq/dynamic_eq.hpp"
#include "../../plugins/effects/early_reflections/early_reflections.hpp"
#include "../../plugins/effects/endless_reverb/endless_reverb.hpp"
#include "../../plugins/effects/eq_32_band_graphical/eq_32_band_graphical.hpp"
#include "../../plugins/effects/eq_8_band_parametric/eq_8_band_parametric.hpp"
#include "../../plugins/effects/flanger/flanger.hpp"
#include "../../plugins/effects/gain/gain.hpp"
#include "../../plugins/effects/gate/gate.hpp"
#include "../../plugins/effects/limiter/limiter.hpp"
#include "../../plugins/effects/monitor/monitor.hpp"
#include "../../plugins/effects/multiband_compressor/multiband_compressor.hpp"
#include "../../plugins/effects/panner/panner.hpp"
#include "../../plugins/effects/phase_scope/phase_scope.hpp"
#include "../../plugins/effects/phaser/phaser.hpp"
#include "../../plugins/effects/reference/reference.hpp"
#include "../../plugins/effects/reverb/reverb.hpp"
#include "../../plugins/effects/reverb_224/reverb_224.hpp"
#include "../../plugins/effects/saturator/saturator.hpp"
#include "../../plugins/effects/simple_eq/simple_eq.hpp"
#include "../../plugins/effects/stereo_enhancer/stereo_enhancer.hpp"
#include "../../plugins/effects/stereo_exciter/stereo_exciter.hpp"
#include "../../plugins/effects/stereo_widener/stereo_widener.hpp"
#include "../../plugins/effects/tremolo/tremolo.hpp"
#include "../../plugins/effects/tube_stage/tube_stage.hpp"
#include "../../plugins/effects/vintage_passive_eq/vintage_passive_eq.hpp"
#include "../../plugins/effects/wave_designer/wave_designer.hpp"
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
    registerEffect(AirBandEq::typeIdString(), []() { return std::make_shared<AirBandEq>(); });
    registerEffect(AllPassFilter::typeIdString(), []() { return std::make_shared<AllPassFilter>(); });
    registerEffect(AutoDucker::typeIdString(), []() { return std::make_shared<AutoDucker>(); });
    registerEffect(AutoFilter::typeIdString(), []() { return std::make_shared<AutoFilter>(); });
    registerEffect(AutoPanner::typeIdString(), []() { return std::make_shared<AutoPanner>(); });
    registerEffect(EndlessReverb::typeIdString(), []() { return std::make_shared<EndlessReverb>(); });
    registerEffect(Chorus::typeIdString(), []() { return std::make_shared<Chorus>(); });
    registerEffect(Clipper::typeIdString(), []() { return std::make_shared<Clipper>(); });
    registerEffect(Compressor::typeIdString(), []() { return std::make_shared<Compressor>(); });
    registerEffect(DbTpMeter::typeIdString(), []() { return std::make_shared<DbTpMeter>(); });
    registerEffect(Delay::typeIdString(), []() { return std::make_shared<Delay>(); });
    registerEffect(Dimension::typeIdString(), []() { return std::make_shared<Dimension>(); });
    registerEffect(Compressor2A::typeIdString(), []() { return std::make_shared<Compressor2A>(); });
    registerEffect(Doubler::typeIdString(), []() { return std::make_shared<Doubler>(); });
    registerEffect(AnalogFuzz::typeIdString(), []() { return std::make_shared<AnalogFuzz>(); });
    registerEffect(BassGrinder::typeIdString(), []() { return std::make_shared<BassGrinder>(); });
    registerEffect(Drive::typeIdString(), []() { return std::make_shared<Drive>(); });
    registerEffect(EarlyReflections::typeIdString(), []() { return std::make_shared<EarlyReflections>(); });
    registerEffect(Eq32BandGraphical::typeIdString(), []() { return std::make_shared<Eq32BandGraphical>(); });
    registerEffect(Eq8BandParametric::typeIdString(), []() { return std::make_shared<Eq8BandParametric>(); });
    registerEffect(Limiter::typeIdString(), []() { return std::make_shared<Limiter>(); });
    registerEffect(LufsMeter::typeIdString(), []() { return std::make_shared<LufsMeter>(); });
    registerEffect(Crossfeed::typeIdString(), []() { return std::make_shared<Crossfeed>(); });
    registerEffect(BitCrusher::typeIdString(), []() { return std::make_shared<BitCrusher>(); });
    registerEffect(Flanger::typeIdString(), []() { return std::make_shared<Flanger>(); });
    registerEffect(Gate::typeIdString(), []() { return std::make_shared<Gate>(); });
    registerEffect(Tremolo::typeIdString(), []() { return std::make_shared<Tremolo>(); });
    registerEffect(Reference::typeIdString(), []() { return std::make_shared<Reference>(); });
    registerEffect(Monitor::typeIdString(), []() { return std::make_shared<Monitor>(); });
    // Its type id and its type are the same string, as the All-Pass Filter's are
    registerEffect(DynamicEq::typeIdString(), []() { return std::make_shared<DynamicEq>(); });
    registerEffect(Ambience::typeIdString(), []() { return std::make_shared<Ambience>(); });
    registerEffect(Reverb224::typeIdString(), []() { return std::make_shared<Reverb224>(); });
    registerEffect(PhaseScope::typeIdString(), []() { return std::make_shared<PhaseScope>(); });
    // The type id it was saved under for the first hour it existed, as Utility
    registerEffect("utility", []() { return std::make_shared<PhaseScope>(); });
    registerEffect(Gain::typeIdString(), []() { return std::make_shared<Gain>(); });
    registerEffect(MultibandCompressor::typeIdString(), []() { return std::make_shared<MultibandCompressor>(); });
    registerEffect(Panner::typeIdString(), []() { return std::make_shared<Panner>(); });
    registerEffect(Phaser::typeIdString(), []() { return std::make_shared<Phaser>(); });
    registerEffect(Rta::typeIdString(), []() { return std::make_shared<Rta>(); });
    registerEffect(StereoFieldMeter::typeIdString(), []() { return std::make_shared<StereoFieldMeter>(); });
    registerEffect(Reverb::typeIdString(), []() { return std::make_shared<Reverb>(); });
    registerEffect(Saturator::typeIdString(), []() { return std::make_shared<Saturator>(); });
    registerEffect(TubeStage::typeIdString(), []() { return std::make_shared<TubeStage>(); });
    registerEffect(VintagePassiveEq::typeIdString(), []() { return std::make_shared<VintagePassiveEq>(); });
    registerEffect(SimpleEq::typeIdString(), []() { return std::make_shared<SimpleEq>(); });

    // Readable-string aliases so the gallery can create effects by their type() name
    registerEffect(Constants::RackEffectType::autoDucker().toStdString(), []() { return std::make_shared<AutoDucker>(); });
    registerEffect(Constants::RackEffectType::crossfeed().toStdString(), []() { return std::make_shared<Crossfeed>(); });
    registerEffect(Constants::RackEffectType::bitCrusher().toStdString(), []() { return std::make_shared<BitCrusher>(); });
    registerEffect(Constants::RackEffectType::flanger().toStdString(), []() { return std::make_shared<Flanger>(); });
    registerEffect(Constants::RackEffectType::gate().toStdString(), []() { return std::make_shared<Gate>(); });
    registerEffect(Constants::RackEffectType::tremolo().toStdString(), []() { return std::make_shared<Tremolo>(); });
    registerEffect(Constants::RackEffectType::reference().toStdString(), []() { return std::make_shared<Reference>(); });
    registerEffect(Constants::RackEffectType::autoFilter().toStdString(), []() { return std::make_shared<AutoFilter>(); });
    registerEffect(Constants::RackEffectType::autoPanner().toStdString(), []() { return std::make_shared<AutoPanner>(); });
    registerEffect(Constants::RackEffectType::endless().toStdString(), []() { return std::make_shared<EndlessReverb>(); });
    registerEffect(Constants::RackEffectType::clipper().toStdString(), []() { return std::make_shared<Clipper>(); });
    registerEffect(Constants::RackEffectType::compressor().toStdString(), []() { return std::make_shared<Compressor>(); });
    registerEffect(Constants::RackEffectType::delay().toStdString(), []() { return std::make_shared<Delay>(); });
    registerEffect(Constants::RackEffectType::dimension().toStdString(), []() { return std::make_shared<Dimension>(); });
    registerEffect(Constants::RackEffectType::compressor2A().toStdString(), []() { return std::make_shared<Compressor2A>(); });
    registerEffect(Constants::RackEffectType::doubler().toStdString(), []() { return std::make_shared<Doubler>(); });
    registerEffect(Constants::RackEffectType::analogFuzz().toStdString(), []() { return std::make_shared<AnalogFuzz>(); });
    registerEffect(Constants::RackEffectType::bassGrinder().toStdString(), []() { return std::make_shared<BassGrinder>(); });
    registerEffect(Constants::RackEffectType::drive().toStdString(), []() { return std::make_shared<Drive>(); });
    registerEffect(Constants::RackEffectType::earlyReflections().toStdString(), []() { return std::make_shared<EarlyReflections>(); });
    registerEffect(Constants::RackEffectType::eq8BandParametric().toStdString(), []() { return std::make_shared<Eq8BandParametric>(); });
    registerEffect(Constants::RackEffectType::eq32BandGraphical().toStdString(), []() { return std::make_shared<Eq32BandGraphical>(); });
    registerEffect(Constants::RackEffectType::limiter().toStdString(), []() { return std::make_shared<Limiter>(); });
    registerEffect(Constants::RackEffectType::monitor().toStdString(), []() { return std::make_shared<Monitor>(); });
    registerEffect(Constants::RackEffectType::gain().toStdString(), []() { return std::make_shared<Gain>(); });
    registerEffect(Constants::RackEffectType::multibandCompressor().toStdString(), []() { return std::make_shared<MultibandCompressor>(); });
    registerEffect(Constants::RackEffectType::panner().toStdString(), []() { return std::make_shared<Panner>(); });
    registerEffect(Constants::RackEffectType::phaser().toStdString(), []() { return std::make_shared<Phaser>(); });
    registerEffect(Constants::RackEffectType::reverb().toStdString(), []() { return std::make_shared<Reverb>(); });
    registerEffect(Constants::RackEffectType::rta().toStdString(), []() { return std::make_shared<Rta>(); });
    registerEffect(Constants::RackEffectType::stereoFieldMeter().toStdString(), []() { return std::make_shared<StereoFieldMeter>(); });
    registerEffect(Constants::RackEffectType::saturator().toStdString(), []() { return std::make_shared<Saturator>(); });
    registerEffect(Constants::RackEffectType::vintagePassiveEq().toStdString(), []() { return std::make_shared<VintagePassiveEq>(); });
    registerEffect(Constants::RackEffectType::airBandEq().toStdString(), []() { return std::make_shared<AirBandEq>(); });
    registerEffect(Constants::RackEffectType::simpleEq().toStdString(), []() { return std::make_shared<SimpleEq>(); });
    registerEffect(Constants::RackEffectType::tubeStage().toStdString(), []() { return std::make_shared<TubeStage>(); });
    registerEffect(StereoEnhancer::typeIdString(), []() { return std::make_shared<StereoEnhancer>(); });
    registerEffect(Constants::RackEffectType::stereoEnhancer().toStdString(), []() { return std::make_shared<StereoEnhancer>(); });
    registerEffect(StereoExciter::typeIdString(), []() { return std::make_shared<StereoExciter>(); });
    registerEffect(Constants::RackEffectType::stereoExciter().toStdString(), []() { return std::make_shared<StereoExciter>(); });
    registerEffect(StereoWidener::typeIdString(), []() { return std::make_shared<StereoWidener>(); });
    registerEffect(Constants::RackEffectType::stereoWidener().toStdString(), []() { return std::make_shared<StereoWidener>(); });
    registerEffect(WaveDesigner::typeIdString(), []() { return std::make_shared<WaveDesigner>(); });
    registerEffect(Constants::RackEffectType::waveDesigner().toStdString(), []() { return std::make_shared<WaveDesigner>(); });

    // Legacy support
    registerLegacyEffect("auto_ducker", []() { return std::make_shared<AutoDucker>(); });
    registerLegacyEffect("cross_feed", []() { return std::make_shared<Crossfeed>(); });
    registerLegacyEffect("bit_crusher", []() { return std::make_shared<BitCrusher>(); });
    registerLegacyEffect("flanger", []() { return std::make_shared<Flanger>(); });
    registerLegacyEffect("gate", []() { return std::make_shared<Gate>(); });
    registerLegacyEffect("tremolo", []() { return std::make_shared<Tremolo>(); });
    registerLegacyEffect("reference_monitor", []() { return std::make_shared<Reference>(); });
    registerLegacyEffect("auto_filter", []() { return std::make_shared<AutoFilter>(); });
    registerLegacyEffect("multiband_compressor", []() { return std::make_shared<MultibandCompressor>(); });
    registerLegacyEffect("stereo_widener", []() { return std::make_shared<StereoWidener>(); });
    registerLegacyEffect("stereo_field_meter", []() { return std::make_shared<StereoFieldMeter>(); });
    registerLegacyEffect("early_reflections", []() { return std::make_shared<EarlyReflections>(); });
    registerLegacyEffect("multibandStereoWidth", []() { return std::make_shared<StereoWidener>(); });
    registerLegacyEffect("multiband_stereo_width", []() { return std::make_shared<StereoWidener>(); });
    registerLegacyEffect("analog_fuzz", []() { return std::make_shared<AnalogFuzz>(); });
    registerLegacyEffect("bass_grinder", []() { return std::make_shared<BassGrinder>(); });
    registerLegacyEffect("tube_stage", []() { return std::make_shared<TubeStage>(); });
    registerLegacyEffect("wave_designer", []() { return std::make_shared<WaveDesigner>(); });
    registerLegacyEffect("stereo_enhancer", []() { return std::make_shared<StereoEnhancer>(); });
    registerLegacyEffect("stereo_exciter", []() { return std::make_shared<StereoExciter>(); });
    registerLegacyEffect("auto_panner", []() { return std::make_shared<AutoPanner>(); });
    registerLegacyEffect("endless", []() { return std::make_shared<EndlessReverb>(); });
    registerLegacyEffect("chorus", []() { return std::make_shared<Chorus>(); });
    registerLegacyEffect("clipper", []() { return std::make_shared<Clipper>(); });
    registerLegacyEffect("compressor", []() { return std::make_shared<Compressor>(); });
    registerLegacyEffect("delay", []() { return std::make_shared<Delay>(); });
    registerLegacyEffect("eq_8_band_parametric", []() { return std::make_shared<Eq8BandParametric>(); });
    registerLegacyEffect("eq_32_band_graphical", []() { return std::make_shared<Eq32BandGraphical>(); });
    registerLegacyEffect("limiter", []() { return std::make_shared<Limiter>(); });
    registerLegacyEffect("monitor", []() { return std::make_shared<Monitor>(); });
    registerLegacyEffect("gain", []() { return std::make_shared<Gain>(); });
    registerLegacyEffect("panner", []() { return std::make_shared<Panner>(); });
    registerLegacyEffect("phaser", []() { return std::make_shared<Phaser>(); });
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
