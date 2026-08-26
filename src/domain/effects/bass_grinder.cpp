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

#include "bass_grinder.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../dsp/upsampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

namespace {

//! Drive range in dB at the top of the control. Generous: the point of this effect is to be able to
//! ruin the top of a kick on purpose.
constexpr double MaxDriveDb = 40.0;

//! Output trim range either side of unity, in dB.
constexpr double OutputRangeDb = 12.0;

//! Cut and boost the tone stack reaches at either extreme, in dB.
constexpr double EqRangeDb = 15.0;

//! Travel of the Split control. The bottom sits below the audio band, so everything reaches the
//! clipper and the effect behaves like a plain full-band blend.
constexpr double MinSplitHz = 20.0;
constexpr double MaxSplitHz = 800.0;

//! Travel of the sweepable mid bell: from the box a kick sits in up to its click.
constexpr double MinMidHz = 200.0;
constexpr double MaxMidHz = 3000.0;

//! Q of the split. Butterworth, so the band the clipper sees and the one that bypasses it cross over
//! without a bump at the corner.
constexpr double SplitQ = 0.707;

//! Fixed corners of the tone stack. The mid's is swept by its own control.
constexpr double BassShelfHz = 80.0;
constexpr double HighShelfHz = 3000.0;
constexpr double ShelfQ = 0.707;
constexpr double MidQ = 0.8;

//! The Color voicing: a scooped smile. Mids out of the way, weight underneath and edge on top, which
//! is what turns the clipper's output from woolly into mean.
constexpr double ColorScoopHz = 500.0;
constexpr double ColorScoopQ = 0.7;
constexpr double ColorScoopDb = -8.0;
constexpr double ColorLowHz = 80.0;
constexpr double ColorLowDb = 3.0;
constexpr double ColorHighHz = 3500.0;
constexpr double ColorHighDb = 3.0;

//! Forward thresholds of the two diodes. Different, so the curve is not odd-symmetric and the stage
//! generates even harmonics.
constexpr double PositiveThreshold = 1.0;
constexpr double NegativeThreshold = 0.72;

//! Standing bias into the clipper, the way a single-supply stage idles off a half-rail reference.
//!
//! Mismatched thresholds alone are not enough: driven hard, both halves saturate and what comes out
//! is a symmetric square once its offset is removed, so the even harmonics vanish exactly where the
//! effect is supposed to have the most character. A bias ahead of the drive gain shifts the zero
//! crossings instead of the levels, which changes the duty cycle, and a duty cycle that is not half
//! stays asymmetric however hard it is driven.
constexpr double ClipperBias = 0.05;

//! Release of the saturation meter, so it falls back at a readable speed rather than flickering.
constexpr double MeterReleaseMs = 100.0;

} // namespace

struct BassGrinder::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;

    //! The untouched signal, carried through a decimator of its own so that the Mix dry path has the
    //! same resampling latency as the wet one. Blending against the raw input would comb.
    Decimator dryDecimatorL;
    Decimator dryDecimatorR;
};

void BassGrinder::StereoFilter::reset()
{
    left.reset();
    right.reset();
}

BassGrinder::BassGrinder()
  : m_oversampling { std::make_unique<Oversampling>() }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyDrive().toStdString(), 0.125f, 0, 4000, 500, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyBlend().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeySplitFreq().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyColor().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyBassGain().toStdString(), 0.5f, -1500, 1500, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMidGain().toStdString(), 0.5f, -1500, 1500, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMidFreq().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHighGain().toStdString(), 0.5f, -1500, 1500, 0, 100, Parameter::Type::Continuous });
    addMixParameter(1.0f, MixLaw::Internal);
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });

    syncParameters();
}

BassGrinder::~BassGrinder() = default;

std::string BassGrinder::typeIdString()
{
    return "b2f7c4a8-3d19-4e05-9a6b-8c1d0e7f5432";
}

std::string BassGrinder::type() const
{
    return Constants::RackEffectType::bassGrinder().toStdString();
}

std::string BassGrinder::typeId() const
{
    return typeIdString();
}

double BassGrinder::clip(double x) const
{
    // Exponential diode curve. Soft at low level, hard against the ceiling, and asymptotic to the
    // threshold of whichever half we are on, so it cannot overshoot.
    const double threshold = x >= 0.0 ? PositiveThreshold : NegativeThreshold;
    return std::copysign(threshold * (1.0 - std::exp(-std::abs(x) / threshold)), x);
}

void BassGrinder::updateSplit(double splitSampleRate)
{
    const double frequency = static_cast<double>(m_splitFreq);
    if (frequency == m_splitCoeffFreq && splitSampleRate == m_splitCoeffSampleRate) {
        return;
    }
    m_splitL.calculateLowCut(frequency, splitSampleRate, SplitQ);
    m_splitR.calculateLowCut(frequency, splitSampleRate, SplitQ);
    m_postSplitL.calculateLowCut(frequency, splitSampleRate, SplitQ);
    m_postSplitR.calculateLowCut(frequency, splitSampleRate, SplitQ);
    m_splitCoeffFreq = frequency;
    m_splitCoeffSampleRate = splitSampleRate;
}

