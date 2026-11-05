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

#include "reverb_224.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double twoPi { 2.0 * std::numbers::pi };

constexpr double minDecaySeconds { 0.6 };
constexpr double maxDecaySeconds { 70.0 };
//! Both the crossover and the treble cutoff, as on the machine.
constexpr double minCutoffHz { 100.0 };
constexpr double maxCutoffHz { 10900.0 };
constexpr double maxPreDelayMs { 255.0 };
//! The Low Cut's travel. At the bottom it is out of the circuit, not a filter at 20 Hz, so a
//! project saved before it existed sounds as it did.
constexpr double minLowCutHz { 20.0 };
constexpr double maxLowCutHz { 1000.0 };
constexpr double lowCutQ { 1.0 / std::numbers::sqrt2 };

//! The machine's band limit, an eighth-order low-pass in front of the converter. The same is put
//! behind it to take the images of the machine's rate back out.
constexpr double bandwidthHz { 8100.0 };

//! The gain ranger's thresholds, as fractions of the converter's full scale (service manual
//! Table 3.3: 0.56, 1.12 and 2.24 V against clipping at 5 V). Below each the signal is amplified a
//! further 6 dB before conversion and shifted back down after it.
constexpr std::array<double, 3> rangeThresholds { 0.112, 0.224, 0.448 };
constexpr double converterSteps { 2047.0 };

//! Dattorro's tank was published at this rate, and its lengths are in samples at it.
constexpr double dattorroRate { 29761.0 };
constexpr double rateScale { Reverb224::InternalRate / dattorroRate };

constexpr std::array<double, Reverb224::NumDiffusers> diffuserLengths { 142.0, 107.0, 379.0, 277.0 };
//! The first two diffusers are denser than the last two.
constexpr std::array<double, Reverb224::NumDiffusers> diffuserWeights { 1.0, 1.0, 0.8333, 0.8333 };

struct HalfLengths
{
    double modulated;
    double firstDelay;
    double second;
    double secondDelay;
};

constexpr std::array<HalfLengths, 2> halfLengths { { { 672.0, 4453.0, 1800.0, 3720.0 }, { 908.0, 4217.0, 2656.0, 3163.0 } } };
//! The furthest the modulated all-passes swing either way, at the top of Enhance.
constexpr double maxExcursion { 16.0 };
//! Each half's modulation at its own rate, so the two never line up.
constexpr std::array<double, 2> lfoRatesHz { 0.93, 1.21 };

//! Where the output is tapped from the tank: which half, which element, how far in and which way
//! up. The left channel's seven taps first, then the right's.
enum class Element
{
    FirstDelay,
    Second,
    SecondDelay
};

struct OutputTap
{
    size_t half;
    Element element;
    double position;
    double sign;
};

constexpr std::array<OutputTap, 14> outputTaps { {
  { 1, Element::FirstDelay, 266.0, 1.0 },
  { 1, Element::FirstDelay, 2974.0, 1.0 },
  { 1, Element::Second, 1913.0, -1.0 },
  { 1, Element::SecondDelay, 1996.0, 1.0 },
  { 0, Element::FirstDelay, 1990.0, -1.0 },
  { 0, Element::Second, 187.0, -1.0 },
  { 0, Element::SecondDelay, 1066.0, -1.0 },
  { 0, Element::FirstDelay, 353.0, 1.0 },
  { 0, Element::FirstDelay, 3627.0, 1.0 },
  { 0, Element::Second, 1228.0, -1.0 },
  { 0, Element::SecondDelay, 2673.0, 1.0 },
  { 1, Element::FirstDelay, 2111.0, -1.0 },
  { 1, Element::Second, 335.0, -1.0 },
  { 1, Element::SecondDelay, 121.0, -1.0 },
} };
constexpr double tankOutputScale { 0.6 };

