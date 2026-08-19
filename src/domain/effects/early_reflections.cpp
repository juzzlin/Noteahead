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

#include "early_reflections.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! How far the reflection pattern is spread, from a small room to a hall. The top of the range is
//! where early reflections stop being early: past it they belong to a reverb's tail.
constexpr double minSpanMs = 20.0;
constexpr double maxSpanMs = 140.0;

//! How long the first reflection can be held off. Pre-delay is what separates a source from the
//! room it is in: without it the two arrive together and the source sounds glued to the wall.
constexpr double maxPreDelayMs = 100.0;

//! What absorption takes off the top, from an untreated room to a heavily furnished one.
constexpr double minDampingHz = 1200.0;
constexpr double maxDampingHz = 18000.0;

//! Range the Low Cut sweeps.
constexpr double minLowCutHz = 20.0;
constexpr double maxLowCutHz = 500.0;
constexpr double lowCutQ = 0.707;

//! Widest the reflection pattern can be pushed.
constexpr double maxWidth = 2.0;

//! Where each reflection arrives, as a fraction of the span, and how loud it comes back. The two
//! channels differ throughout: that difference is the stereo image, and it is why no width control
//! is needed to create one.
//!
//! The gaps shrink as the pattern runs, which is what a real early field does -- reflections arrive
//! sparsely at first and pile up as they accumulate bounces -- and they vary by nearly ten to one
//! with no two alike. Even spacing would be a comb filter, and a comb filter across the whole
//! spectrum is the metallic ring these tables were rewritten to remove.
constexpr std::array<double, EarlyReflections::NumTaps> tapTimesLeft {
    0.1242, 0.2671, 0.3507, 0.4416, 0.5464, 0.6082, 0.6758, 0.7541,
    0.8019, 0.8552, 0.8791, 0.9008, 0.9205, 0.9384, 0.9548, 0.9700
};
constexpr std::array<double, EarlyReflections::NumTaps> tapGainsLeft {
    0.6248, 0.4344, 0.3683, 0.3170, 0.2007, 0.1846, 0.1235, 0.1023,
    0.0946, 0.0886, 0.0661, 0.0632, 0.0615, 0.0612, 0.0626, 0.0665
};
constexpr std::array<double, EarlyReflections::NumTaps> tapTimesRight {
    0.1403, 0.2220, 0.3109, 0.4132, 0.4728, 0.5376, 0.6121, 0.6556,
    0.7028, 0.7572, 0.7889, 0.8233, 0.8629, 0.8860, 0.9111, 0.9400
};
constexpr std::array<double, EarlyReflections::NumTaps> tapGainsRight {
    0.6048, 0.5060, 0.4269, 0.2519, 0.2143, 0.1807, 0.1497, 0.1378,
    0.1285, 0.1211, 0.0908, 0.0864, 0.0832, 0.0870, 0.0692, 0.0698
};

//! All-pass lengths, in samples at 44.1 kHz, scaled with the sample rate. Mutually prime and unlike
//! the reverbs' own, so a room and a tail placed on the same signal do not scatter it identically.
constexpr std::array<double, EarlyReflections::NumDiffusers> diffuserLengths { 113.0, 181.0, 293.0, 421.0 };

//! How far the all-passes are pushed at the top of the Diffusion control. Past this they start to
//! ring in their own right rather than scattering what is fed to them.
constexpr double maxDiffusionCoefficient = 0.75;

//! Head-room either side of the longest tap, so a sample rate change cannot read off the end.
constexpr double bufferMarginMs = 20.0;

} // namespace

EarlyReflections::EarlyReflections()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeySize().toStdString(), 0.4f, 0, 10000, 4000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPreDelay().toStdString(), 0.12f, 0, 100, 12, 1, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDamping().toStdString(), 0.4f, 0, 10000, 4000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyWidth().toStdString(), 0.5f, 0, 200, 100, 1, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), static_cast<float>(ParameterMapper::unmapLogFrequency(150.0, minLowCutHz, maxLowCutHz)), 20, 500, 150, 1, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDiffusion().toStdString(), 0.7f, 0, 10000, 7000, 100, Parameter::Type::Continuous });

    // The dry has to stay whole: reflections are something a room adds to a sound, not something it
    // replaces the sound with.
    addMixParameter(0.0f, MixLaw::Additive, 0, 10000, 100);

    // What a room adds is quiet by design and hard to judge underneath the source that caused it.
    addSoloParameter();

    syncParameters();
}

