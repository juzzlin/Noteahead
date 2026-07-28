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

#include "air_band_eq.hpp"

#include "../../common/constants.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

// Fixed band-pass centres, mirroring the front panel. The first four are bells; 2.5 kHz is a shelf.
constexpr std::array<double, AirBandEq::BellCount> BellFreqs { 10.0, 40.0, 160.0, 650.0 };
constexpr double ShelfFreq = 2500.0;

// AIR BAND selector positions. Index 0 is the panel's OFF detent.
constexpr std::array<double, 6> AirFreqs { 0.0, 2500.0, 5000.0, 10000.0, 20000.0, 40000.0 };
constexpr size_t AirOffIndex = 0;

// Documented panel ranges. The band passes share +15 dB of boost against 4.5 dB of cut; AIR GAIN is
// boost only. The output trim has no hardware counterpart: the original expects you to pull the
// five band knobs down by hand to offset the air band's added gain, which is worth keeping possible
// but tedious as the only option.
constexpr double MaxBandBoostDb = 15.0;
constexpr double MaxBandCutDb = 4.5;
constexpr double MaxAirGainDb = 20.0;
constexpr double MaxOutputTrimDb = 12.0;

// The bell centres sit two octaves apart, so a Q of about 0.667 gives each tap a two-octave -3 dB
// bandwidth. Neighbouring bands then overlap enough that moving all five together shifts the whole
// curve instead of rippling it, which is the interaction the hardware is built around.
constexpr double BandQ = 0.667;

//! Coefficient a tap is scaled by before being summed into the dry path.
//!
//! The dry path already contributes unity at every frequency, so scaling the tap by (linear gain -
//! 1) lands the summed response on the requested gain at the band centre. Zero leaves the dry
//! signal untouched. The asymmetry of the panel range is a consequence rather than a choice: a
//! coefficient of -1 would null the dry signal completely, so the cut side runs out of headroom
//! after a few dB while the boost side does not.
double bandMixCoefficient(double normalized)
{
    const double position = (normalized - 0.5) * 2.0;
    const double gainDb = position >= 0.0 ? position * MaxBandBoostDb : position * MaxBandCutDb;
    return std::pow(10.0, gainDb / 20.0) - 1.0;
}

double airMixCoefficient(double normalized)
{
    return std::pow(10.0, std::clamp(normalized, 0.0, 1.0) * MaxAirGainDb / 20.0) - 1.0;
}

double outputGainFactor(double normalized)
{
    const double position = (normalized - 0.5) * 2.0;
    return std::pow(10.0, position * MaxOutputTrimDb / 20.0);
}

} // namespace

void AirBandEq::ChannelState::reset()
{
    for (auto & bell : bells) {
        bell.reset();
    }
    shelf.reset();
    air.reset();
}

AirBandEq::AirBandEq()
{
    namespace C = Constants::NahdXml;

    // Stored in panel units rather than dB: the band knobs are detented -5..+5 about a flat centre
    // and AIR GAIN runs 0..5, which keeps the asymmetric dB mapping an implementation detail.
    for (size_t i = 0; i < BandCount; i++) {
        addParameter(Parameter { C::xmlKeyBandGain(i).toStdString(), 0.5f, -500, 500, 0, 100, Parameter::Type::Continuous });
    }
    addParameter(Parameter { C::xmlKeyAirFreq().toStdString(), 3.0f, 0, static_cast<int>(AirFreqs.size()) - 1, 3, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyAirGain().toStdString(), 0.0f, 0, 500, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });

    syncParameters();
}

void AirBandEq::process(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    processStereo(left, right);
}

void AirBandEq::process(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processStereo(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void AirBandEq::updateBuffers()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldUpdateBuffers) {
        m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
        m_shouldUpdateBuffers = false;
        m_shouldSyncParameters = true;
    }

    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
}

void AirBandEq::processStereo(double & left, double & right)
{
    left = processChannel(left, m_channels[0]);
    right = processChannel(right, m_channels[1]);
}

double AirBandEq::processChannel(double input, ChannelState & state)
{
    // Every tap is fed the dry input, never the running sum. That is what makes the bands interact
    // by addition instead of stacking multiplicatively the way a cascade would.
    double output = input;

    for (size_t i = 0; i < BellCount; i++) {
        output += m_bandGains[i] * state.bells[i].process(input);
    }

    state.shelf.process(input);
    output += m_bandGains[BellCount] * state.shelf.highPass();

    state.air.process(input);
    output += m_airGain * state.air.highPass();

    return output * m_outputGain;
}

void AirBandEq::reset()
{
    for (auto & channel : m_channels) {
        channel.reset();
    }
}

void AirBandEq::sync()
{
    m_shouldUpdateBuffers = true;
}

void AirBandEq::syncParameters()
{
    namespace C = Constants::NahdXml;

    const auto valueOf = [this](const QString & key) {
        const auto p = parameter(key.toStdString());
        return p ? static_cast<double>(p->get().value()) : 0.0;
    };

    for (size_t i = 0; i < BandCount; i++) {
        m_bandGains[i] = bandMixCoefficient(valueOf(C::xmlKeyBandGain(i)));
    }

    const auto airIndex = std::clamp(static_cast<size_t>(std::lround(valueOf(C::xmlKeyAirFreq()))), size_t { 0 }, AirFreqs.size() - 1);

    // A first-order tap's magnitude below its corner is proportional to frequency/corner, so when
    // the requested corner sits above what the sample rate can represent, scaling the coefficient by
    // the ratio of the clamped corner to the requested one restores the intended skirt exactly in
    // the region where it is audible. Without this the 40 kHz position would collapse onto 20 kHz
    // at 44.1 kHz instead of staying the gentler of the two.
    const double selectedFreq = AirFreqs[airIndex];
    const double maxCorner = OnePoleFilter::maxCorner(m_sampleRate);
    const double airCorner = std::min(selectedFreq, maxCorner);
    const double skirtScale = selectedFreq > maxCorner ? airCorner / selectedFreq : 1.0;

    m_airGain = airIndex == AirOffIndex ? 0.0 : airMixCoefficient(valueOf(C::xmlKeyAirGain())) * skirtScale;

    for (auto & channel : m_channels) {
        for (size_t i = 0; i < BellCount; i++) {
            channel.bells[i].calculateBandPass(BellFreqs[i], m_sampleRate, BandQ);
        }
        channel.shelf.calculate(ShelfFreq, m_sampleRate);
        // Kept in sync even while OFF so that selecting a frequency does not click.
        channel.air.calculate(airIndex == AirOffIndex ? AirFreqs[1] : airCorner, m_sampleRate);
    }

    m_outputGain = outputGainFactor(valueOf(C::xmlKeyGain()));
}

std::string AirBandEq::typeIdString()
{
    return "b3f7c1d9-2a48-4e6c-9b17-5d0e8a4f2c33";
}

std::string AirBandEq::type() const
{
    return Constants::RackEffectType::airBandEq().toStdString();
}

std::string AirBandEq::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
