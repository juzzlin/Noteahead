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

#include "clipper.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../dsp/audio_context.hpp"
#include "../dsp/upsampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

struct Clipper::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;
};

Clipper::Clipper()
  : m_oversampling { std::make_unique<Oversampling>() }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyMode().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Discrete, { "clipperMode" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyThreshold().toStdString(), 1.0f, -2400, 0, 0, 100, Parameter::Type::Continuous, { "clipperThreshold" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -2400, 2400, 0, 100, Parameter::Type::Continuous, { "clipperGain" } });

    syncParameters();
}

Clipper::~Clipper() = default;

float Clipper::clipSample(float sample, double thresholdLin) const
{
    const auto t = static_cast<float>(thresholdLin);
    if (m_mode == Mode::Hard) {
        return std::clamp(sample, -t, t);
    }
    return t * std::tanh(sample / t);
}

void Clipper::process(double & left, double & right)
{
    const auto thresholdLin = std::max(1e-5, static_cast<double>(Utils::Dsp::dbToLinear(m_thresholdDb)));
    const auto gainLin = static_cast<double>(Utils::Dsp::dbToLinear(m_gainDb));
    const uint8_t factor = clampOversampleFactor(oversampleFactor());

    const double preL = left;
    const double preR = right;

    if (factor == 1) {
        left = clipSample(static_cast<float>(left), thresholdLin);
        right = clipSample(static_cast<float>(right), thresholdLin);
    } else {
        // Clip at the oversampled rate so the harmonics generated fold above Nyquist and are removed
        // by the decimation filter instead of aliasing back into the audible band.
        std::array<float, 4> highL {};
        std::array<float, 4> highR {};
        m_oversampling->upsamplerL.process(static_cast<float>(left), highL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(right), highR.data(), factor);
        for (uint8_t k = 0; k < factor; k++) {
            highL[k] = clipSample(highL[k], thresholdLin);
            highR[k] = clipSample(highR[k], thresholdLin);
        }
        left = static_cast<double>(m_oversampling->decimatorL.process(highL.data(), factor));
        right = static_cast<double>(m_oversampling->decimatorR.process(highR.data(), factor));
    }

    const double peakPre = std::max(std::abs(preL), std::abs(preR));
    const double peakPost = std::max(std::abs(left), std::abs(right));

    double reductionDb = 0.0;
    if (peakPre > 1e-10 && peakPost < peakPre) {
        reductionDb = Utils::Dsp::linearToDb(static_cast<float>(peakPost / peakPre));
    }

    if (reductionDb < m_reductionDb) {
        m_reductionDb = reductionDb;
    } else {
        m_reductionDb = m_meterReleaseCoeff * m_reductionDb + (1.0 - m_meterReleaseCoeff) * reductionDb;
    }

    // Denormal protection
    if (std::abs(m_reductionDb) < 1.0e-15) {
        m_reductionDb = 0.0;
    }

    left *= gainLin;
    right *= gainLin;
}

void Clipper::process(AudioContext & context)
{
    setOversampleFactor(context.oversampleFactor);
    if (static_cast<uint32_t>(context.sampleRate) != m_lastSampleRate) {
        m_meterReleaseCoeff = std::exp(-1.0 / (100.0 * context.sampleRate / 1000.0));
        m_lastSampleRate = static_cast<uint32_t>(context.sampleRate);
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        process(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Clipper::reset()
{
    m_reductionDb = 0.0;
    m_oversampling->upsamplerL.reset();
    m_oversampling->upsamplerR.reset();
    m_oversampling->decimatorL.reset();
    m_oversampling->decimatorR.reset();
}

void Clipper::sync()
{
    syncParameters();
}

float Clipper::reductionDb() const
{
    return static_cast<float>(m_reductionDb);
}

void Clipper::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        m_mode = static_cast<int>(p->get().value()) == 0 ? Mode::Hard : Mode::Soft;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        m_thresholdDb = -24.0f + p->get().value() * 24.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_gainDb = -24.0f + p->get().value() * 48.0f;
    }
}

std::string Clipper::typeIdString()
{
    return "9e1f2a3b-4c5d-6e7f-8a9b-0c1d2e3f4a5b";
}

std::string Clipper::type() const
{
    return Constants::RackEffectType::clipper().toStdString();
}

std::string Clipper::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