double BassGrinder::grind(SvfFilter & split, SvfFilter & postSplit, double sample, double driveLin, double quiescent)
{
    // Complementary split: what the high-pass rejects is exactly what is left over, so the two bands
    // sum back to the input with no crossover ripple to work around.
    const double high = split.process(sample);
    const double low = sample - high;

    const double driven = (high + ClipperBias) * driveLin;
    m_peakPre = std::max(m_peakPre, std::abs(high * driveLin));
    const double clipped = clip(driven) - quiescent;
    m_peakPost = std::max(m_peakPost, std::abs(clipped));

    // The clipper's output is high-passed again at the same corner. Without this the split only keeps
    // the low band out of the clipper on the way in, and not out of the result: what leaks past the
    // split comes back at nearly the opposite phase, and driven hard enough it cancels the very
    // fundamental the split exists to protect. Filtering the clipped band means nothing it generates
    // below the corner can reach the output at all, at any drive.
    const double blend = static_cast<double>(m_blend);
    return low + high * (1.0 - blend) + postSplit.process(clipped) * blend;
}

void BassGrinder::processSample(double & left, double & right)
{
    const double baseSampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    if (baseSampleRate != m_lastSampleRate) {
        m_lastSampleRate = baseSampleRate;
        m_shouldUpdateToneStack = true;
    }
    if (m_shouldUpdateToneStack) {
        updateToneStack();
        m_shouldUpdateToneStack = false;
    }

    const uint8_t factor = clampOversampleFactor(oversampleFactor());
    updateSplit(baseSampleRate * static_cast<double>(factor));

    const double driveLin = static_cast<double>(Utils::Dsp::dbToLinear(m_drive * static_cast<float>(MaxDriveDb)));
    // What the clipper idles at with the bias but no signal. Subtracting it keeps silence silent, so
    // moving Drive cannot thump the rack.
    const double quiescent = clip(ClipperBias * driveLin);
    const double outputLin = static_cast<double>(Utils::Dsp::dbToLinear(m_outputDb));
    const double mix = static_cast<double>(m_mix);

    m_peakPre = 0.0;
    m_peakPost = 0.0;

    double wetL = 0.0;
    double wetR = 0.0;
    double dryL = left;
    double dryR = right;

    if (factor == 1) {
        wetL = grind(m_splitL, m_postSplitL, left, driveLin, quiescent);
        wetR = grind(m_splitR, m_postSplitR, right, driveLin, quiescent);
    } else {
        // Split and clip at the oversampled rate: the harmonics the clipper generates then fold above
        // Nyquist and are removed by the decimation filter rather than aliasing back into the band.
        // The split runs up here too, so the clean low band shares the wet band's phase exactly.
        std::array<float, 4> highL {};
        std::array<float, 4> highR {};
        std::array<float, 4> dryHighL {};
        std::array<float, 4> dryHighR {};
        m_oversampling->upsamplerL.process(static_cast<float>(left), highL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(right), highR.data(), factor);
        for (uint8_t k = 0; k < factor; k++) {
            dryHighL[k] = highL[k];
            dryHighR[k] = highR[k];
            highL[k] = static_cast<float>(grind(m_splitL, m_postSplitL, static_cast<double>(highL[k]), driveLin, quiescent));
            highR[k] = static_cast<float>(grind(m_splitR, m_postSplitR, static_cast<double>(highR[k]), driveLin, quiescent));
        }
        wetL = static_cast<double>(m_oversampling->decimatorL.process(highL.data(), factor));
        wetR = static_cast<double>(m_oversampling->decimatorR.process(highR.data(), factor));
        dryL = static_cast<double>(m_oversampling->dryDecimatorL.process(dryHighL.data(), factor));
        dryR = static_cast<double>(m_oversampling->dryDecimatorR.process(dryHighR.data(), factor));
    }

    // The asymmetric curve leaves a DC offset that no downstream effect should have to deal with.
    // Blocking it at the base rate is enough, since DC survives decimation unchanged.
    m_dcBlockerL.setSampleRate(baseSampleRate);
    m_dcBlockerR.setSampleRate(baseSampleRate);
    wetL = m_dcBlockerL.process(wetL);
    wetR = m_dcBlockerR.process(wetR);

    if (m_color) {
        wetL = m_colorHigh.left.process(m_colorLow.left.process(m_colorScoop.left.process(wetL)));
        wetR = m_colorHigh.right.process(m_colorLow.right.process(m_colorScoop.right.process(wetR)));
    }

    wetL = m_high.left.process(m_mid.left.process(m_bass.left.process(wetL)));
    wetR = m_high.right.process(m_mid.right.process(m_bass.right.process(wetR)));

    double saturationDb = 0.0;
    if (m_peakPre > 1e-10 && m_peakPost < m_peakPre) {
        saturationDb = Utils::Dsp::linearToDb(static_cast<float>(m_peakPost / m_peakPre));
    }

    const double meterReleaseCoeff = std::exp(-1.0 / (MeterReleaseMs * baseSampleRate / 1000.0));
    if (saturationDb < m_saturationDb) {
        m_saturationDb = saturationDb;
    } else {
        m_saturationDb = meterReleaseCoeff * m_saturationDb + (1.0 - meterReleaseCoeff) * saturationDb;
    }

    // Denormal protection
    if (std::abs(m_saturationDb) < 1.0e-15) {
        m_saturationDb = 0.0;
    }

    left = blendWet(dryL, wetL, mix, outputLin);
    right = blendWet(dryR, wetR, mix, outputLin);
}

