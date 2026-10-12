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

#include "ambience.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double twoPi { 2.0 * std::numbers::pi };

constexpr double minDecaySeconds { 0.2 };
constexpr double maxDecaySeconds { 3.0 };
constexpr double maxPreDelayMs { 100.0 };

//! How far the early field is spread, from a vocal booth to a large live room.
constexpr double minSpanMs { 6.0 };
constexpr double maxSpanMs { 80.0 };

//! The late lines at the top of Size, in milliseconds. No two share a factor that would line their
//! echoes up, and Size scales all of them together, down to a quarter at its bottom.
constexpr std::array<double, Ambience::NumLines> lineLengthsMs { 31.7, 37.9, 43.1, 47.3, 53.9, 61.1, 67.7, 76.3 };
constexpr double minLineScale { 0.25 };
//! Which way each line takes its input, so the eight do not start out as one signal.
constexpr std::array<double, Ambience::NumLines> lineInputSigns { 1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0 };

//! The input all-passes, in milliseconds. The right channel's are stretched a little, so the
//! smearing itself already differs between the channels.
constexpr std::array<double, Ambience::NumDiffusers> diffuserLengthsMs { 1.31, 2.27, 3.53, 5.17 };
constexpr double rightDiffuserStretch { 1.093 };
//! Past this the all-passes start to ring in their own right.
constexpr double maxDiffusionCoefficient { 0.75 };

//! What Damping sweeps, as the corner of the low-pass in every line.
constexpr double minDampingHz { 1500.0 };
constexpr double maxDampingHz { 20000.0 };

constexpr double minModRateHz { 0.1 };
constexpr double maxModRateHz { 2.0 };
//! The furthest a line drifts either way. A few cents of wander at most: enough to break the
//! network's resonances up, far from enough to be heard as a chorus.
constexpr double maxModDepthMs { 0.8 };

constexpr double minLowCutHz { 20.0 };
constexpr double maxLowCutHz { 1000.0 };
constexpr double minHighCutHz { 1000.0 };
constexpr double maxHighCutHz { 20000.0 };
constexpr double filterQ { 0.707 };

constexpr double maxWidth { 2.0 };

//! Where each early reflection arrives, as a fraction of the span, and how loud and which way up.
//! The gaps shrink as the field runs, vary by more than ten to one and none is a near-repeat of
//! another: even spacing is a comb filter, which is what makes a short room ring. The channels
//! differ throughout, and that difference is most of the stereo image.
constexpr std::array<double, Ambience::NumTaps> tapTimesLeft {
    0.0200, 0.1279, 0.2033, 0.3013, 0.3315, 0.3873, 0.4564, 0.5205, 0.5444, 0.6029, 0.6299, 0.6495,
    0.6668, 0.7101, 0.7494, 0.7903, 0.8251, 0.8564, 0.8824, 0.8962, 0.9072, 0.9168, 0.9376, 0.9659
};
constexpr std::array<double, Ambience::NumTaps> tapGainsLeft {
    -0.874, -0.955, 0.934, 0.914, -0.843, -0.894, -0.856, -0.974, -0.830, 0.870, 0.827, 0.747,
    -0.767, 0.847, -0.810, 0.719, 0.855, 0.997, 0.942, 0.790, -0.860, 0.825, -0.854, 0.823
};
constexpr std::array<double, Ambience::NumTaps> tapTimesRight {
    0.0310, 0.1543, 0.2424, 0.3463, 0.4314, 0.4599, 0.4867, 0.5262, 0.5719, 0.6085, 0.6603, 0.7049,
    0.7396, 0.7861, 0.8110, 0.8246, 0.8472, 0.8684, 0.8798, 0.9015, 0.9190, 0.9292, 0.9424, 0.9593
};
constexpr std::array<double, Ambience::NumTaps> tapGainsRight {
    0.895, 0.860, 0.994, -0.949, 0.833, -0.930, 0.971, 0.857, 0.705, 0.981, -0.898, 0.754,
    0.818, 0.975, -0.883, 0.962, -0.916, -0.845, 0.937, 0.910, -0.972, -0.935, 0.941, -0.849
};

//! The level of the tail against the early field, before Early/Late. Fitted so that the two read
//! about equally loud at the defaults.
constexpr double lateOutputScale { 0.5 };

//! How much of each channel's space is fed to the other. Left wholly apart the two channels are
//! uncorrelated, which is as wide as a space gets but leaves the mono sum a full 3 dB down; a
//! little shared puts back a centre without narrowing it to the ear.
constexpr double crossFeed { 0.12 };

