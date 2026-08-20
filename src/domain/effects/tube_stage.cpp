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

#include "tube_stage.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../dsp/upsampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

namespace {

//! Drive range in dB at the top of the control, and the range it used to have. A device in this
//! rack puts out around -26 dBFS, and against that the old range needed three quarters of the
//! travel before the valve did anything. The old range is kept so a project saved against it can be
//! converted rather than reinterpreted.
constexpr double MaxDriveDb = 48.0;
constexpr double LegacyMaxDriveDb = 36.0;

//! Output trim range either side of unity, in dB.
constexpr double OutputRangeDb = 12.0;

//! How far the bias control can push the grid either side of the centre point. Enough to reach
//! visible asymmetry without parking the stage so far into cutoff that it only gates.
constexpr double BiasRange = 0.7;

//! Ratio between the two halves of the triode curve. The cutoff side saturates sooner and at a lower
//! level than the grid side, which is where the even harmonics come from.
constexpr double TriodeAsymmetry = 1.8;

//! Knee order of the pentode curve. Higher holds linear longer and then gives way faster than the
//! triode's tanh.
constexpr double PentodeKnee = 3.0;

//! Corner the tilt control pivots around, roughly where a valve channel's character sits.
constexpr double TiltPivotHz = 700.0;

//! Boost and cut the tilt reaches at either extreme.
constexpr double TiltRange = 0.6;

//! Coupling capacitor after the stage. Also removes the DC the asymmetric curve introduces.
constexpr double MeterReleaseMs = 100.0;

} // namespace

struct TubeStage::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;
};

TubeStage::TubeStage()
  : m_oversampling { std::make_unique<Oversampling>() }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDriveDb().toStdString(), 0.1875f, 0, static_cast<int>(MaxDriveDb * 100), 900, 100, Parameter::Type::Continuous, { Constants::NahdXml::xmlKeyDrive().toStdString() },
                             [](float legacyPosition) {
                                 return static_cast<float>(static_cast<double>(legacyPosition) * LegacyMaxDriveDb / MaxDriveDb);
                             } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyBias().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyTone().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous });
    addMixParameter(1.0f, MixLaw::Internal);
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });

    syncParameters();
}

TubeStage::~TubeStage() = default;

std::string TubeStage::typeIdString()
{
    return "e4d9b3c2-7a15-4f68-9b02-5c8e1d6a3f47";
}

std::string TubeStage::type() const
{
    return Constants::RackEffectType::tubeStage().toStdString();
}

std::string TubeStage::typeId() const
{
    return typeIdString();
}

double TubeStage::shape(double v) const
{
    if (m_mode == Mode::Triode) {
        // Grid side compresses gently; the cutoff side saturates sooner and lower, so the curve is
        // not odd-symmetric and the stage generates even harmonics.
        return v >= 0.0 ? std::tanh(v) : std::tanh(v * TriodeAsymmetry) / TriodeAsymmetry;
    }

    // Pentode: same ceiling either side, but a knee that stays linear longer before it gives way.
    const double magnitude = std::abs(v);
    const double sign = v >= 0.0 ? 1.0 : -1.0;
    return sign * magnitude / std::pow(1.0 + std::pow(magnitude, PentodeKnee), 1.0 / PentodeKnee);
}

void TubeStage::processSample(double & left, double & right)
{
    const double baseSampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    const uint8_t factor = clampOversampleFactor(oversampleFactor());

    const double driveLin = static_cast<double>(Utils::Dsp::dbToLinear(m_driveDb));
    const double outputLin = static_cast<double>(Utils::Dsp::dbToLinear(m_outputDb));
    const double mix = static_cast<double>(m_mix);

    const double dryL = left;
    const double dryR = right;

    double peakPre = 0.0;
    double peakPost = 0.0;
    double wetL = 0.0;
    double wetR = 0.0;

    const auto valve = [&](double sample) {
        const double driven = sample * driveLin;
        peakPre = std::max(peakPre, std::abs(driven));
        const double plate = shape(driven + m_biasOffset) - m_quiescent;
        peakPost = std::max(peakPost, std::abs(plate));
        return plate;
    };

    if (factor == 1) {
        wetL = valve(dryL);
        wetR = valve(dryR);
    } else {
        // Shape at the oversampled rate so the harmonics the valve generates fold above Nyquist and
        // are removed by the decimation filter rather than aliasing back into the audible band.
        std::array<float, 4> highL {};
        std::array<float, 4> highR {};
        m_oversampling->upsamplerL.process(static_cast<float>(dryL), highL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(dryR), highR.data(), factor);
        for (uint8_t k = 0; k < factor; k++) {
            highL[k] = static_cast<float>(valve(static_cast<double>(highL[k])));
            highR[k] = static_cast<float>(valve(static_cast<double>(highR[k])));
        }
        wetL = static_cast<double>(m_oversampling->decimatorL.process(highL.data(), factor));
        wetR = static_cast<double>(m_oversampling->decimatorR.process(highR.data(), factor));
    }

    // The bias point leaves a DC offset on the output that no downstream effect should have to deal
    // with. Blocking it at the base rate is enough, since DC survives decimation unchanged.
    m_dcBlockerL.setSampleRate(baseSampleRate);
    m_dcBlockerR.setSampleRate(baseSampleRate);
    wetL = m_dcBlockerL.process(wetL);
    wetR = m_dcBlockerR.process(wetR);

    // Tilt: one filter call yields both taps, so the two halves are always complementary and the
    // centre position is exactly flat.
    const double tiltAmount = (static_cast<double>(m_tone) - 0.5) * 2.0 * TiltRange;
    const double lowGain = 1.0 - tiltAmount;
    const double highGain = 1.0 + tiltAmount;
    m_tiltL.calculate(TiltPivotHz, baseSampleRate);
    m_tiltR.calculate(TiltPivotHz, baseSampleRate);
    m_tiltL.process(wetL);
    m_tiltR.process(wetR);
    wetL = m_tiltL.lowPass() * lowGain + m_tiltL.highPass() * highGain;
    wetR = m_tiltR.lowPass() * lowGain + m_tiltR.highPass() * highGain;

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

void TubeStage::reset()
{
    m_saturationDb = 0.0;
    m_tiltL.reset();
    m_tiltR.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
    m_oversampling->upsamplerL.reset();
    m_oversampling->upsamplerR.reset();
    m_oversampling->decimatorL.reset();
    m_oversampling->decimatorR.reset();
}

void TubeStage::sync()
{
    syncParameters();
}

float TubeStage::saturationDb() const
{
    return static_cast<float>(m_saturationDb);
}

void TubeStage::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        m_mode = static_cast<int>(p->get().value()) == 1 ? Mode::Pentode : Mode::Triode;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString()); p) {
        m_driveDb = static_cast<float>(p->get().value() * MaxDriveDb);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBias().toStdString()); p) {
        m_bias = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyTone().toStdString()); p) {
        m_tone = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        m_mix = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_outputDb = static_cast<float>((p->get().value() - 0.5f) * 2.0 * OutputRangeDb);
    }

    m_biasOffset = (static_cast<double>(m_bias) - 0.5) * 2.0 * BiasRange;
    m_quiescent = shape(m_biasOffset);
}

} // namespace noteahead
