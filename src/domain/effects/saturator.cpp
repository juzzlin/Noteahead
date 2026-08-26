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

#include "saturator.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../dsp/upsampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

namespace {

//! Drive range in dB at the top of the control, and the range it used to have. The old one is kept
//! so a project saved against it can be converted rather than reinterpreted.
constexpr double MaxDriveDb = 40.0;
constexpr double LegacyMaxDriveDb = 24.0;

} // namespace

struct Saturator::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;
};

Saturator::Saturator()
  : m_oversampling { std::make_unique<Oversampling>() }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDriveDb().toStdString(), 0.15f, 0, static_cast<int>(MaxDriveDb * 100), 600, 100, Parameter::Type::Continuous, { Constants::NahdXml::xmlKeyDrive().toStdString() },
                             // 24 dB was not enough to reach the knee on the -26 dBFS a device in
                             // this rack puts out: full drive measured 4.2 % THD on a real signal.
                             // A position saved against the old range is converted to the dB it
                             // used to mean, so an existing project sounds exactly as it did.
                             [](float legacyPosition) {
                                 return static_cast<float>(static_cast<double>(legacyPosition) * LegacyMaxDriveDb / MaxDriveDb);
                             } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyTone().toStdString(), 1.0f, 0, 100, 100, 100, Parameter::Type::Continuous });
    addMixParameter(1.0f, MixLaw::Internal);
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });

    m_toneFilterL.setMode(CascadedSvf::Mode::LowPass);
    m_toneFilterR.setMode(CascadedSvf::Mode::LowPass);

    syncParameters();
}

Saturator::~Saturator() = default;

double Saturator::shape(double x) const
{
    switch (m_mode) {
    case Mode::Tape:
        return std::tanh(x);
    case Mode::Tube:
        // Asymmetric curve: softer negative half emulates tube-style even harmonics
        return x >= 0.0 ? std::tanh(x) : std::tanh(x * 1.4) * 0.9;
    case Mode::Diode:
    default: {
        const double ax = std::abs(x);
        const double sign = x >= 0.0 ? 1.0 : -1.0;
        if (ax < 1.0 / 3.0) {
            return 2.0 * x;
        } else if (ax < 2.0 / 3.0) {
            const double t = 2.0 - 3.0 * ax;
            return sign * (3.0 - t * t) / 3.0;
        } else {
            return sign;
        }
    }
    }
}

void Saturator::processSample(double & left, double & right)
{
    const double baseSampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    const uint8_t factor = clampOversampleFactor(oversampleFactor());
    // The tone filter runs alongside the shaper, so it must operate at the oversampled rate. Its cutoff
    // is capped at 20 kHz, so the audible tone is unchanged across factors for normal sample rates.
    const double stageSampleRate = baseSampleRate * factor;
    m_toneFilterL.setSampleRate(stageSampleRate);
    m_toneFilterR.setSampleRate(stageSampleRate);
    m_toneFilterL.setCutoff(static_cast<double>(m_tone));
    m_toneFilterR.setCutoff(static_cast<double>(m_tone));

    const double driveLin = static_cast<double>(Utils::Dsp::dbToLinear(m_driveDb));
    const double outputLin = static_cast<double>(Utils::Dsp::dbToLinear(m_outputDb));
    const double mix = static_cast<double>(m_mix);

    const double dryL = left;
    const double dryR = right;

    double peakPre = 0.0;
    double peakPost = 0.0;
    double outL = 0.0;
    double outR = 0.0;

    if (factor == 1) {
        const double drivenL = dryL * driveLin;
        const double drivenR = dryR * driveLin;
        double wetL = shape(drivenL);
        double wetR = shape(drivenR);
        peakPre = std::max(std::abs(drivenL), std::abs(drivenR));
        peakPost = std::max(std::abs(wetL), std::abs(wetR));
        wetL = m_toneFilterL.process(wetL);
        wetR = m_toneFilterR.process(wetR);
        outL = blendWetPart(dryL, wetL, mix);
        outR = blendWetPart(dryR, wetR, mix);
    } else {
        std::array<float, 4> highL {};
        std::array<float, 4> highR {};
        m_oversampling->upsamplerL.process(static_cast<float>(dryL), highL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(dryR), highR.data(), factor);
        for (uint8_t k = 0; k < factor; k++) {
            const double dryHiL = static_cast<double>(highL[k]);
            const double dryHiR = static_cast<double>(highR[k]);
            const double drivenL = dryHiL * driveLin;
            const double drivenR = dryHiR * driveLin;
            double wetL = shape(drivenL);
            double wetR = shape(drivenR);
            peakPre = std::max(peakPre, std::max(std::abs(drivenL), std::abs(drivenR)));
            peakPost = std::max(peakPost, std::max(std::abs(wetL), std::abs(wetR)));
            wetL = m_toneFilterL.process(wetL);
            wetR = m_toneFilterR.process(wetR);
            highL[k] = static_cast<float>(blendWetPart(dryHiL, wetL, mix));
            highR[k] = static_cast<float>(blendWetPart(dryHiR, wetR, mix));
        }
        outL = static_cast<double>(m_oversampling->decimatorL.process(highL.data(), factor));
        outR = static_cast<double>(m_oversampling->decimatorR.process(highR.data(), factor));
    }

    double saturationDb = 0.0;
    if (peakPre > 1e-10 && peakPost < peakPre) {
        saturationDb = Utils::Dsp::linearToDb(static_cast<float>(peakPost / peakPre));
    }

    const double meterReleaseCoeff = std::exp(-1.0 / (100.0 * baseSampleRate / 1000.0));
    if (saturationDb < m_saturationDb) {
        m_saturationDb = saturationDb;
    } else {
        m_saturationDb = meterReleaseCoeff * m_saturationDb + (1.0 - meterReleaseCoeff) * saturationDb;
    }

    // Denormal protection
    if (std::abs(m_saturationDb) < 1.0e-15) {
        m_saturationDb = 0.0;
    }

    left = completeBlend(dryL, outL, outputLin);
    right = completeBlend(dryR, outR, outputLin);
}

void Saturator::reset()
{
    m_saturationDb = 0.0;
    m_toneFilterL.reset();
    m_toneFilterR.reset();
    m_oversampling->upsamplerL.reset();
    m_oversampling->upsamplerR.reset();
    m_oversampling->decimatorL.reset();
    m_oversampling->decimatorR.reset();
}

void Saturator::sync()
{
    syncParameters();
}

float Saturator::saturationDb() const
{
    return static_cast<float>(m_saturationDb);
}

void Saturator::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        const int mode = static_cast<int>(p->get().value());
        m_mode = mode == 1 ? Mode::Tube : mode == 2 ? Mode::Diode
                                                    : Mode::Tape;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString()); p) {
        m_driveDb = static_cast<float>(p->get().value() * MaxDriveDb);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyTone().toStdString()); p) {
        m_tone = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        m_mix = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_outputDb = -12.0f + p->get().value() * 24.0f;
    }
}

std::string Saturator::typeIdString()
{
    return "2f4a6c8e-1b3d-4f5a-9c7e-0d2b4e6f8a1c";
}

std::string Saturator::type() const
{
    return Constants::RackEffectType::saturator().toStdString();
}

std::string Saturator::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
