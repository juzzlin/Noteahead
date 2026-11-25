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

#include "analog_bus_presets.hpp"

#include "analog_bus.hpp"

#include "../../../common/constants.hpp"

#include <map>
#include <string>

namespace noteahead {

namespace {

EffectPreset makePreset(std::string name, std::map<std::string, float> values)
{
    return EffectPreset { std::move(name), std::move(values) };
}

} // namespace

const EffectPresetList & AnalogBusPresets::presets()
{
    namespace C = Constants::NahdXml;

    static const EffectPresetList list {
        makePreset("Init", {}),
        makePreset("Mix Bus Glue", {
                                     { C::xmlKeyLowCutMode().toStdString(), static_cast<float>(AnalogBus::LowCutMode::Mix25Hz) },
                                     { C::xmlKeyRouting().toStdString(), static_cast<float>(AnalogBus::Routing::AToN) },
                                     { C::xmlKeyDriveA().toStdString(), AnalogBus::drivePosition(10.0) },
                                     { C::xmlKeyDriveN().toStdString(), AnalogBus::drivePosition(8.0) },
                                     { C::xmlKeyColorMode().toStdString(), static_cast<float>(AnalogBus::ColorMode::Bus4000) },
                                     { C::xmlKeyColorDrive().toStdString(), 0.25f },
                                     { C::xmlKeyColorMix().toStdString(), 0.60f },
                                     { C::xmlKeyLowFreq().toStdString(), static_cast<float>(AnalogBus::LowFreq::Sub2_45Hz) },
                                     { C::xmlKeyLowGain().toStdString(), AnalogBus::eqGainPosition(1.5) },
                                     { C::xmlKeyAirGain().toStdString(), AnalogBus::airGainPosition(1.5) },
                                     { C::xmlKeyAspect().toStdString(), 0.25f },
                                   }),
        makePreset("American Punch", {
                                       { C::xmlKeyLowCutMode().toStdString(), static_cast<float>(AnalogBus::LowCutMode::Track50Hz) },
                                       { C::xmlKeyRouting().toStdString(), static_cast<float>(AnalogBus::Routing::A) },
                                       { C::xmlKeyDriveA().toStdString(), AnalogBus::drivePosition(18.0) },
                                       { C::xmlKeyEqPre().toStdString(), 1.0f },
                                       { C::xmlKeyLowFreq().toStdString(), static_cast<float>(AnalogBus::LowFreq::Bass_65Hz) },
                                       { C::xmlKeyLowGain().toStdString(), AnalogBus::eqGainPosition(2.0) },
                                       { C::xmlKeyHighFreq().toStdString(), static_cast<float>(AnalogBus::HighFreq::Presence_4k5Hz) },
                                       { C::xmlKeyHighGain().toStdString(), AnalogBus::eqGainPosition(1.5) },
                                     }),
        makePreset("British Iron", {
                                     { C::xmlKeyRouting().toStdString(), static_cast<float>(AnalogBus::Routing::N) },
                                     { C::xmlKeyDriveN().toStdString(), AnalogBus::drivePosition(20.0) },
                                     { C::xmlKeyLowFreq().toStdString(), static_cast<float>(AnalogBus::LowFreq::Sub2_45Hz) },
                                     { C::xmlKeyLowGain().toStdString(), AnalogBus::eqGainPosition(2.5) },
                                     { C::xmlKeyHighFreq().toStdString(), static_cast<float>(AnalogBus::HighFreq::Brite_8kHz) },
                                     { C::xmlKeyHighGain().toStdString(), AnalogBus::eqGainPosition(1.0) },
                                     { C::xmlKeyVintage().toStdString(), 1.0f },
                                     { C::xmlKeyAnalog().toStdString(), 1.0f },
                                   }),
        makePreset("Master Tape Glue", {
                                         { C::xmlKeyRouting().toStdString(), static_cast<float>(AnalogBus::Routing::AToN) },
                                         { C::xmlKeyDriveA().toStdString(), AnalogBus::drivePosition(12.0) },
                                         { C::xmlKeyDriveN().toStdString(), AnalogBus::drivePosition(12.0) },
                                         { C::xmlKeyColorMode().toStdString(), static_cast<float>(AnalogBus::ColorMode::Tape) },
                                         { C::xmlKeyColorDrive().toStdString(), 0.55f },
                                         { C::xmlKeyColorMix().toStdString(), 0.80f },
                                         { C::xmlKeyLowFreq().toStdString(), static_cast<float>(AnalogBus::LowFreq::Sub1_30Hz) },
                                         { C::xmlKeyLowGain().toStdString(), AnalogBus::eqGainPosition(1.5) },
                                         { C::xmlKeyHighFreq().toStdString(), static_cast<float>(AnalogBus::HighFreq::Sheen_14kHz) },
                                         { C::xmlKeyHighGain().toStdString(), AnalogBus::eqGainPosition(2.0) },
                                         { C::xmlKeyAnalog().toStdString(), 1.0f },
                                         { C::xmlKeyAspect().toStdString(), 0.20f },
                                       }),
        makePreset("Vinyl Warmth", {
                                     { C::xmlKeyRouting().toStdString(), static_cast<float>(AnalogBus::Routing::NToA) },
                                     { C::xmlKeyDriveA().toStdString(), AnalogBus::drivePosition(10.0) },
                                     { C::xmlKeyDriveN().toStdString(), AnalogBus::drivePosition(14.0) },
                                     { C::xmlKeyColorMode().toStdString(), static_cast<float>(AnalogBus::ColorMode::Vinyl) },
                                     { C::xmlKeyColorDrive().toStdString(), 0.50f },
                                     { C::xmlKeyColorMix().toStdString(), 0.75f },
                                     { C::xmlKeyLowFreq().toStdString(), static_cast<float>(AnalogBus::LowFreq::Body_110Hz) },
                                     { C::xmlKeyLowGain().toStdString(), AnalogBus::eqGainPosition(1.5) },
                                     { C::xmlKeyVintage().toStdString(), 1.0f },
                                     { C::xmlKeyAspect().toStdString(), 0.30f },
                                   }),
        makePreset("12-Bit Sampler Bus", {
                                           { C::xmlKeyLowCutMode().toStdString(), static_cast<float>(AnalogBus::LowCutMode::Track50Hz) },
                                           { C::xmlKeyRouting().toStdString(), static_cast<float>(AnalogBus::Routing::AToN) },
                                           { C::xmlKeyDriveA().toStdString(), AnalogBus::drivePosition(15.0) },
                                           { C::xmlKeyDriveN().toStdString(), AnalogBus::drivePosition(12.0) },
                                           { C::xmlKeyColorMode().toStdString(), static_cast<float>(AnalogBus::ColorMode::LoFi12) },
                                           { C::xmlKeyColorDrive().toStdString(), 0.65f },
                                           { C::xmlKeyColorMix().toStdString(), 0.85f },
                                           { C::xmlKeyLowFreq().toStdString(), static_cast<float>(AnalogBus::LowFreq::Body_110Hz) },
                                           { C::xmlKeyLowGain().toStdString(), AnalogBus::eqGainPosition(2.0) },
                                           { C::xmlKeyVintage().toStdString(), 1.0f },
                                         }),
        makePreset("Parallel Smash", {
                                       { C::xmlKeyRouting().toStdString(), static_cast<float>(AnalogBus::Routing::AToN) },
                                       { C::xmlKeyDriveA().toStdString(), AnalogBus::drivePosition(30.0) },
                                       { C::xmlKeyDriveN().toStdString(), AnalogBus::drivePosition(28.0) },
                                       { C::xmlKeyColorMode().toStdString(), static_cast<float>(AnalogBus::ColorMode::Bus4000) },
                                       { C::xmlKeyColorDrive().toStdString(), 0.70f },
                                       { C::xmlKeyColorMix().toStdString(), 1.0f },
                                       { C::xmlKeyEqPre().toStdString(), 1.0f },
                                       { C::xmlKeyLowFreq().toStdString(), static_cast<float>(AnalogBus::LowFreq::Sub2_45Hz) },
                                       { C::xmlKeyLowGain().toStdString(), AnalogBus::eqGainPosition(3.0) },
                                       { C::xmlKeyHighFreq().toStdString(), static_cast<float>(AnalogBus::HighFreq::Presence_4k5Hz) },
                                       { C::xmlKeyHighGain().toStdString(), AnalogBus::eqGainPosition(2.0) },
                                       { C::xmlKeyGain().toStdString(), AnalogBus::outputGainPosition(-1.5) },
                                       { C::xmlKeyMix().toStdString(), 0.35f },
                                     }),
        makePreset("Wide Airy Synth", {
                                        { C::xmlKeyRouting().toStdString(), static_cast<float>(AnalogBus::Routing::A) },
                                        { C::xmlKeyDriveA().toStdString(), AnalogBus::drivePosition(11.0) },
                                        { C::xmlKeyColorMode().toStdString(), static_cast<float>(AnalogBus::ColorMode::Tape) },
                                        { C::xmlKeyColorDrive().toStdString(), 0.30f },
                                        { C::xmlKeyColorMix().toStdString(), 0.50f },
                                        { C::xmlKeyLowFreq().toStdString(), static_cast<float>(AnalogBus::LowFreq::Sub2_45Hz) },
                                        { C::xmlKeyLowGain().toStdString(), AnalogBus::eqGainPosition(1.0) },
                                        { C::xmlKeyHighFreq().toStdString(), static_cast<float>(AnalogBus::HighFreq::Brite_8kHz) },
                                        { C::xmlKeyHighGain().toStdString(), AnalogBus::eqGainPosition(2.0) },
                                        { C::xmlKeyAirGain().toStdString(), AnalogBus::airGainPosition(4.5) },
                                        { C::xmlKeyAspect().toStdString(), 0.65f },
                                        { C::xmlKeyAnalog().toStdString(), 1.0f },
                                      }),
    };
    return list;
}

} // namespace noteahead