void BassGrinder::updateToneStack()
{
    const double sampleRate = m_lastSampleRate > 0 ? m_lastSampleRate : 48000.0;

    const auto shelfLow = [&](StereoFilter & filter, double frequency, double gainDb) {
        filter.left.calculateLowShelf(frequency, sampleRate, ShelfQ, gainDb);
        filter.right.calculateLowShelf(frequency, sampleRate, ShelfQ, gainDb);
    };
    const auto shelfHigh = [&](StereoFilter & filter, double frequency, double gainDb) {
        filter.left.calculateHighShelf(frequency, sampleRate, ShelfQ, gainDb);
        filter.right.calculateHighShelf(frequency, sampleRate, ShelfQ, gainDb);
    };
    const auto bell = [&](StereoFilter & filter, double frequency, double q, double gainDb) {
        filter.left.calculateBell(frequency, sampleRate, q, gainDb);
        filter.right.calculateBell(frequency, sampleRate, q, gainDb);
    };

    if (m_color) {
        bell(m_colorScoop, ColorScoopHz, ColorScoopQ, ColorScoopDb);
        shelfLow(m_colorLow, ColorLowHz, ColorLowDb);
        shelfHigh(m_colorHigh, ColorHighHz, ColorHighDb);
    } else {
        m_colorScoop.left.setBypass();
        m_colorScoop.right.setBypass();
        m_colorLow.left.setBypass();
        m_colorLow.right.setBypass();
        m_colorHigh.left.setBypass();
        m_colorHigh.right.setBypass();
    }

    shelfLow(m_bass, BassShelfHz, static_cast<double>(m_bassGainDb));
    bell(m_mid, static_cast<double>(m_midFreq), MidQ, static_cast<double>(m_midGainDb));
    shelfHigh(m_high, HighShelfHz, static_cast<double>(m_highGainDb));
}

void BassGrinder::reset()
{
    m_saturationDb = 0.0;
    m_peakPre = 0.0;
    m_peakPost = 0.0;
    m_splitL.reset();
    m_splitR.reset();
    m_postSplitL.reset();
    m_postSplitR.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
    m_colorScoop.reset();
    m_colorLow.reset();
    m_colorHigh.reset();
    m_bass.reset();
    m_mid.reset();
    m_high.reset();
    m_oversampling->upsamplerL.reset();
    m_oversampling->upsamplerR.reset();
    m_oversampling->decimatorL.reset();
    m_oversampling->decimatorR.reset();
    m_oversampling->dryDecimatorL.reset();
    m_oversampling->dryDecimatorR.reset();
}

void BassGrinder::sync()
{
    syncParameters();
}

float BassGrinder::saturationDb() const
{
    return static_cast<float>(m_saturationDb);
}

void BassGrinder::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDrive().toStdString()); p) {
        m_drive = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBlend().toStdString()); p) {
        m_blend = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeySplitFreq().toStdString()); p) {
        m_splitFreq = static_cast<float>(ParameterMapper::mapLogFrequency(p->get().value(), MinSplitHz, MaxSplitHz));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyColor().toStdString()); p) {
        m_color = static_cast<int>(p->get().value()) == 1;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBassGain().toStdString()); p) {
        m_bassGainDb = static_cast<float>((p->get().value() - 0.5f) * 2.0 * EqRangeDb);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMidGain().toStdString()); p) {
        m_midGainDb = static_cast<float>((p->get().value() - 0.5f) * 2.0 * EqRangeDb);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMidFreq().toStdString()); p) {
        m_midFreq = static_cast<float>(ParameterMapper::mapLogFrequency(p->get().value(), MinMidHz, MaxMidHz));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHighGain().toStdString()); p) {
        m_highGainDb = static_cast<float>((p->get().value() - 0.5f) * 2.0 * EqRangeDb);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        m_mix = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_outputDb = static_cast<float>((p->get().value() - 0.5f) * 2.0 * OutputRangeDb);
    }

    m_shouldUpdateToneStack = true;
}

} // namespace noteahead
