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

#include "analog_fuzz.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../dsp/upsampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

namespace {

//! Drive range in dB at the top of the control.
constexpr double MaxDriveDb = 42.0;

//! Level the drive stage clips at, as a fraction of full scale. This is the number that decides
//! whether the effect does anything at a usable Drive setting, and it is deliberately not 0 dBFS.
//!
//! A device in this rack puts out around -26 dBFS on a default patch: there is headroom left for a
//! whole song's worth of tracks. A stage whose knee sits at full scale never gets near it on a
//! signal that quiet, so the control has to be run right to the top before anything is heard, which
//! is exactly how this effect first came out. Putting the knee at -12 dBFS instead is the same as
//! saying the circuit was built for the levels it is actually being fed. It costs nothing at the
//! quiet end -- the curve is unity-slope through the origin either way -- so a low Drive setting
//! still passes the signal at the level it came in.
constexpr double StageHeadroom = 0.25;

//! Output trim range either side of unity, in dB.
constexpr double OutputRangeDb = 12.0;

//! How far Bias can push the operating point either side of centre. Reaches the starved, gating end
//! of a fuzz without parking the stage there for the whole top half of the control.
constexpr double BiasRange = 0.9;

//! Knee order at either end of the Fuzz control. 1 is the tanh-like soft valve knee; at the top the
//! curve holds linear almost to the ceiling and then gives way at once, which is the square-ish
//! clipping a fuzz circuit does.
constexpr double MinKnee = 1.0;
constexpr double MaxKnee = 12.0;

//! Corner range of the filter the stage is driven into.
constexpr double MinCutoffHz = 60.0;
constexpr double MaxCutoffHz = 12000.0;

//! Headroom of the filter the stage is driven into, as the level its integrators saturate at. Fixed,
//! because it is a property of the circuit rather than of the control: what Drive moves is how hot
//! the stage runs into it.
constexpr double FilterHeadroom = 1.0;

//! How hard the shaper's output is run into the filter, at the bottom of the Drive control and how
//! fast it grows from there. The shaper's output is bounded whatever it is fed, so without this the
//! filter would see the same level at every setting and its resonance would never give way.
//!
//! The floor is what keeps the two stages from fighting: run the filter deep into its ceiling at
//! every setting and it does all the shaping, the drive stage stops mattering and the Fuzz control
//! goes deaf. Starting below the ceiling leaves the filter nearly linear at the bottom of the
//! control -- where the resonance is intact and the shaper's own character comes through -- and
//! saturating at the top.
constexpr double FilterDriveFloor = 1.0;
constexpr double FilterDriveExponent = 0.25;

//! How much of the drive gain is taken back out again on the way past, as an exponent on it. Full
//! compensation would be right if the filter were linear, but it is not: the harder it is driven the
//! more of the level its own ceiling removes, so compensating in full on top of that walks the output
//! down as Drive comes up. Tuned so the level holds roughly steady across the control instead.
constexpr double LevelCompensationExponent = 0.29;

constexpr double MeterReleaseMs = 100.0;

} // namespace

struct AnalogFuzz::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;
};

AnalogFuzz::AnalogFuzz()
  : m_oversampling { std::make_unique<Oversampling>() }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyDrive().toStdString(), 0.4f, 0, 10000, 4000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFuzz().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyBias().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCutoff().toStdString(), 0.8f, 0, 10000, 8000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyResonance().toStdString(), 0.25f, 0, 10000, 2500, 100, Parameter::Type::Continuous });
    addMixParameter(1.0f, MixLaw::Internal);
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });

    syncParameters();
}

AnalogFuzz::~AnalogFuzz() = default;

std::string AnalogFuzz::typeIdString()
{
    return "b7f2c916-53ad-4e08-8c7d-2f61a94e0d35";
}

std::string AnalogFuzz::type() const
{
    return Constants::RackEffectType::analogFuzz().toStdString();
}

std::string AnalogFuzz::typeId() const
{
    return typeIdString();
}

double AnalogFuzz::shape(double x) const
{
    // Knee order: 1 is a soft valve-like curve, high orders stay linear and then break hard. Both
    // asymptote to the stage's headroom, so it cannot run away however far Drive is pushed.
    const double knee = MinKnee + (MaxKnee - MinKnee) * static_cast<double>(m_fuzz);
    const double magnitude = std::abs(x) / StageHeadroom;
    const double sign = x >= 0.0 ? 1.0 : -1.0;
    return sign * StageHeadroom * magnitude / std::pow(1.0 + std::pow(magnitude, knee), 1.0 / knee);
}

double AnalogFuzz::fuzz(SaturatingSvf & filter, double sample, double driveLin, double filterDrive, double & peakPre, double & peakPost)
{
    const double driven = sample * driveLin;
    peakPre = std::max(peakPre, std::abs(driven));

    // The drive stage, sitting where Bias puts it on the curve. Subtracting the quiescent point
    // keeps the stage idling at zero, so moving Bias does not step the output.
    const double shaped = shape(driven + m_biasOffset) - m_quiescent;
    peakPost = std::max(peakPost, std::abs(shaped));

    // ...and into the filter it is overdriving, which is where this stops being a shaper with a
    // filter behind it. How far past the filter's headroom this lands is what decides how much of
    // the resonant peak survives, and the harmonics just made go round the poles rather than
    // sitting on top of them.
    return filter.process(shaped * filterDrive);
}