std::string EarlyReflections::typeIdString()
{
    return "6d1590ab-62e9-4141-aa19-a4015c038046";
}

std::string EarlyReflections::type() const
{
    return Constants::RackEffectType::earlyReflections().toStdString();
}

std::string EarlyReflections::typeId() const
{
    return typeIdString();
}

double EarlyReflections::Allpass::process(double input)
{
    const double buffered = buffer[writePos];
    const double output = -input + buffered;
    buffer[writePos] = input + buffered * coeff;
    writePos = (writePos + 1) % size;
    return output;
}

void EarlyReflections::Allpass::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0);
    writePos = 0;
}

double EarlyReflections::diffuse(Channel & channel, double input) const
{
    if (m_diffusion <= 0.0f) {
        return input;
    }

    double output = input;
    for (auto && allpass : channel.diffusers) {
        if (allpass.size > 0) {
            output = allpass.process(output);
        }
    }
    return output;
}

void EarlyReflections::Channel::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0);
    writePos = 0;
    for (auto && filter : damping) {
        filter.reset();
    }
    lowCut.reset();
    for (auto && allpass : diffusers) {
        allpass.reset();
    }
}

void EarlyReflections::updateDamping(Channel & channel, const std::array<double, NumTaps> & times,
                                     double dampedHz, double sampleRate)
{
    // The first reflection keeps almost all of its top and the last is damped to the corner the
    // control asks for, with everything between them on the curve joining the two. At no damping
    // the two ends are the same frequency, so nothing is filtered at all.
    for (size_t i = 0; i < NumTaps; i++) {
        const double corner = maxDampingHz * std::pow(dampedHz / maxDampingHz, times[i]);
        channel.damping[i].calculate(corner, sampleRate);
    }
}

void EarlyReflections::updateBuffers()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    const size_t samples = static_cast<size_t>((maxPreDelayMs + maxSpanMs + bufferMarginMs) * 0.001 * sampleRate) + 1;

    if (m_left.buffer.size() != samples) {
        m_left.buffer.assign(samples, 0.0);
        m_right.buffer.assign(samples, 0.0);
        m_left.writePos = 0;
        m_right.writePos = 0;
    }

    // The two channels are given lengths a little apart, so the scattering itself carries some of
    // the stereo difference rather than leaving it all to the tap pattern.
    const double rateScale = sampleRate / 44100.0;
    for (size_t i = 0; i < NumDiffusers; i++) {
        const auto resize = [&](Allpass & allpass, double stretch) {
            const uint32_t wanted = std::max(1u, static_cast<uint32_t>(diffuserLengths[i] * rateScale * stretch));
            if (wanted != allpass.size) {
                allpass.buffer.assign(wanted, 0.0);
                allpass.size = wanted;
                allpass.writePos = 0;
            }
        };
        resize(m_left.diffusers[i], 1.0);
        resize(m_right.diffusers[i], 1.13);
    }
}

void EarlyReflections::updateState()
{
    if (m_shouldSyncParameters || std::abs(m_sampleRate - m_lastSampleRate) >= 0.1) {
        updateBuffers();
        syncParameters();
        m_lastSampleRate = m_sampleRate;
        m_shouldSyncParameters = false;
    }
}

