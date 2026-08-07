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

#include "stereo_enhancer.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Range the Bass frequency control sweeps. The X32 labels this one 1 - 30; real hertz is what
//! every other frequency control in this application shows, so that is what it sweeps here.
constexpr double MinBassHz = 40.0;
constexpr double MaxBassHz = 400.0;

//! Range the Hi frequency control sweeps, likewise in hertz rather than the console's 1 - 30.
constexpr double MinHighHz = 1500.0;
constexpr double MaxHighHz = 16000.0;

//! Where the midrange dip sits. Fixed, because the control that would move it is spent on Q: it is
//! the width of the dip that decides how much room the two ends get.
constexpr double MidHz = 1000.0;

//! Q range of the midrange dip, mapped the way every other Q in this application is.
constexpr double MinMidQ = 0.3;
constexpr double MaxMidQ = 10.0;

//! Q of the two band taps. Broad, because these are meant to be regions rather than bands.
constexpr double BandQ = 0.7;

//! How much of each band is added back at the top of its control.
constexpr double MaxBassAmount = 0.9;
constexpr double MaxHighAmount = 0.7;

//! How deep the midrange dip goes at the top of its control, in dB.
constexpr double MaxMidDipDb = 9.0;

//! Drive into the bass saturator. What returns from it is mostly harmonics of the low end, which is
//! what lets a small speaker imply a fundamental it cannot actually reproduce.
constexpr double BassDrive = 2.5;

//! How far Spread can push the side signal.
constexpr double MaxSpread = 1.4;

//! Output trim range either side of unity, in dB.
constexpr double OutputRangeDb = 12.0;

} // namespace

StereoEnhancer::StereoEnhancer()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyBassGain().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyBassFreq().toStdString(), 0.5f, 40, 400, 120, 1, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMidGain().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMidQ().toStdString(), 0.5f, 1, 100, 10, 10, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHighGain().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHighFreq().toStdString(), 0.5f, 1500, 16000, 6000, 1, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeySpread().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMix().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous });

    // Solo passes only what this adds, which is the point of a psycho EQ: what it contributes is
    // hard to judge underneath the signal it is contributing to.
    addSoloParameter();

    syncParameters();
}

std::string StereoEnhancer::typeIdString()
{
    return "3f6a25d9-58c1-4e37-8b04-2d9f7c1e6b58";
}

std::string StereoEnhancer::type() const
{
    return Constants::RackEffectType::stereoEnhancer().toStdString();
}

std::string StereoEnhancer::typeId() const
{
    return typeIdString();
}

void StereoEnhancer::updateFilters()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    if (!m_coefficientsDirty && std::abs(sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }
    m_lastSampleRate = sampleRate;
    m_coefficientsDirty = false;

    const double bassHz = ParameterMapper::mapLogFrequency(static_cast<double>(m_bassFrequency), MinBassHz, MaxBassHz);
    const double highHz = ParameterMapper::mapLogFrequency(static_cast<double>(m_highFrequency), MinHighHz, MaxHighHz);
    const double midQ = ParameterMapper::mapExponential(static_cast<double>(m_midQ), MinMidQ, MaxMidQ);
    const double midDipDb = -static_cast<double>(m_midGain) * MaxMidDipDb;

    m_bassTapL.calculateBandPass(bassHz, sampleRate, BandQ);
    m_bassTapR.calculateBandPass(bassHz, sampleRate, BandQ);
    m_highTapL.calculateBandPass(highHz, sampleRate, BandQ);
    m_highTapR.calculateBandPass(highHz, sampleRate, BandQ);
    m_midL.calculateBell(MidHz, sampleRate, midQ, midDipDb);
    m_midR.calculateBell(MidHz, sampleRate, midQ, midDipDb);
}

void StereoEnhancer::processSample(double & left, double & right)
{
    updateFilters();

    const double dryL = left;
    const double dryR = right;

    // The midrange is shaped in place: pulling the middle back is what opens up the two ends.
    double wetL = m_midL.process(dryL);
    double wetR = m_midR.process(dryR);

    // Bass: take the band out, saturate it, add back what that produces. The saturator returns
    // harmonics of the low end rather than more of the low end itself.
    if (m_bassGain > 0.0f) {
        const double amount = static_cast<double>(m_bassGain) * MaxBassAmount;
        wetL += std::tanh(m_bassTapL.process(dryL) * BassDrive) / BassDrive * amount;
        wetR += std::tanh(m_bassTapR.process(dryR) * BassDrive) / BassDrive * amount;
    } else {
        // The taps carry state, so they have to keep running even when they are not being summed.
        m_bassTapL.process(dryL);
        m_bassTapR.process(dryR);
    }

    if (m_highGain > 0.0f) {
        const double amount = static_cast<double>(m_highGain) * MaxHighAmount;
        wetL += m_highTapL.process(dryL) * amount;
        wetR += m_highTapR.process(dryR) * amount;
    } else {
        m_highTapL.process(dryL);
        m_highTapR.process(dryR);
    }

    // Spread works on the side signal alone, so a mono source stays mono however far it is turned
    // up: there is nothing off-centre for it to widen.
    if (m_spread > 0.0f) {
        const double mid = (wetL + wetR) * 0.5;
        const double side = (wetL - wetR) * 0.5 * (1.0 + static_cast<double>(m_spread) * MaxSpread);
        wetL = mid + side;
        wetR = mid - side;
    }

    const double outputLin = static_cast<double>(Utils::Dsp::dbToLinear(m_outputDb));
    wetL *= outputLin;
    wetR *= outputLin;

    const double mix = static_cast<double>(m_mix);
    left = dryL * (1.0 - mix) + wetL * mix;
    right = dryR * (1.0 - mix) + wetR * mix;
}

void StereoEnhancer::reset()
{
    m_bassTapL.reset();
    m_bassTapR.reset();
    m_highTapL.reset();
    m_highTapR.reset();
    m_midL.reset();
    m_midR.reset();
}

void StereoEnhancer::syncParameters()
{
    const auto value = [this](const QString & key, float fallback) {
        const auto parameter = this->parameter(key.toStdString());
        return parameter ? parameter->get().value() : fallback;
    };

    m_bassGain = value(Constants::NahdXml::xmlKeyBassGain(), 0.0f);
    m_bassFrequency = value(Constants::NahdXml::xmlKeyBassFreq(), 0.5f);
    m_midGain = value(Constants::NahdXml::xmlKeyMidGain(), 0.0f);
    m_midQ = value(Constants::NahdXml::xmlKeyMidQ(), 0.5f);
    m_highGain = value(Constants::NahdXml::xmlKeyHighGain(), 0.0f);
    m_highFrequency = value(Constants::NahdXml::xmlKeyHighFreq(), 0.5f);
    m_outputDb = (value(Constants::NahdXml::xmlKeyGain(), 0.5f) - 0.5f) * 2.0f * static_cast<float>(OutputRangeDb);
    m_spread = value(Constants::NahdXml::xmlKeySpread(), 0.0f);
    m_mix = value(Constants::NahdXml::xmlKeyMix(), 1.0f);

    m_coefficientsDirty = true;
}

void StereoEnhancer::sync()
{
    syncParameters();
}

} // namespace noteahead
