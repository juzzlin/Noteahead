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

#include "eq_8_band_parametric.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/audio_context.hpp"
#include "eq_8_band_parametric_presets.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace noteahead {

double Eq8BandParametric::magnitudeDbAt(double frequency) const
{
    // Measured on bands of its own rather than on the ones processing audio. Two reasons, and either
    // alone would be enough: the processing bands are written by the audio thread and this is asked
    // from the user interface thread, and they are only brought up to date when a block is
    // processed -- so a dialog opened before anything played would draw a flat line over settings
    // that are not flat at all.
    double magnitude = 1.0;
    for (size_t i = 0; i < NumBands; i++) {
        auto band = bandFromParameters(i);
        band.updateCoefficients(m_sampleRate);
        magnitude *= band.filterMid.magnitudeAt(frequency, m_sampleRate);
        for (size_t stage = 1; stage < band.stages; stage++) {
            magnitude *= band.extraMid.at(stage - 1).magnitudeAt(frequency, m_sampleRate);
        }
    }
    // Floored, because a notch at its own centre is a division by nothing: the honest answer is
    // minus infinity and there is no curve to draw with it. This is far enough down to read as "all
    // the way off" on any scale a dialog would use.
    constexpr double floorMagnitude = 1.0e-6;
    return 20.0 * std::log10(std::max(floorMagnitude, magnitude));
}

std::optional<float> Eq8BandParametric::defaultQParameterValue(SvfFilter::Type type)
{
    // The Q each type opens at. The parameter is exponential over 0.1 to 10, so the value stored is
    // where that Q falls on it.
    const auto asParameterValue = [](double q) {
        return static_cast<float>(ParameterMapper::unmapExponential(q, 0.1, 10.0));
    };

    switch (type) {
    case SvfFilter::Type::Bell:
        // One octave across, measured at the half-gain points.
        return asParameterValue(1.4142);
    case SvfFilter::Type::LowShelf:
    case SvfFilter::Type::HighShelf:
    case SvfFilter::Type::LowCut:
    case SvfFilter::Type::HighCut:
        // Butterworth, which is the shelf that does not overshoot and the cut whose corner is flat.
        return asParameterValue(0.7071);
    case SvfFilter::Type::Notch:
        // A notch is for removing one thing, so it starts narrow enough to be worth reaching for.
        return asParameterValue(4.0);
    case SvfFilter::Type::Bypass:
    case SvfFilter::Type::BandPass:
        // Nothing is being shaped, so there is no width to have an opinion about.
        return std::nullopt;
    }
    return std::nullopt;
}

Eq8BandParametric::Eq8BandParametric()
{
    for (int i = 0; i < static_cast<int>(NumBands); i++) {
        addParameter(Parameter { Constants::NahdXml::xmlKeyBandType(i).toStdString(), 0.0f, 0, 6, 0, 1, Parameter::Type::Discrete, { std::format("eq8BandParametricBand{}Type", i + 1) } });
        addParameter(Parameter { Constants::NahdXml::xmlKeyBandFreq(i).toStdString(), 0.5f, 20, 20000, 1000, 100, Parameter::Type::Continuous, { std::format("eq8BandParametricBand{}Freq", i + 1) } });
        addParameter(Parameter { Constants::NahdXml::xmlKeyBandGain(i).toStdString(), 0.5f, -2400, 2400, 0, 100, Parameter::Type::Continuous, { std::format("eq8BandParametricBand{}Gain", i + 1) } });
        // 0.5753 maps to Q = 1.414, which is a bell exactly one octave wide -- the width a
        // parametric is expected to open at, and the one a musician reads as "a bit either side of
        // this note". The stored default said 10 where the live one said the equivalent of 50, and
        // only the live one was ever heard; they now agree.
        addParameter(Parameter { Constants::NahdXml::xmlKeyBandQ(i).toStdString(), 0.5753f, 1, 100, 58, 10, Parameter::Type::Continuous, { std::format("eq8BandParametricBand{}Q", i + 1) } });
        // 12 dB/oct, which is what a cut band has always been, so a project that never touches this
        // keeps the curve it was written with.
        addParameter(Parameter { Constants::NahdXml::xmlKeyBandSlope(i).toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    }

    addParameter(Parameter { Constants::NahdXml::xmlKeyStereoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });

    syncParameters();
}

void Eq8BandParametric::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    processStereo(left, right);
}

void Eq8BandParametric::processBlock(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processSample(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Eq8BandParametric::updateBuffers()
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

void Eq8BandParametric::processStereo(double & left, double & right)
{
    // The EQ always works in Mid/Side internally. Processing both channels identically (MidSide mode) is
    // mathematically equivalent to processing L/R independently, so it preserves the classic stereo behavior.
    double mid = (left + right) * 0.5;
    double side = (left - right) * 0.5;

    const bool processMid = m_stereoMode != StereoMode::Side;
    const bool processSide = m_stereoMode != StereoMode::Mid;

    for (auto & band : m_bands) {
        if (processMid) {
            mid = band.filterMid.process(mid);
            for (size_t stage = 1; stage < band.stages; stage++) {
                mid = band.extraMid.at(stage - 1).process(mid);
            }
        }
        if (processSide) {
            side = band.filterSide.process(side);
            for (size_t stage = 1; stage < band.stages; stage++) {
                side = band.extraSide.at(stage - 1).process(side);
            }
        }
    }

    left = mid + side;
    right = mid - side;
}

void Eq8BandParametric::reset()
{
    for (auto & band : m_bands) {
        band.reset();
    }
}

void Eq8BandParametric::sync()
{
    m_shouldUpdateBuffers = true;
}

Eq8BandParametric::Band Eq8BandParametric::bandFromParameters(size_t index) const
{
    Band band;
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBandType(index).toStdString()); p) {
        band.type = static_cast<SvfFilter::Type>(std::clamp(static_cast<int>(std::round(p->get().value())), 0, 6));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBandFreq(index).toStdString()); p) {
        band.frequency = ParameterMapper::mapLogFrequency(static_cast<double>(p->get().value()), 20.0, 20000.0);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBandGain(index).toStdString()); p) {
        band.gainDb = -24.0 + static_cast<double>(p->get().value()) * 48.0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBandQ(index).toStdString()); p) {
        band.q = ParameterMapper::mapExponential(static_cast<double>(p->get().value()), 0.1, 10.0);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBandSlope(index).toStdString()); p) {
        band.slope = static_cast<int>(p->get().value());
    }
    return band;
}

void Eq8BandParametric::syncParameters()
{
    for (size_t i = 0; i < NumBands; i++) {
        auto && band = m_bands[i];
        const auto settings = bandFromParameters(i);
        band.type = settings.type;
        band.frequency = settings.frequency;
        band.gainDb = settings.gainDb;
        band.q = settings.q;
        band.slope = settings.slope;
        band.updateCoefficients(m_sampleRate);
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyStereoMode().toStdString()); p) {
        m_stereoMode = static_cast<StereoMode>(std::clamp(static_cast<int>(std::round(p->get().value())), 0, 2));
    }
}

const EffectPresetList & Eq8BandParametric::factoryPresets() const
{
    return Eq8BandParametricPresets::presets();
}

std::string Eq8BandParametric::typeIdString()
{
    return "b2e1f3a4-c5d6-4e7f-8a9b-0c1d2e3f4a5b";
}

std::string Eq8BandParametric::type() const
{
    return Constants::RackEffectType::eq8BandParametric().toStdString();
}

std::string Eq8BandParametric::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