//! What sets one program apart from another: how big the tank is, how dense it is, and how far the
//! early reflections spread. A span of zero means none, as on the plates.
//!
//! The all-passes in the loop stretch the decay: their delay varies with frequency, the modes
//! crowd where it is longest, and those decay slowest. How much depends on the program, so each
//! carries the factor its decay was measured long by: octave bands from 125 Hz to 2 kHz, decays
//! from 1.5 to 5 s, averaged.
struct ProgramShape
{
    double tankScale;
    double inputDiffusion;
    double decayDiffusion1;
    double decayDiffusion2;
    double earlySpanMs;
    double decayStretch;
};

constexpr std::array<ProgramShape, Reverb224::ProgramCount> programShapes { {
  { 1.0, 0.75, 0.70, 0.50, 40.0, 1.035 }, // Small Hall A
  { 1.15, 0.70, 0.65, 0.45, 55.0, 1.02 }, // Small Hall B: larger and grainier
  { 1.6, 0.75, 0.70, 0.50, 80.0, 1.035 }, // Large Hall
  { 0.85, 0.80, 0.70, 0.50, 0.0, 1.045 }, // Vocal Plate: dense from the start
  { 0.75, 0.70, 0.60, 0.45, 25.0, 1.05 }, // Chamber
  { 0.65, 0.85, 0.75, 0.55, 0.0, 1.07 }, // Percussion Plate: small, bright and dense
  { 0.9, 0.80, 0.75, 0.60, 0.0, 1.075 }, // Constant Density Plate
  { 0.45, 0.65, 0.60, 0.40, 18.0, 1.01 }, // Room
} };
//! A second-order shelf, at its flattest without overshoot: its gain never strays outside the
//! two decays it joins, so it cannot make the loop gain more than one.
constexpr double bassShelfQ { 1.0 / std::numbers::sqrt2 };
constexpr double maxTankScale { 1.6 };
constexpr double maxEarlySpanMs { 80.0 };

//! Where the early reflections arrive, as fractions of the span, and how loud and which way up.
//! The channels differ throughout, which is most of the early field's width.
constexpr std::array<double, Reverb224::NumEarlyTaps> earlyTimesLeft { 0.11, 0.23, 0.37, 0.52, 0.71, 0.93 };
constexpr std::array<double, Reverb224::NumEarlyTaps> earlyGainsLeft { 0.85, -0.70, 0.62, -0.50, 0.42, -0.33 };
constexpr std::array<double, Reverb224::NumEarlyTaps> earlyTimesRight { 0.14, 0.29, 0.41, 0.60, 0.78, 0.97 };
constexpr std::array<double, Reverb224::NumEarlyTaps> earlyGainsRight { 0.82, -0.68, 0.60, -0.52, 0.40, -0.30 };
//! The early field against the tank, before Depth.
constexpr double earlyOutputScale { 0.5 };

//! Below this the tail counts as gone: -100 dB.
constexpr double settledLevel { 1.0e-5 };

uint32_t machineSamples(double ms)
{
    return static_cast<uint32_t>(std::lround(ms * 0.001 * Reverb224::InternalRate));
}

double flushDenormal(double value)
{
    return std::abs(value) < 1.0e-20 ? 0.0 : value;
}

//! Catmull-Rom between @p x0 and @p x1, at @p t from 0 to 1.
double hermite(double xm1, double x0, double x1, double x2, double t)
{
    const double c1 = 0.5 * (x1 - xm1);
    const double c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
    const double c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);
    return ((c3 * t + c2) * t + c1) * t + x0;
}

std::string key(const QString & name)
{
    return name.toStdString();
}

} // namespace

