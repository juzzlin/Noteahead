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

#include "drive.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../dsp/upsampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

struct Drive::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;
};

Drive::Drive()
  : m_oversampling { std::make_unique<Oversampling>() }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyMode().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDrive().toStdString(), 0.5f, 0, 100, 50, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMix().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });

    syncParameters();
}

Drive::~Drive() = default;

double Drive::shape(double x) const
{
    switch (m_mode) {
    case Mode::Hard:
        return std::clamp(x, -1.0, 1.0);
    case Mode::Fold: {
        // Triangle wavefolder: reflect the signal back whenever it exceeds the [-1, 1] range. The
        // drive gain is bounded, so only a handful of reflections are ever needed.
        double y = x;
        while (y > 1.0 || y < -1.0) {
            if (y > 1.0) {
                y = 2.0 - y;
            } else {
                y = -2.0 - y;
            }
        }
        return y;
    }
    case Mode::Dist: {
        // Asymmetric, hard-edged clipping reminiscent of an overdriven guitar amp. A small DC bias
        // before a steep tanh adds even harmonics for a richer, buzzier tone; the bias is subtracted
        // back out afterwards so the output stays centred, then clamped so it never exceeds unity.
        static const double bias = 0.15;
        return std::clamp(std::tanh((x + bias) * 2.0) - std::tanh(bias * 2.0), -1.0, 1.0);
    }
    case Mode::Soft:
    default:
        return std::tanh(x);
    }
}

float Drive::processOversampled(Upsampler & upsampler, Decimator & decimator, float sample, double driveGain, double mix, uint8_t factor)
{
    std::array<float, 4> high {};
    upsampler.process(sample, high.data(), factor);
    for (uint8_t k = 0; k < factor; k++) {
        const double dry = static_cast<double>(high[k]);
        const double wet = shape(dry * driveGain);
        // Reconstruct the dry/wet blend at the high rate so the dry path shares the resampler latency
        // and stays aligned with the shaped signal (no comb filtering at partial mix).
        high[k] = static_cast<float>(dry * (1.0 - mix) + wet * mix);
    }
    return decimator.process(high.data(), factor);
}

void Drive::process(double & left, double & right)
{
    const double driveGain = 1.0 + static_cast<double>(m_drive) * 9.0; // 1x .. 10x
    const double mix = static_cast<double>(m_mix);
    const double outputLin = static_cast<double>(Utils::Dsp::dbToLinear(m_outputDb));
    const uint8_t factor = clampOversampleFactor(oversampleFactor());

    const double dryL = left;
    const double dryR = right;

    if (factor == 1 || mix <= 0.0) {
        const double wetL = shape(dryL * driveGain);
        const double wetR = shape(dryR * driveGain);
        left = (dryL * (1.0 - mix) + wetL * mix) * outputLin;
        right = (dryR * (1.0 - mix) + wetR * mix) * outputLin;
        return;
    }

    left = static_cast<double>(processOversampled(m_oversampling->upsamplerL, m_oversampling->decimatorL, static_cast<float>(dryL), driveGain, mix, factor)) * outputLin;
    right = static_cast<double>(processOversampled(m_oversampling->upsamplerR, m_oversampling->decimatorR, static_cast<float>(dryR), driveGain, mix, factor)) * outputLin;
}

void Drive::reset()
{
    m_oversampling->upsamplerL.reset();
    m_oversampling->upsamplerR.reset();
    m_oversampling->decimatorL.reset();
    m_oversampling->decimatorR.reset();
}

void Drive::sync()
{
    syncParameters();
}

void Drive::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        const int mode = static_cast<int>(p->get().value());
        m_mode = mode == 1 ? Mode::Hard : mode == 2 ? Mode::Fold
          : mode == 3                               ? Mode::Dist
                                                    : Mode::Soft;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDrive().toStdString()); p) {
        m_drive = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        m_mix = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_outputDb = -12.0f + p->get().value() * 24.0f;
    }
}

std::string Drive::typeIdString()
{
    return "7d3e9a1c-4b2f-4c6d-8e5a-1f0b2c3d4e5f";
}

std::string Drive::type() const
{
    return Constants::RackEffectType::drive().toStdString();
}

std::string Drive::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
