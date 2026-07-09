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

#include "saturator_effect.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

SaturatorEffect::SaturatorEffect()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDrive().toStdString(), 0.25f, 0, 2400, 600, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyTone().toStdString(), 1.0f, 0, 100, 100, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMix().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });

    m_toneFilterL.setMode(CascadedSvf::Mode::LowPass);
    m_toneFilterR.setMode(CascadedSvf::Mode::LowPass);

    syncParameters();
}

double SaturatorEffect::shape(double x) const
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

void SaturatorEffect::process(double & left, double & right)
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    m_toneFilterL.setSampleRate(sampleRate);
    m_toneFilterR.setSampleRate(sampleRate);
    m_toneFilterL.setCutoff(static_cast<double>(m_tone));
    m_toneFilterR.setCutoff(static_cast<double>(m_tone));

    const double driveLin = static_cast<double>(Utils::Dsp::dbToLinear(m_driveDb));
    const double outputLin = static_cast<double>(Utils::Dsp::dbToLinear(m_outputDb));

    const double dryL = left;
    const double dryR = right;

    const double drivenL = dryL * driveLin;
    const double drivenR = dryR * driveLin;

    double wetL = shape(drivenL);
    double wetR = shape(drivenR);

    const double peakPre = std::max(std::abs(drivenL), std::abs(drivenR));
    const double peakPost = std::max(std::abs(wetL), std::abs(wetR));

    double saturationDb = 0.0;
    if (peakPre > 1e-10 && peakPost < peakPre) {
        saturationDb = Utils::Dsp::linearToDb(static_cast<float>(peakPost / peakPre));
    }

    const double meterReleaseCoeff = std::exp(-1.0 / (100.0 * sampleRate / 1000.0));
    if (saturationDb < m_saturationDb) {
        m_saturationDb = saturationDb;
    } else {
        m_saturationDb = meterReleaseCoeff * m_saturationDb + (1.0 - meterReleaseCoeff) * saturationDb;
    }

    // Denormal protection
    if (std::abs(m_saturationDb) < 1.0e-15) {
        m_saturationDb = 0.0;
    }

    wetL = m_toneFilterL.process(wetL);
    wetR = m_toneFilterR.process(wetR);

    const double mix = static_cast<double>(m_mix);
    left = (dryL * (1.0 - mix) + wetL * mix) * outputLin;
    right = (dryR * (1.0 - mix) + wetR * mix) * outputLin;
}

void SaturatorEffect::reset()
{
    m_saturationDb = 0.0;
    m_toneFilterL.reset();
    m_toneFilterR.reset();
}

void SaturatorEffect::sync()
{
    syncParameters();
}

float SaturatorEffect::saturationDb() const
{
    return static_cast<float>(m_saturationDb);
}

void SaturatorEffect::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        const int mode = static_cast<int>(p->get().value());
        m_mode = mode == 1 ? Mode::Tube : mode == 2 ? Mode::Diode
                                                    : Mode::Tape;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDrive().toStdString()); p) {
        m_driveDb = p->get().value() * 24.0f;
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

std::string SaturatorEffect::typeIdString()
{
    return "2f4a6c8e-1b3d-4f5a-9c7e-0d2b4e6f8a1c";
}

std::string SaturatorEffect::type() const
{
    return Constants::RackEffectType::saturator().toStdString();
}

std::string SaturatorEffect::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
