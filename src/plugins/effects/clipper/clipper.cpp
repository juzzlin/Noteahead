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
#include "../../../common/constants.hpp"
#include "../../../common/utils.hpp"
#include "../../../domain/dsp/audio_context.hpp"
#include "../../../domain/dsp/upsampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace noteahead {

struct Clipper::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;
    std::vector<float> inL;
    std::vector<float> inR;
    std::vector<float> highL;
    std::vector<float> highR;
    std::vector<float> outL;
    std::vector<float> outR;

    void ensureCapacity(size_t frames, uint8_t factor)
    {
        if (inL.size() < frames) {
            inL.resize(frames);
            inR.resize(frames);
            outL.resize(frames);
            outR.resize(frames);
        }
        const size_t highCount = frames * factor;
        if (highL.size() < highCount) {
            highL.resize(highCount);
            highR.resize(highCount);
        }
    }
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

float Clipper::clipSample(float sample, float thresholdLin, float invThreshold) const
{
    if (m_mode == Mode::Hard) {
        return std::clamp(sample, -thresholdLin, thresholdLin);
    }
    return thresholdLin * std::tanh(sample * invThreshold);
}

void Clipper::updateMeter(double peakPre, double peakPost)
{
    double reductionDb = 0.0;
    if (peakPre > 1e-10 && peakPost < peakPre) {
        reductionDb = Utils::Dsp::linearToDb(static_cast<float>(peakPost / peakPre));
    }

    if (reductionDb == 0.0 && m_reductionDb == 0.0) {
        return;
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
}

void Clipper::processSample(double & left, double & right)
{
    const float thresholdLin = static_cast<float>(m_threshold);
    const float invThreshold = m_invThreshold;
    const auto gainLin = m_gain;
    const uint8_t factor = clampOversampleFactor(oversampleFactor());

    // Measured around the clipping itself, never across the whole block: the half-band filters are
    // linear-phase, so the upsample/decimate round trip delays the output by tens of base-rate
    // samples. Dividing the current input by an output that old pins the meter near full scale on
    // any periodic signal, whether or not a single sample was actually clipped.
    double peakPre = 0.0;
    double peakPost = 0.0;

    if (factor == 1) {
        peakPre = std::max(std::abs(left), std::abs(right));
        left = clipSample(static_cast<float>(left), thresholdLin, invThreshold);
        right = clipSample(static_cast<float>(right), thresholdLin, invThreshold);
        peakPost = std::max(std::abs(left), std::abs(right));
    } else {
        // Clip at the oversampled rate so the harmonics generated fold above Nyquist and are removed
        // by the decimation filter instead of aliasing back into the audible band.
        std::array<float, 4> highL {};
        std::array<float, 4> highR {};
        m_oversampling->upsamplerL.process(static_cast<float>(left), highL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(right), highR.data(), factor);
        for (uint8_t k = 0; k < factor; k++) {
            peakPre = std::max(peakPre, std::max(std::abs(static_cast<double>(highL[k])), std::abs(static_cast<double>(highR[k]))));
            highL[k] = clipSample(highL[k], thresholdLin, invThreshold);
            highR[k] = clipSample(highR[k], thresholdLin, invThreshold);
            peakPost = std::max(peakPost, std::max(std::abs(static_cast<double>(highL[k])), std::abs(static_cast<double>(highR[k]))));
        }
        left = static_cast<double>(m_oversampling->decimatorL.process(highL.data(), factor));
        right = static_cast<double>(m_oversampling->decimatorR.process(highR.data(), factor));
    }

    updateMeter(peakPre, peakPost);

    left *= gainLin;
    right *= gainLin;
}

void Clipper::processBlock(AudioContext & context)
{
    setOversampleFactor(context.oversampleFactor);
    if (static_cast<uint32_t>(context.sampleRate) != m_lastSampleRate) {
        if (context.sampleRate > 0) {
            m_meterReleaseCoeff = std::exp(-1.0 / (100.0 * context.sampleRate / 1000.0));
        }
        m_lastSampleRate = static_cast<uint32_t>(context.sampleRate);
    }

    const uint8_t factor = clampOversampleFactor(oversampleFactor());
    const float thresholdLin = static_cast<float>(m_threshold);
    const float invThreshold = m_invThreshold;
    const double gainLin = m_gain;

    if (factor == 1) {
        for (uint32_t i = 0; i < context.frameCount; i++) {
            double left = context.buffer[i * 2];
            double right = context.buffer[i * 2 + 1];
            const double peakPre = std::max(std::abs(left), std::abs(right));
            left = clipSample(static_cast<float>(left), thresholdLin, invThreshold);
            right = clipSample(static_cast<float>(right), thresholdLin, invThreshold);
            const double peakPost = std::max(std::abs(left), std::abs(right));
            updateMeter(peakPre, peakPost);
            context.buffer[i * 2] = left * gainLin;
            context.buffer[i * 2 + 1] = right * gainLin;
        }
        return;
    }

    const size_t frames = context.frameCount;
    m_oversampling->ensureCapacity(frames, factor);
    float * const inL = m_oversampling->inL.data();
    float * const inR = m_oversampling->inR.data();
    float * const highL = m_oversampling->highL.data();
    float * const highR = m_oversampling->highR.data();
    float * const outL = m_oversampling->outL.data();
    float * const outR = m_oversampling->outR.data();

    for (size_t i = 0; i < frames; i++) {
        inL[i] = static_cast<float>(context.buffer[i * 2]);
        inR[i] = static_cast<float>(context.buffer[i * 2 + 1]);
    }

    m_oversampling->upsamplerL.processBlock(inL, highL, frames, factor);
    m_oversampling->upsamplerR.processBlock(inR, highR, frames, factor);

    for (size_t i = 0; i < frames; i++) {
        double peakPre = 0.0;
        double peakPost = 0.0;
        const size_t base = i * factor;
        for (uint8_t k = 0; k < factor; k++) {
            const float hl = highL[base + k];
            const float hr = highR[base + k];
            peakPre = std::max(peakPre, std::max(std::abs(static_cast<double>(hl)), std::abs(static_cast<double>(hr))));
            const float cl = clipSample(hl, thresholdLin, invThreshold);
            const float cr = clipSample(hr, thresholdLin, invThreshold);
            highL[base + k] = cl;
            highR[base + k] = cr;
            peakPost = std::max(peakPost, std::max(std::abs(static_cast<double>(cl)), std::abs(static_cast<double>(cr))));
        }
        updateMeter(peakPre, peakPost);
    }

    m_oversampling->decimatorL.processBlock(highL, outL, frames, factor);
    m_oversampling->decimatorR.processBlock(highR, outR, frames, factor);

    for (size_t i = 0; i < frames; i++) {
        context.buffer[i * 2] = static_cast<double>(outL[i]) * gainLin;
        context.buffer[i * 2 + 1] = static_cast<double>(outR[i]) * gainLin;
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
    // The gains follow the parameters alone, so they are worked out here rather than once a sample
    m_threshold = std::max(1e-5, static_cast<double>(Utils::Dsp::dbToLinear(m_thresholdDb)));
    m_invThreshold = static_cast<float>(1.0 / m_threshold);
    m_gain = static_cast<double>(Utils::Dsp::dbToLinear(m_gainDb));
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

std::vector<std::string> Clipper::modulationPriority() const
{
    namespace NahdXml = Constants::NahdXml;
    return {
        NahdXml::xmlKeyThreshold().toStdString(),
        NahdXml::xmlKeyGain().toStdString()
    };
}

} // namespace noteahead