void AnalogFuzz::processSample(double & left, double & right)
{
    const double baseSampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    const uint8_t factor = clampOversampleFactor(oversampleFactor());
    const double filterSampleRate = baseSampleRate * factor;

    const double driveLin = static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(static_cast<double>(m_drive) * MaxDriveDb)));
    const double outputLin = static_cast<double>(Utils::Dsp::dbToLinear(m_outputDb));
    const double mix = static_cast<double>(m_mix);

    // The filter runs inside the oversampled section, so its corner has to be resolved against the
    // rate the loop is actually running at.
    m_filterL.setSampleRate(filterSampleRate);
    m_filterR.setSampleRate(filterSampleRate);
    m_filterL.setCutoff(static_cast<double>(m_cutoff));
    m_filterR.setCutoff(static_cast<double>(m_cutoff));
    m_filterL.setResonance(static_cast<double>(m_resonance));
    m_filterR.setResonance(static_cast<double>(m_resonance));
    m_filterL.setSaturation(FilterHeadroom);
    m_filterR.setSaturation(FilterHeadroom);
    const double filterDrive = FilterDriveFloor * std::pow(driveLin, FilterDriveExponent);
    // Drive is a character control, so most of the gain it adds is taken back out here rather than
    // left for the Output trim to chase.
    const double compensation = std::pow(driveLin, -LevelCompensationExponent);

    const double dryL = left;
    const double dryR = right;

    double peakPre = 0.0;
    double peakPost = 0.0;
    double wetL = 0.0;
    double wetR = 0.0;

    if (factor == 1) {
        wetL = fuzz(m_filterL, dryL, driveLin, filterDrive, peakPre, peakPost);
        wetR = fuzz(m_filterR, dryR, driveLin, filterDrive, peakPre, peakPost);
    } else {
        std::array<float, 4> highL {};
        std::array<float, 4> highR {};
        m_oversampling->upsamplerL.process(static_cast<float>(dryL), highL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(dryR), highR.data(), factor);
        for (uint8_t k = 0; k < factor; k++) {
            highL[k] = static_cast<float>(fuzz(m_filterL, static_cast<double>(highL[k]), driveLin, filterDrive, peakPre, peakPost));
            highR[k] = static_cast<float>(fuzz(m_filterR, static_cast<double>(highR[k]), driveLin, filterDrive, peakPre, peakPost));
        }
        wetL = static_cast<double>(m_oversampling->decimatorL.process(highL.data(), factor));
        wetR = static_cast<double>(m_oversampling->decimatorR.process(highR.data(), factor));
    }

    wetL *= compensation;
    wetR *= compensation;

    // Bias leaves a standing offset the filter passes straight through, so it is blocked here rather
    // than handed to the rest of the rack. DC survives decimation unchanged, so the base rate is
    // where this belongs.
    m_dcBlockerL.setSampleRate(baseSampleRate);
    m_dcBlockerR.setSampleRate(baseSampleRate);
    wetL = m_dcBlockerL.process(wetL);
    wetR = m_dcBlockerR.process(wetR);

    double saturationDb = 0.0;
    if (peakPre > 1e-10 && peakPost < peakPre) {
        saturationDb = Utils::Dsp::linearToDb(static_cast<float>(peakPost / peakPre));
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

    left = (dryL * (1.0 - mix) + wetL * mix) * outputLin;
    right = (dryR * (1.0 - mix) + wetR * mix) * outputLin;
}

void AnalogFuzz::reset()
{
    m_saturationDb = 0.0;
    m_filterL.reset();
    m_filterR.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
    m_oversampling->upsamplerL.reset();
    m_oversampling->upsamplerR.reset();
    m_oversampling->decimatorL.reset();
    m_oversampling->decimatorR.reset();
}

void AnalogFuzz::sync()
{
    syncParameters();
}

float AnalogFuzz::saturationDb() const
{
    return static_cast<float>(m_saturationDb);
}

void AnalogFuzz::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDrive().toStdString()); p) {
        m_drive = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyFuzz().toStdString()); p) {
        m_fuzz = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBias().toStdString()); p) {
        m_bias = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) {
        m_cutoff = static_cast<float>(ParameterMapper::mapLogFrequency(p->get().value(), MinCutoffHz, MaxCutoffHz));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyResonance().toStdString()); p) {
        m_resonance = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        m_mix = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_outputDb = static_cast<float>((p->get().value() - 0.5f) * 2.0 * OutputRangeDb);
    }

    m_biasOffset = (static_cast<double>(m_bias) - 0.5) * 2.0 * BiasRange * StageHeadroom;
    m_quiescent = shape(m_biasOffset);
}

} // namespace noteahead