void EarlyReflections::syncParameters()
{
    const auto value = [this](const QString & key, float fallback) {
        const auto parameter = this->parameter(key.toStdString());
        return parameter ? parameter->get().value() : fallback;
    };

    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;

    m_size = value(Constants::NahdXml::xmlKeySize(), 0.4f);
    m_preDelayMs = value(Constants::NahdXml::xmlKeyPreDelay(), 0.12f) * static_cast<float>(maxPreDelayMs);
    m_damping = value(Constants::NahdXml::xmlKeyDamping(), 0.4f);
    m_width = value(Constants::NahdXml::xmlKeyWidth(), 0.5f) * static_cast<float>(maxWidth);
    m_lowCutHz = static_cast<float>(ParameterMapper::mapLogFrequency(value(Constants::NahdXml::xmlKeyHpfCutoff(), 0.5f), minLowCutHz, maxLowCutHz));
    m_diffusion = value(Constants::NahdXml::xmlKeyDiffusion(), 0.7f);

    for (size_t i = 0; i < NumDiffusers; i++) {
        m_left.diffusers[i].coeff = static_cast<double>(m_diffusion) * maxDiffusionCoefficient;
        m_right.diffusers[i].coeff = static_cast<double>(m_diffusion) * maxDiffusionCoefficient;
    }

    const double spanMs = minSpanMs + static_cast<double>(m_size) * (maxSpanMs - minSpanMs);
    const double preDelaySamples = static_cast<double>(m_preDelayMs) * 0.001 * sampleRate;
    const double spanSamples = spanMs * 0.001 * sampleRate;

    const size_t capacity = m_left.buffer.empty() ? 1 : m_left.buffer.size() - 1;
    const auto toSamples = [&](double normalizedTime) {
        const double samples = preDelaySamples + normalizedTime * spanSamples;
        // At least one, so that the shortest tap is still a tap and not the sample being written.
        return static_cast<uint32_t>(std::clamp(samples, 1.0, static_cast<double>(capacity)));
    };

    double power = 0.0;
    for (size_t i = 0; i < NumTaps; i++) {
        m_tapSamplesLeft[i] = toSamples(tapTimesLeft[i]);
        m_tapSamplesRight[i] = toSamples(tapTimesRight[i]);
        power += tapGainsLeft[i] * tapGainsLeft[i];
    }

    // The taps land at unrelated times, so what they sum to grows as the root of their combined
    // power rather than as their total gain.
    m_tapNormalization = power > 0.0 ? 1.0 / std::sqrt(power) : 1.0;

    const double dampedHz = ParameterMapper::mapLogFrequency(1.0 - static_cast<double>(m_damping), minDampingHz, maxDampingHz);
    updateDamping(m_left, tapTimesLeft, dampedHz, sampleRate);
    updateDamping(m_right, tapTimesRight, dampedHz, sampleRate);

    m_left.lowCut.calculateLowCut(m_lowCutHz, sampleRate, lowCutQ);
    m_right.lowCut.calculateLowCut(m_lowCutHz, sampleRate, lowCutQ);
}

double EarlyReflections::renderTaps(Channel & channel, const std::array<uint32_t, NumTaps> & taps,
                                    const std::array<double, NumTaps> & gains, double input) const
{
    if (channel.buffer.empty()) {
        return 0.0;
    }

    const uint32_t size = static_cast<uint32_t>(channel.buffer.size());
    channel.buffer[channel.writePos] = input;

    double sum = 0.0;
    for (size_t i = 0; i < NumTaps; i++) {
        const uint32_t offset = std::min(taps[i], size - 1);
        const uint32_t position = (channel.writePos + size - offset) % size;
        channel.damping[i].process(channel.buffer[position]);
        sum += channel.damping[i].lowPass() * gains[i];
    }

    channel.writePos = (channel.writePos + 1) % size;

    return sum * m_tapNormalization;
}

void EarlyReflections::processSample(double & left, double & right)
{
    updateState();

    double wetLeft = diffuse(m_left, renderTaps(m_left, m_tapSamplesLeft, tapGainsLeft, left));
    double wetRight = diffuse(m_right, renderTaps(m_right, m_tapSamplesRight, tapGainsRight, right));

    wetLeft = m_left.lowCut.process(wetLeft);
    wetRight = m_right.lowCut.process(wetRight);

    const double mid = (wetLeft + wetRight) * 0.5;
    const double side = (wetLeft - wetRight) * 0.5 * static_cast<double>(m_width);

    // Wet only: the Mix control's additive law is what puts the dry back, whole.
    left = mid + side;
    right = mid - side;
}

void EarlyReflections::reset()
{
    m_left.reset();
    m_right.reset();
}

void EarlyReflections::sync()
{
    m_shouldSyncParameters = true;
}

} // namespace noteahead