//! Below this the tail counts as gone: -100 dB.
constexpr double settledLevel { 1.0e-5 };

//! Head-room in every buffer, so a sample-rate change or the modulation cannot read off the end.
constexpr double bufferMarginMs { 5.0 };

uint32_t samplesFor(double ms, double sampleRate)
{
    return static_cast<uint32_t>(std::ceil(ms * 0.001 * sampleRate));
}

double flushDenormal(double value)
{
    return std::abs(value) < 1.0e-20 ? 0.0 : value;
}

std::string key(const QString & name)
{
    return name.toStdString();
}

} // namespace

Ambience::Ambience()
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    addParameter(Parameter { key(C::xmlKeySize()), 0.4f, 0, 10000, 4000, 100 });
    addParameter(Parameter { key(C::xmlKeyDecay()), decayPosition(0.8), 0, 10000, static_cast<int>(std::lround(decayPosition(0.8) * 10000.0f)), 100 });
    addParameter(Parameter { key(C::xmlKeyPreDelay()), preDelayPosition(8.0), 0, 100, 8, 1 });
    addParameter(Parameter { key(C::xmlKeyDiffusion()), 0.7f, 0, 10000, 7000, 100 });
    addParameter(Parameter { key(C::xmlKeyShape()), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { key(C::xmlKeyEarlyLate()), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { key(C::xmlKeyDamping()), 0.35f, 0, 10000, 3500, 100 });
    addParameter(Parameter { key(C::xmlKeyRate()), modRatePosition(0.5), 0, 10000, static_cast<int>(std::lround(modRatePosition(0.5) * 10000.0f)), 100 });
    addParameter(Parameter { key(C::xmlKeyDepth()), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { key(C::xmlKeyHpfCutoff()), lowCutPosition(150.0), 0, 10000, static_cast<int>(std::lround(lowCutPosition(150.0) * 10000.0f)), 100 });
    addParameter(Parameter { key(C::xmlKeyLpfCutoff()), highCutPosition(9000.0), 0, 10000, static_cast<int>(std::lround(highCutPosition(9000.0) * 10000.0f)), 100 });
    addParameter(Parameter { key(C::xmlKeyWidth()), 0.5f, 0, 200, 100, 1 });

    // The dry stays whole: a space is something added to a sound, not something it is replaced with.
    // Audible from the start, unlike the other reverbs, as the point is a level that sits under
    // the source rather than one that has to be found.
    addMixParameter(0.3f, MixLaw::Additive, 0, 10000, 100);
    addSoloParameter();

    // The readouts the dialog shows, so that a parameter automation speaks in the same units
    const auto describe = [this](const QString & name, Display display) {
        setParameterDisplay(key(name), std::move(display));
    };
    describe(C::xmlKeyDecay(), { .mapping = Display::Mapping::Exponential, .unit = "s", .min = minDecaySeconds, .max = maxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Decay") });
    describe(C::xmlKeyPreDelay(), { .unit = "ms", .min = 0.0, .max = maxPreDelayMs, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pre-Delay") });
    describe(C::xmlKeyEarlyLate(), { .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Early/Late") });
    describe(C::xmlKeyRate(), { .mapping = Display::Mapping::Exponential, .unit = "Hz", .min = minModRateHz, .max = maxModRateHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Mod Rate") });
    describe(C::xmlKeyWidth(), { .mapping = Display::Mapping::Value, .unit = "%", .min = 0.0, .max = maxWidth * 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Width") });
    describe(C::xmlKeyHpfCutoff(), { .mapping = Display::Mapping::LogFrequency, .unit = "Hz", .min = minLowCutHz, .max = maxLowCutHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Low Cut") });
    describe(C::xmlKeyLpfCutoff(), { .mapping = Display::Mapping::LogFrequency, .unit = "Hz", .min = minHighCutHz, .max = maxHighCutHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "High Cut") });

    syncParameters();
}

std::string Ambience::typeIdString()
{
    return "ambience";
}

std::string Ambience::type() const
{
    return Constants::RackEffectType::ambience().toStdString();
}

std::string Ambience::typeId() const
{
    return typeIdString();
}

double Ambience::decaySeconds(float position)
{
    return ParameterMapper::mapExponential(static_cast<double>(position), minDecaySeconds, maxDecaySeconds);
}

float Ambience::decayPosition(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, minDecaySeconds, maxDecaySeconds));
}

double Ambience::preDelayMs(float position)
{
    return std::clamp(static_cast<double>(position), 0.0, 1.0) * maxPreDelayMs;
}

float Ambience::preDelayPosition(double ms)
{
    return static_cast<float>(std::clamp(ms / maxPreDelayMs, 0.0, 1.0));
}

double Ambience::modRateHz(float position)
{
    return ParameterMapper::mapExponential(static_cast<double>(position), minModRateHz, maxModRateHz);
}

float Ambience::modRatePosition(double hz)
{
    return static_cast<float>(ParameterMapper::unmapExponential(hz, minModRateHz, maxModRateHz));
}

double Ambience::lowCutHz(float position)
{
    return ParameterMapper::mapLogFrequency(static_cast<double>(position), minLowCutHz, maxLowCutHz);
}

float Ambience::lowCutPosition(double hz)
{
    return static_cast<float>(ParameterMapper::unmapLogFrequency(hz, minLowCutHz, maxLowCutHz));
}

double Ambience::highCutHz(float position)
{
    return ParameterMapper::mapLogFrequency(static_cast<double>(position), minHighCutHz, maxHighCutHz);
}

float Ambience::highCutPosition(double hz)
{
    return static_cast<float>(ParameterMapper::unmapLogFrequency(hz, minHighCutHz, maxHighCutHz));
}

double Ambience::Allpass::process(double input)
{
    const double buffered = buffer[writePos];
    const double stored = flushDenormal(input + buffered * coeff);
    buffer[writePos] = stored;
    writePos = (writePos + 1) % static_cast<uint32_t>(buffer.size());
    return buffered - stored * coeff;
}

void Ambience::Allpass::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0);
    writePos = 0;
}

double Ambience::Line::read(double delaySamples)
{
    // All-pass interpolated rather than linearly. A linear read is a low-pass that changes with the
    // fraction, and inside a loop it is applied on every pass: it took 13 % off every decay time
    // and more off the top. The all-pass keeps the magnitude whole, so only the loop gain decides
    // how fast the tail dies. Fraction kept between a half and one and a half samples, where the
    // first-order section's phase is closest to the delay it stands in for.
    const auto whole = static_cast<size_t>(std::max(1.0, std::floor(delaySamples - 0.5)));
    const double fraction = delaySamples - static_cast<double>(whole);
    const double eta = (1.0 - fraction) / (1.0 + fraction);
    const auto size = buffer.size();
    const double newer = buffer[(writePos + size - whole) % size];
    const double older = buffer[(writePos + size - whole - 1) % size];
    interpolatorState = flushDenormal(eta * newer + older - eta * interpolatorState);
    return interpolatorState;
}

void Ambience::Line::write(double sample)
{
    buffer[writePos] = sample;
    writePos = (writePos + 1) % static_cast<uint32_t>(buffer.size());
}

void Ambience::Line::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0);
    writePos = 0;
    damping.reset();
    interpolatorState = 0.0;
}

void Ambience::Channel::reset()
{
    std::fill(earlyBuffer.begin(), earlyBuffer.end(), 0.0);
    earlyWritePos = 0;
    std::fill(preDelayBuffer.begin(), preDelayBuffer.end(), 0.0);
    preDelayWritePos = 0;
    for (auto && allpass : diffusers) {
        allpass.reset();
    }
    earlyDamping.reset();
    lowCut.reset();
    highCut.reset();
}

void Ambience::updateBuffers()
{
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    const auto resize = [](std::vector<double> & buffer, uint32_t size) {
        if (buffer.size() != size) {
            buffer.assign(size, 0.0);
        }
    };
    for (auto && channel : { std::ref(m_left), std::ref(m_right) }) {
        resize(channel.get().preDelayBuffer, samplesFor(maxPreDelayMs + bufferMarginMs, rate));
        resize(channel.get().earlyBuffer, samplesFor(maxSpanMs + bufferMarginMs, rate));
        channel.get().preDelayWritePos = 0;
        channel.get().earlyWritePos = 0;
    }
    for (size_t i = 0; i < NumDiffusers; i++) {
        m_left.diffusers[i].buffer.assign(std::max<uint32_t>(1, samplesFor(diffuserLengthsMs[i], rate)), 0.0);
        m_right.diffusers[i].buffer.assign(std::max<uint32_t>(1, samplesFor(diffuserLengthsMs[i] * rightDiffuserStretch, rate)), 0.0);
        m_left.diffusers[i].writePos = 0;
        m_right.diffusers[i].writePos = 0;
    }
    for (size_t i = 0; i < NumLines; i++) {
        resize(m_lines[i].buffer, samplesFor(lineLengthsMs[i] + maxModDepthMs + bufferMarginMs, rate));
        m_lines[i].writePos = 0;
        // Spread around the circle, so the lines never drift together
        m_lines[i].lfoPhase = twoPi * static_cast<double>(i) / static_cast<double>(NumLines);
    }
    m_lastSampleRate = static_cast<uint32_t>(rate);

    // A new rate is a new stream: a render at 44.1 kHz after playing at 48 kHz, or the other way
    // round. Whatever is left in a buffer that happened to keep its size belongs to the old one,
    // and would come back as a tail at the wrong pitch at the start of the new
    reset();
}

void Ambience::updateTaps(Channel & channel, const std::array<double, NumTaps> & times, const std::array<double, NumTaps> & gains) const
{
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    const double spanMs = minSpanMs + static_cast<double>(m_size) * (maxSpanMs - minSpanMs);
    // Shape tilts the envelope of the field: falling away at the bottom, flat in the middle and
    // building up towards the end of the span at the top
    const double tilt = 5.0 * (1.0 - 2.0 * m_shape);
    double energy = 0.0;
    for (size_t i = 0; i < NumTaps; i++) {
        channel.taps[i] = std::max<uint32_t>(1, samplesFor(times[i] * spanMs, rate));
        channel.tapGains[i] = gains[i] * std::exp(-tilt * times[i]);
        energy += channel.tapGains[i] * channel.tapGains[i];
    }
    // As loud as the signal fed to it, whatever the shape
    const double scale = energy > 0.0 ? 1.0 / std::sqrt(energy) : 0.0;
    for (auto && gain : channel.tapGains) {
        gain *= scale;
    }
}

void Ambience::syncParameters()
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & name) {
        const auto p = parameter(key(name));
        return p ? p->get().value() : 0.0f;
    };
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();

    m_size = std::clamp(valueOf(C::xmlKeySize()), 0.0f, 1.0f);
    m_decaySeconds = decaySeconds(valueOf(C::xmlKeyDecay()));
    m_preDelaySamples = preDelayMs(valueOf(C::xmlKeyPreDelay())) * 0.001 * rate;
    m_diffusion = std::clamp(static_cast<double>(valueOf(C::xmlKeyDiffusion())), 0.0, 1.0);
    m_shape = std::clamp(static_cast<double>(valueOf(C::xmlKeyShape())), 0.0, 1.0);
    // Equal power across the travel, so the middle is not a dip
    const double earlyLate = std::clamp(static_cast<double>(valueOf(C::xmlKeyEarlyLate())), 0.0, 1.0);
    m_earlyGain = std::cos(earlyLate * std::numbers::pi * 0.5);
    m_lateGain = std::sin(earlyLate * std::numbers::pi * 0.5);
    const double damping = std::clamp(static_cast<double>(valueOf(C::xmlKeyDamping())), 0.0, 1.0);
    m_damped = damping > 0.0;
    m_dampingHz = maxDampingHz * std::pow(minDampingHz / maxDampingHz, damping);
    m_modRateHz = modRateHz(valueOf(C::xmlKeyRate()));
    m_modDepthSamples = std::clamp(static_cast<double>(valueOf(C::xmlKeyDepth())), 0.0, 1.0) * maxModDepthMs * 0.001 * rate;
    const float lowCut = valueOf(C::xmlKeyHpfCutoff());
    const float highCut = valueOf(C::xmlKeyLpfCutoff());
    m_lowCutEnabled = lowCut > 0.0f;
    m_highCutEnabled = highCut < 1.0f;
    m_width = std::clamp(static_cast<double>(valueOf(C::xmlKeyWidth())), 0.0, 1.0) * maxWidth;

    for (auto && allpass : m_left.diffusers) {
        allpass.coeff = m_diffusion * maxDiffusionCoefficient;
    }
    for (auto && allpass : m_right.diffusers) {
        allpass.coeff = m_diffusion * maxDiffusionCoefficient;
    }
    updateTaps(m_left, tapTimesLeft, tapGainsLeft);
    updateTaps(m_right, tapTimesRight, tapGainsRight);
    for (auto && channel : { std::ref(m_left), std::ref(m_right) }) {
        channel.get().earlyDamping.calculate(std::min(maxDampingHz, m_dampingHz * 1.5), rate);
        channel.get().lowCut.calculateLowCut(lowCutHz(lowCut), rate, filterQ);
        channel.get().highCut.calculateHighCut(highCutHz(highCut), rate, filterQ);
    }

    const double lineScale = minLineScale + (1.0 - minLineScale) * static_cast<double>(m_size);
    for (size_t i = 0; i < NumLines; i++) {
        auto && line = m_lines[i];
        line.lengthSamples = std::max(2.0, lineLengthsMs[i] * lineScale * 0.001 * rate);
        // Whatever the line's length, sixty decibels down in the time Decay asks for
        line.gain = std::pow(10.0, -3.0 * line.lengthSamples / (m_decaySeconds * rate));
        // Each line at its own rate, so their drift never lines up
        line.lfoIncrement = twoPi * m_modRateHz * (0.83 + 0.047 * static_cast<double>(i)) / rate;
        line.damping.calculate(m_dampingHz, rate);
    }
}

double Ambience::diffuseAndStore(Channel & channel, double input)
{
    // Pre-delay first, so both the early field and the tail come after it
    const auto preDelaySize = channel.preDelayBuffer.size();
    channel.preDelayBuffer[channel.preDelayWritePos] = input;
    const auto delay = std::min(static_cast<size_t>(std::lround(m_preDelaySamples)), preDelaySize - 1);
    const double delayed = channel.preDelayBuffer[(channel.preDelayWritePos + preDelaySize - delay) % preDelaySize];
    channel.preDelayWritePos = static_cast<uint32_t>((channel.preDelayWritePos + 1) % preDelaySize);

    double diffused = delayed;
    if (m_diffusion > 0.0) {
        for (auto && allpass : channel.diffusers) {
            diffused = allpass.process(diffused);
        }
    }
    channel.earlyBuffer[channel.earlyWritePos] = diffused;
    channel.earlyWritePos = static_cast<uint32_t>((channel.earlyWritePos + 1) % channel.earlyBuffer.size());
    return diffused;
}

double Ambience::renderEarly(const Channel & channel) const
{
    const auto size = channel.earlyBuffer.size();
    // The newest sample is one behind the write position
    const auto newest = (channel.earlyWritePos + size - 1) % size;
    double sum = 0.0;
    for (size_t i = 0; i < NumTaps; i++) {
        sum += channel.earlyBuffer[(newest + size - std::min<size_t>(channel.taps[i], size - 1)) % size] * channel.tapGains[i];
    }
    return sum;
}

void Ambience::renderFrame(double & left, double & right)
{
    const double diffusedL = diffuseAndStore(m_left, left);
    const double diffusedR = diffuseAndStore(m_right, right);

    double earlyL = renderEarly(m_left);
    double earlyR = renderEarly(m_right);
    if (m_damped) {
        m_left.earlyDamping.process(earlyL);
        m_right.earlyDamping.process(earlyR);
        earlyL = m_left.earlyDamping.lowPass();
        earlyR = m_right.earlyDamping.lowPass();
    }

    // The tail: every line read at its drifting length, damped, scaled to the decay, and mixed into
    // every other through a Householder reflection, which loses no energy of its own
    std::array<double, NumLines> outs {};
    double sum = 0.0;
    for (size_t i = 0; i < NumLines; i++) {
        auto && line = m_lines[i];
        const double drift = m_modDepthSamples * std::sin(line.lfoPhase);
        line.lfoPhase += line.lfoIncrement;
        if (line.lfoPhase >= twoPi) {
            line.lfoPhase -= twoPi;
        }
        double out = line.read(line.lengthSamples + drift);
        if (m_damped) {
            line.damping.process(out);
            out = line.damping.lowPass();
        }
        outs[i] = flushDenormal(out * line.gain);
        sum += outs[i];
    }
    const double reflection = sum * 2.0 / static_cast<double>(NumLines);
    for (size_t i = 0; i < NumLines; i++) {
        // The left half of the lines takes the left, the right half the right
        const double input = (i % 2 == 0 ? diffusedL : diffusedR) * lineInputSigns[i] * 0.5;
        m_lines[i].write(input + outs[i] - reflection);
    }
    const double lateL = (outs[0] - outs[2] + outs[4] - outs[6]) * lateOutputScale;
    const double lateR = (outs[1] - outs[3] + outs[5] - outs[7]) * lateOutputScale;
    m_tailPeak = std::max({ m_tailPeak, std::abs(lateL), std::abs(lateR), std::abs(earlyL), std::abs(earlyR) });

    const double spaceL = earlyL * m_earlyGain + lateL * m_lateGain;
    const double spaceR = earlyR * m_earlyGain + lateR * m_lateGain;
    double wetL = spaceL + spaceR * crossFeed;
    double wetR = spaceR + spaceL * crossFeed;

    const double mid = (wetL + wetR) * 0.5;
    const double side = (wetL - wetR) * 0.5 * m_width;
    wetL = mid + side;
    wetR = mid - side;

    if (m_lowCutEnabled) {
        wetL = m_left.lowCut.process(wetL);
        wetR = m_right.lowCut.process(wetR);
    }
    if (m_highCutEnabled) {
        wetL = m_left.highCut.process(wetL);
        wetR = m_right.highCut.process(wetR);
    }

    left = wetL;
    right = wetR;
}

void Ambience::processSample(double & left, double & right)
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate) {
        updateBuffers();
        m_shouldSyncParameters = true;
    }
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
    renderFrame(left, right);
}

void Ambience::processBlock(AudioContext & context)
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate) {
        updateBuffers();
        m_shouldSyncParameters = true;
    }
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
    m_tailPeak = 0.0;
    for (uint32_t i = 0; i < context.frameCount; i++) {
        renderFrame(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Ambience::reset()
{
    m_left.reset();
    m_right.reset();
    for (size_t i = 0; i < NumLines; i++) {
        m_lines[i].reset();
        m_lines[i].lfoPhase = twoPi * static_cast<double>(i) / static_cast<double>(NumLines);
    }
    m_tailPeak = 0.0;
}

void Ambience::sync()
{
    m_shouldSyncParameters = true;
}

bool Ambience::isSettled() const
{
    return m_tailPeak < settledLevel;
}

const EffectPresetList & Ambience::factoryPresets() const
{
    namespace C = Constants::NahdXml;
    const auto room = [](float size, double decay, double preDelay, float diffusion, float shape, float earlyLate, float damping, double lowCut, double highCut, float width, float mix) {
        return std::map<std::string, float> {
            { key(C::xmlKeySize()), size },
            { key(C::xmlKeyDecay()), decayPosition(decay) },
            { key(C::xmlKeyPreDelay()), preDelayPosition(preDelay) },
            { key(C::xmlKeyDiffusion()), diffusion },
            { key(C::xmlKeyShape()), shape },
            { key(C::xmlKeyEarlyLate()), earlyLate },
            { key(C::xmlKeyDamping()), damping },
            { key(C::xmlKeyHpfCutoff()), lowCutPosition(lowCut) },
            { key(C::xmlKeyLpfCutoff()), highCutPosition(highCut) },
            { key(C::xmlKeyWidth()), width / 2.0f },
            { key(C::xmlKeyMix()), mix },
        };
    };
    // Init first, as the way back; the rest alphabetical
    static const EffectPresetList presets {
        { "Init", {} },
        // Tiny and dry-sounding, mostly early: a voice in a treated room
        { "Booth", room(0.1f, 0.3, 2.0, 0.6f, 0.15f, 0.3f, 0.5f, 200.0, 8000.0, 0.8f, 0.25f) },
        // A punchy room for kit and percussion: early-heavy, the lows kept out
        { "Drum Room", room(0.35f, 0.6, 3.0, 0.75f, 0.35f, 0.4f, 0.4f, 250.0, 7000.0, 1.2f, 0.3f) },
        // A real room around everything, a little longer and more open
        { "Live Room", room(0.6f, 1.1, 10.0, 0.8f, 0.4f, 0.55f, 0.3f, 150.0, 9000.0, 1.0f, 0.3f) },
        // Under a whole mix at a low level: smooth, short, no mud and no fizz
        { "Mix Glue", room(0.45f, 0.8, 8.0, 0.8f, 0.3f, 0.5f, 0.35f, 180.0, 8000.0, 1.0f, 0.2f) },
        // Bright and open, held off the voice so the words stay in front
        { "Vocal Air", room(0.5f, 1.0, 25.0, 0.85f, 0.5f, 0.6f, 0.15f, 300.0, 14000.0, 1.1f, 0.25f) },
        // Wide and soft, for pads and keys
        { "Wide Ambience", room(0.75f, 1.6, 15.0, 0.9f, 0.6f, 0.7f, 0.35f, 200.0, 10000.0, 1.6f, 0.3f) },
    };
    return presets;
}

} // namespace noteahead