Reverb224::Reverb224()
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    const auto continuous = [this](const QString & name, float position) {
        addParameter(Parameter { key(name), position, 0, 10000, static_cast<int>(std::lround(position * 10000.0f)), 100 });
    };
    addParameter(Parameter { key(C::xmlKeyProgram()), static_cast<float>(Program::LargeHall), 0, static_cast<int>(ProgramCount) - 1, static_cast<int>(Program::LargeHall), 1, Parameter::Type::Discrete });
    addParameter(Parameter { key(C::xmlKeyPreDelay()), preDelayPosition(20.0), 0, static_cast<int>(maxPreDelayMs), 20, 1 });
    continuous(C::xmlKeyBassDecay(), decayPosition(3.0));
    continuous(C::xmlKeyMidDecay(), decayPosition(2.5));
    continuous(C::xmlKeyCrossoverFreq(0), crossoverPosition(700.0));
    continuous(C::xmlKeyTrebleDecay(), treblePosition(5000.0));
    continuous(C::xmlKeyDepth(), 0.5f);
    continuous(C::xmlKeyDiffusion(), 0.8f);
    continuous(C::xmlKeyEnhance(), 0.5f);
    addParameter(Parameter { key(C::xmlKeyConverters()), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    continuous(C::xmlKeyHpfCutoff(), 0.0f);

    // Audible from the start, as on the Ambience: a reverb that is silent when added reads as broken
    addMixParameter(0.3f, MixLaw::Additive, 0, 10000, 100);
    addSoloParameter();

    // The readouts the dialog shows, so that a parameter automation speaks in the same units
    const auto describe = [this](const QString & name, Display display) {
        setParameterDisplay(key(name), std::move(display));
    };
    describe(C::xmlKeyPreDelay(), { .unit = "ms", .min = 0.0, .max = maxPreDelayMs, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pre-Delay") });
    describe(C::xmlKeyBassDecay(), { .mapping = Display::Mapping::Exponential, .unit = "s", .min = minDecaySeconds, .max = maxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Bass Decay") });
    describe(C::xmlKeyMidDecay(), { .mapping = Display::Mapping::Exponential, .unit = "s", .min = minDecaySeconds, .max = maxDecaySeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Mid Decay") });
    describe(C::xmlKeyCrossoverFreq(0), { .mapping = Display::Mapping::LogFrequency, .unit = "Hz", .min = minCutoffHz, .max = maxCutoffHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Crossover") });
    describe(C::xmlKeyTrebleDecay(), { .mapping = Display::Mapping::LogFrequency, .unit = "Hz", .min = minCutoffHz, .max = maxCutoffHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Treble Decay") });
    describe(C::xmlKeyHpfCutoff(), { .mapping = Display::Mapping::LogFrequencyHighPass, .unit = "Hz", .min = minLowCutHz, .max = maxLowCutHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Low Cut") });

    for (auto && allpass : m_diffusers) {
        allpass.line.buffer.assign(machineSamples(1.0) + static_cast<uint32_t>(std::ceil(*std::ranges::max_element(diffuserLengths) * rateScale)), 0.0);
    }
    for (size_t i = 0; i < m_halves.size(); i++) {
        auto && half = m_halves[i];
        const auto size = [](double length, double margin) {
            return static_cast<uint32_t>(std::ceil(length * rateScale * maxTankScale + margin)) + 4;
        };
        half.modulated.line.buffer.assign(size(halfLengths[i].modulated, maxExcursion * rateScale), 0.0);
        half.firstDelay.buffer.assign(size(halfLengths[i].firstDelay, 0.0), 0.0);
        half.second.line.buffer.assign(size(halfLengths[i].second, 0.0), 0.0);
        half.secondDelay.buffer.assign(size(halfLengths[i].secondDelay, 0.0), 0.0);
        half.lfoIncrement = twoPi * lfoRatesHz[i] / InternalRate;
    }
    m_input.buffer.assign(machineSamples(maxPreDelayMs + maxEarlySpanMs) + 4, 0.0);

    reset();
    syncParameters();
}

std::string Reverb224::typeIdString()
{
    return "reverb224";
}

std::string Reverb224::type() const
{
    return Constants::RackEffectType::reverb224().toStdString();
}

std::string Reverb224::typeId() const
{
    return typeIdString();
}

double Reverb224::decaySeconds(float position)
{
    return ParameterMapper::mapExponential(static_cast<double>(position), minDecaySeconds, maxDecaySeconds);
}

float Reverb224::decayPosition(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, minDecaySeconds, maxDecaySeconds));
}

double Reverb224::crossoverHz(float position)
{
    return ParameterMapper::mapLogFrequency(static_cast<double>(position), minCutoffHz, maxCutoffHz);
}

float Reverb224::crossoverPosition(double hz)
{
    return static_cast<float>(ParameterMapper::unmapLogFrequency(hz, minCutoffHz, maxCutoffHz));
}

double Reverb224::trebleHz(float position)
{
    return crossoverHz(position);
}

float Reverb224::treblePosition(double hz)
{
    return crossoverPosition(hz);
}

double Reverb224::lowCutHz(float position)
{
    return ParameterMapper::mapLogFrequency(std::clamp(static_cast<double>(position), 0.0, 1.0), minLowCutHz, maxLowCutHz);
}

float Reverb224::lowCutPosition(double hz)
{
    return static_cast<float>(ParameterMapper::unmapLogFrequency(hz, minLowCutHz, maxLowCutHz));
}

double Reverb224::preDelayMs(float position)
{
    return std::clamp(static_cast<double>(position), 0.0, 1.0) * maxPreDelayMs;
}

float Reverb224::preDelayPosition(double ms)
{
    return static_cast<float>(std::clamp(ms / maxPreDelayMs, 0.0, 1.0));
}

double Reverb224::convert(double value)
{
    // The ranger picks the largest gain that keeps the sample under clipping
    const double magnitude = std::abs(value);
    double gain = 1.0;
    for (auto && threshold : rangeThresholds) {
        if (magnitude < threshold) {
            gain *= 2.0;
        }
    }
    const double code = std::round(std::clamp(value * gain * converterSteps, -converterSteps - 1.0, converterSteps));
    return code / (converterSteps * gain);
}

bool Reverb224::hasEarlyReflections(Program program)
{
    return programShapes.at(static_cast<size_t>(program)).earlySpanMs > 0.0;
}

double Reverb224::Line::read(double delaySamples)
{
    // All-pass interpolated, as on the Ambience: a linear read is a low-pass that changes with the
    // fraction, and inside the loop it would take the top off the tail on every pass. Fraction kept
    // between a half and one and a half samples, where the section's phase is closest to a delay.
    const auto whole = static_cast<size_t>(std::max(1.0, std::floor(delaySamples - 0.5)));
    const double fraction = delaySamples - static_cast<double>(whole);
    const double eta = (1.0 - fraction) / (1.0 + fraction);
    const auto size = buffer.size();
    const double newer = buffer[(writePos + size - whole) % size];
    const double older = buffer[(writePos + size - whole - 1) % size];
    interpolatorState = flushDenormal(eta * newer + older - eta * interpolatorState);
    return interpolatorState;
}

double Reverb224::Line::tap(uint32_t delaySamples) const
{
    const auto size = buffer.size();
    return buffer[(writePos + size - std::min<size_t>(delaySamples, size)) % size];
}

void Reverb224::Line::write(double sample)
{
    buffer[writePos] = sample;
    writePos = static_cast<uint32_t>((writePos + 1) % buffer.size());
}

void Reverb224::Line::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0);
    writePos = 0;
    interpolatorState = 0.0;
}

double Reverb224::Allpass::process(double input, double modulation)
{
    const double delayed = modulation == 0.0 ? line.tap(static_cast<uint32_t>(length)) : line.read(length + modulation);
    const double stored = flushDenormal(input - coeff * delayed);
    line.write(stored);
    return delayed + coeff * stored;
}

void Reverb224::Allpass::reset()
{
    line.reset();
}

void Reverb224::Half::reset()
{
    modulated.reset();
    firstDelay.reset();
    treble.reset();
    bassShelf.reset();
    second.reset();
    secondDelay.reset();
    output = 0.0;
}

void Reverb224::syncParameters()
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & name) {
        const auto p = parameter(key(name));
        return p ? p->get().value() : 0.0f;
    };

    m_program = static_cast<Program>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyProgram()))), 0, static_cast<int>(ProgramCount) - 1));
    m_converters = valueOf(C::xmlKeyConverters()) > 0.5f;
    const auto & shape = programShapes.at(static_cast<size_t>(m_program));

    m_preDelaySamples = machineSamples(preDelayMs(valueOf(C::xmlKeyPreDelay())));
    m_lowCutHz = lowCutHz(valueOf(C::xmlKeyHpfCutoff()));
    updateLowCut();
    // Equal power across the travel. A program without early reflections is all tank whatever
    // Depth says, so switching to a plate does not turn it down.
    const double depth = std::clamp(static_cast<double>(valueOf(C::xmlKeyDepth())), 0.0, 1.0);
    const bool early = shape.earlySpanMs > 0.0;
    m_earlyGain = early ? std::cos(depth * std::numbers::pi * 0.5) * earlyOutputScale : 0.0;
    m_tankGain = early ? std::sin(depth * std::numbers::pi * 0.5) : 1.0;
    m_excursionSamples = std::clamp(static_cast<double>(valueOf(C::xmlKeyEnhance())), 0.0, 1.0) * maxExcursion * rateScale;

    const double diffusion = std::clamp(static_cast<double>(valueOf(C::xmlKeyDiffusion())), 0.0, 1.0);
    for (size_t i = 0; i < NumDiffusers; i++) {
        m_diffusers[i].length = std::round(diffuserLengths[i] * rateScale);
        m_diffusers[i].coeff = diffusion * shape.inputDiffusion * diffuserWeights[i];
    }

    const auto updateEarly = [&shape](std::array<uint32_t, NumEarlyTaps> & taps, std::array<double, NumEarlyTaps> & gains, const std::array<double, NumEarlyTaps> & times, const std::array<double, NumEarlyTaps> & levels) {
        double energy = 0.0;
        for (size_t i = 0; i < NumEarlyTaps; i++) {
            taps[i] = machineSamples(times[i] * shape.earlySpanMs);
            gains[i] = levels[i];
            energy += levels[i] * levels[i];
        }
        // As loud as the signal fed to it
        for (auto && gain : gains) {
            gain /= std::sqrt(energy);
        }
    };
    updateEarly(m_earlyTapsLeft, m_earlyGainsLeft, earlyTimesLeft, earlyGainsLeft);
    updateEarly(m_earlyTapsRight, m_earlyGainsRight, earlyTimesRight, earlyGainsRight);

    const double bassSeconds = decaySeconds(valueOf(C::xmlKeyBassDecay()));
    const double midSeconds = decaySeconds(valueOf(C::xmlKeyMidDecay()));
    const double crossover = crossoverHz(valueOf(C::xmlKeyCrossoverFreq(0)));
    const double treble = trebleHz(valueOf(C::xmlKeyTrebleDecay()));
    const auto scaled = [&shape](double length) {
        return std::max(1.0, std::round(length * rateScale * shape.tankScale));
    };
    for (size_t i = 0; i < m_halves.size(); i++) {
        auto && half = m_halves[i];
        half.modulated.length = scaled(halfLengths[i].modulated);
        // Dattorro's decay diffusion 1 is the other way up from every other all-pass in the tank
        half.modulated.coeff = -shape.decayDiffusion1;
        half.firstLength = static_cast<uint32_t>(scaled(halfLengths[i].firstDelay));
        half.second.length = scaled(halfLengths[i].second);
        half.second.coeff = shape.decayDiffusion2;
        half.secondLength = static_cast<uint32_t>(scaled(halfLengths[i].secondDelay));
        half.treble.calculate(treble, InternalRate);
        // Whatever the program's size, sixty decibels down in the time each band asks for. A pass
        // through a half takes as long as its four elements together.
        const double length = (half.modulated.length + static_cast<double>(half.firstLength) + half.second.length + static_cast<double>(half.secondLength)) * shape.decayStretch;
        const double bassGain = std::pow(10.0, -3.0 * length / (bassSeconds * InternalRate));
        half.midGain = std::pow(10.0, -3.0 * length / (midSeconds * InternalRate));
        half.bassShelf.calculateLowShelf(crossover, InternalRate, bassShelfQ, 20.0 * std::log10(bassGain / half.midGain));
    }

    for (size_t i = 0; i < outputTaps.size(); i++) {
        m_outputTaps[i] = static_cast<uint32_t>(scaled(outputTaps[i].position));
    }
}

void Reverb224::updateLowCut()
{
    m_lowCutActive = m_lowCutHz > minLowCutHz + 0.01;
    if (m_lowCutActive) {
        const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
        m_lowCut.calculateLowCut(m_lowCutHz, rate, lowCutQ);
    } else {
        // Picked up from silence when it comes back in
        m_lowCut.reset();
    }
}

void Reverb224::updateHostRate()
{
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    m_hostPerInternal = rate / InternalRate;
    const double cutoff = std::min(bandwidthHz, rate * 0.45);
    // Butterworth: the sections' Qs are the pole pairs' angles
    for (size_t i = 0; i < NumBandwidthSections; i++) {
        const double q = 1.0 / (2.0 * std::cos(static_cast<double>(2 * i + 1) * std::numbers::pi / (4.0 * NumBandwidthSections)));
        m_inputFilters[i].calculateHighCut(cutoff, rate, q);
        m_outputFiltersLeft[i].calculateHighCut(cutoff, rate, q);
        m_outputFiltersRight[i].calculateHighCut(cutoff, rate, q);
    }
    m_lastSampleRate = static_cast<uint32_t>(rate);
    updateLowCut();

    // A new rate is a new stream: whatever is left belongs to the old one
    reset();
}

double Reverb224::renderEarly(const std::array<uint32_t, NumEarlyTaps> & taps, const std::array<double, NumEarlyTaps> & gains) const
{
    double sum = 0.0;
    for (size_t i = 0; i < NumEarlyTaps; i++) {
        sum += m_input.tap(m_preDelaySamples + 1 + taps[i]) * gains[i];
    }
    return sum;
}

void Reverb224::tick(double input)
{
    m_input.write(m_converters ? convert(input) : input);
    double diffused = m_input.tap(m_preDelaySamples + 1);
    for (auto && allpass : m_diffusers) {
        diffused = allpass.process(diffused);
    }

    // The figure eight: each half takes the input and what the other half put out last
    const std::array<double, 2> crossing { m_halves[1].output, m_halves[0].output };
    for (size_t i = 0; i < m_halves.size(); i++) {
        auto && half = m_halves[i];
        const double modulation = m_excursionSamples * std::sin(half.lfoPhase);
        half.lfoPhase += half.lfoIncrement;
        if (half.lfoPhase >= twoPi) {
            half.lfoPhase -= twoPi;
        }
        const double modulated = half.modulated.process(diffused + crossing[i], modulation);
        const double delayed = half.firstDelay.tap(half.firstLength);
        half.firstDelay.write(modulated);

        half.treble.process(delayed);
        const double decayed = flushDenormal(half.bassShelf.process(half.treble.lowPass()) * half.midGain);

        const double diffusedAgain = half.second.process(decayed);
        half.output = half.secondDelay.tap(half.secondLength);
        half.secondDelay.write(diffusedAgain);
    }

    std::array<double, 2> tank {};
    for (size_t i = 0; i < outputTaps.size(); i++) {
        const auto & tap = outputTaps[i];
        const auto & half = m_halves[tap.half];
        const auto & line = tap.element == Element::FirstDelay ? half.firstDelay : tap.element == Element::Second ? half.second.line
                                                                                                                  : half.secondDelay;
        tank[i < outputTaps.size() / 2 ? 0 : 1] += line.tap(m_outputTaps[i]) * tap.sign;
    }

    double left = tank[0] * tankOutputScale * m_tankGain;
    double right = tank[1] * tankOutputScale * m_tankGain;
    if (m_earlyGain > 0.0) {
        left += renderEarly(m_earlyTapsLeft, m_earlyGainsLeft) * m_earlyGain;
        right += renderEarly(m_earlyTapsRight, m_earlyGainsRight) * m_earlyGain;
    }
    if (m_converters) {
        left = convert(left);
        right = convert(right);
    }

    std::shift_right(m_outputHistoryLeft.begin(), m_outputHistoryLeft.end(), 1);
    std::shift_right(m_outputHistoryRight.begin(), m_outputHistoryRight.end(), 1);
    m_outputHistoryLeft[0] = left;
    m_outputHistoryRight[0] = right;
}

void Reverb224::renderFrame(double & left, double & right)
{
    double input = (left + right) * 0.5;
    // On the way in only: the dry signal the Mix puts back keeps its lows
    if (m_lowCutActive) {
        input = m_lowCut.process(input);
    }
    for (auto && filter : m_inputFilters) {
        input = filter.process(input);
    }
    std::shift_left(m_inputHistory.begin(), m_inputHistory.end(), 1);
    m_inputHistory[3] = input;

    // Into the machine at its own rate, between the second and the third input sample
    while (m_inputPhase < 1.0) {
        tick(hermite(m_inputHistory[0], m_inputHistory[1], m_inputHistory[2], m_inputHistory[3], m_inputPhase));
        m_inputPhase += m_hostPerInternal;
    }
    // And back out, read one and two host periods' worth of machine samples behind the newest: far
    // enough that there is always a machine sample either side, whatever the two rates
    const double behind = std::clamp(1.0 + (m_inputPhase - 1.0) / m_hostPerInternal, 1.0, 2.0);
    m_inputPhase -= 1.0;
    const double t = 2.0 - behind;
    double wetL = hermite(m_outputHistoryLeft[3], m_outputHistoryLeft[2], m_outputHistoryLeft[1], m_outputHistoryLeft[0], t);
    double wetR = hermite(m_outputHistoryRight[3], m_outputHistoryRight[2], m_outputHistoryRight[1], m_outputHistoryRight[0], t);
    for (size_t i = 0; i < NumBandwidthSections; i++) {
        wetL = m_outputFiltersLeft[i].process(wetL);
        wetR = m_outputFiltersRight[i].process(wetR);
    }
    m_tailPeak = std::max({ m_tailPeak, std::abs(wetL), std::abs(wetR) });

    left = wetL;
    right = wetR;
}

void Reverb224::processSample(double & left, double & right)
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate) {
        updateHostRate();
    }
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
    renderFrame(left, right);
}

void Reverb224::processBlock(AudioContext & context)
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate) {
        updateHostRate();
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

void Reverb224::reset()
{
    m_input.reset();
    for (auto && allpass : m_diffusers) {
        allpass.reset();
    }
    for (size_t i = 0; i < m_halves.size(); i++) {
        m_halves[i].reset();
        // A quarter turn apart
        m_halves[i].lfoPhase = std::numbers::pi * 0.5 * static_cast<double>(i);
    }
    m_lowCut.reset();
    for (auto && filters : { std::ref(m_inputFilters), std::ref(m_outputFiltersLeft), std::ref(m_outputFiltersRight) }) {
        for (auto && filter : filters.get()) {
            filter.reset();
        }
    }
    m_inputHistory.fill(0.0);
    m_outputHistoryLeft.fill(0.0);
    m_outputHistoryRight.fill(0.0);
    m_inputPhase = 0.0;
    m_tailPeak = 0.0;
}

void Reverb224::sync()
{
    m_shouldSyncParameters = true;
}

bool Reverb224::isSettled() const
{
    return m_tailPeak < settledLevel;
}

const EffectPresetList & Reverb224::factoryPresets() const
{
    namespace C = Constants::NahdXml;
    const auto patch = [](Program program, double bass, double mid, double crossover, double treble, double preDelay, float depth, float diffusion, float enhance, float mix) {
        return std::map<std::string, float> {
            { key(C::xmlKeyProgram()), static_cast<float>(program) },
            { key(C::xmlKeyBassDecay()), decayPosition(bass) },
            { key(C::xmlKeyMidDecay()), decayPosition(mid) },
            { key(C::xmlKeyCrossoverFreq(0)), crossoverPosition(crossover) },
            { key(C::xmlKeyTrebleDecay()), treblePosition(treble) },
            { key(C::xmlKeyPreDelay()), preDelayPosition(preDelay) },
            { key(C::xmlKeyDepth()), depth },
            { key(C::xmlKeyDiffusion()), diffusion },
            { key(C::xmlKeyEnhance()), enhance },
            { key(C::xmlKeyMix()), mix },
        };
    };
    // Init first, as the way back; the rest alphabetical
    static const EffectPresetList presets {
        { "Init", {} },
        // The big, dark, endless hall of the late seventies: long lows, the top rolled off early
        { "Blade Runner", patch(Program::LargeHall, 7.0, 5.5, 600.0, 4500.0, 25.0, 0.55f, 0.85f, 0.6f, 0.4f) },
        // A small live chamber: quick, warm and a little boxy
        { "Chamber", patch(Program::Chamber, 1.6, 1.3, 800.0, 6000.0, 10.0, 0.4f, 0.8f, 0.4f, 0.3f) },
        // The classic hall, a little longer in the bass than in the mids
        { "Concert Hall", patch(Program::LargeHall, 2.8, 2.3, 700.0, 6500.0, 20.0, 0.5f, 0.8f, 0.5f, 0.3f) },
        // Smooth and even from the first moment, for pads and strings
        { "Dense Plate", patch(Program::ConstantDensityPlate, 2.2, 2.0, 600.0, 8000.0, 0.0, 0.5f, 1.0f, 0.4f, 0.3f) },
        // As long as the machine goes: a held chord becomes a drone
        { "Endless Hall", patch(Program::LargeHall, 70.0, 60.0, 500.0, 3500.0, 40.0, 0.7f, 0.9f, 0.7f, 0.35f) },
        // A short, tight room for drums and guitars
        { "Room", patch(Program::Room, 0.9, 0.7, 900.0, 7000.0, 2.0, 0.35f, 0.7f, 0.3f, 0.25f) },
        // The smaller hall, clearer in front
        { "Small Hall", patch(Program::SmallHallA, 1.8, 1.5, 700.0, 6500.0, 12.0, 0.45f, 0.8f, 0.5f, 0.3f) },
        // Bright and dense, short enough to sit behind a snare
        { "Snare Plate", patch(Program::PercussionPlate, 1.4, 1.2, 800.0, 9000.0, 5.0, 0.5f, 1.0f, 0.3f, 0.3f) },
        // A plate held off the voice by a long pre-delay, so the words stay in front
        { "Vocal Plate", patch(Program::VocalPlate, 2.0, 1.8, 700.0, 7500.0, 30.0, 0.5f, 0.9f, 0.4f, 0.25f) },
        // The second small hall: larger, grainier and darker
        { "Warm Hall", patch(Program::SmallHallB, 2.2, 1.8, 600.0, 5500.0, 15.0, 0.5f, 0.75f, 0.5f, 0.3f) },
    };
    return presets;
}

} // namespace noteahead
