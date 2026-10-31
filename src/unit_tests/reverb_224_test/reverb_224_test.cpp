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

#include "reverb_224_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/fft.hpp"
#include "../../domain/dsp/svf_filter.hpp"
#include "../../domain/effects/reverb_224.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;
using Program = Reverb224::Program;

void setParameter(Reverb224 & effect, const QString & key, float value)
{
    if (const auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

void setProgram(Reverb224 & effect, Program program)
{
    setParameter(effect, C::xmlKeyProgram(), static_cast<float>(program));
}

std::unique_ptr<Reverb224> makeReverb(double rate)
{
    auto effect = std::make_unique<Reverb224>();
    effect->setSampleRate(rate);
    // The tail on its own, from the first sample
    setParameter(*effect, C::xmlKeyPreDelay(), 0.0f);
    return effect;
}

//! The effect's wet output for @p input, which is what it hands the rack before the Mix blend.
std::vector<double> renderWet(Reverb224 & effect, std::vector<double> stereo, double rate)
{
    const auto frames = static_cast<uint32_t>(stereo.size() / 2);
    AudioContext context { std::span(stereo.data(), stereo.size()), frames, static_cast<uint32_t>(rate) };
    effect.processBlock(context);
    return stereo;
}

std::vector<double> impulse(size_t frames)
{
    std::vector<double> stereo(frames * 2, 0.0);
    stereo[0] = stereo[1] = 0.5;
    return stereo;
}

std::vector<double> sine(double hz, double amplitude, size_t frames, double rate)
{
    std::vector<double> stereo(frames * 2);
    for (size_t i = 0; i < frames; i++) {
        stereo[i * 2] = stereo[i * 2 + 1] = amplitude * std::sin(2.0 * M_PI * hz * static_cast<double>(i) / rate);
    }
    return stereo;
}

std::vector<double> noise(size_t frames, double deviation, unsigned seed)
{
    std::mt19937 generator { seed };
    std::normal_distribution<double> distribution { 0.0, deviation };
    std::vector<double> stereo(frames * 2);
    for (auto && sample : stereo) {
        sample = distribution(generator);
    }
    return stereo;
}

std::vector<double> monoOf(const std::vector<double> & stereo)
{
    std::vector<double> mono(stereo.size() / 2);
    for (size_t i = 0; i < mono.size(); i++) {
        mono[i] = stereo[i * 2] + stereo[i * 2 + 1];
    }
    return mono;
}

double energyOf(const std::vector<double> & stereo, size_t fromFrame, size_t toFrame = SIZE_MAX)
{
    double energy = 0.0;
    for (size_t i = fromFrame * 2; i < std::min(stereo.size(), toFrame * 2); i++) {
        energy += stereo[i] * stereo[i];
    }
    return energy;
}

double db(double ratio)
{
    return 10.0 * std::log10(ratio);
}

//! An octave band of @p stereo: two band-passes in series, so the bands either side stay out.
std::vector<double> octaveBand(const std::vector<double> & stereo, double hz, double rate)
{
    std::array<SvfFilter, 4> filters;
    for (auto && filter : filters) {
        filter.calculateBandPass(hz, rate, std::sqrt(2.0));
    }
    std::vector<double> band(stereo.size());
    for (size_t i = 0; i < stereo.size() / 2; i++) {
        band[i * 2] = filters[1].process(filters[0].process(stereo[i * 2]));
        band[i * 2 + 1] = filters[3].process(filters[2].process(stereo[i * 2 + 1]));
    }
    return band;
}

//! Decay time from the Schroeder integral of the summed energy, extrapolated from -5 to -25 dB.
double t60Seconds(const std::vector<double> & stereo, double rate)
{
    const size_t frames = stereo.size() / 2;
    std::vector<double> integral(frames);
    double sum = 0.0;
    for (size_t i = frames; i-- > 0;) {
        sum += stereo[i * 2] * stereo[i * 2] + stereo[i * 2 + 1] * stereo[i * 2 + 1];
        integral[i] = sum;
    }
    const auto dbAt = [&](size_t i) { return 10.0 * std::log10(integral[i] / integral[0]); };
    size_t start = 0;
    while (start < frames && dbAt(start) > -5.0) {
        start++;
    }
    size_t end = start;
    while (end < frames && dbAt(end) > -25.0) {
        end++;
    }
    return 3.0 * static_cast<double>(end - start) / rate;
}

//! The decay of an impulse in the octave band at @p hz.
double bandT60(Reverb224 & effect, double hz, double seconds, double rate)
{
    const auto out = renderWet(effect, impulse(static_cast<size_t>(rate * seconds)), rate);
    return t60Seconds(octaveBand(out, hz, rate), rate);
}

//! How noise-like a stretch is: the share of samples further than one deviation from zero, against
//! the share a Gaussian has. 1 is a dense, smooth tail; well below it is a train of echoes.
double echoDensity(const std::vector<double> & mono, size_t from, size_t length)
{
    double square = 0.0;
    for (size_t i = from; i < from + length; i++) {
        square += mono[i] * mono[i];
    }
    const double deviation = std::sqrt(square / static_cast<double>(length));
    size_t outside = 0;
    for (size_t i = from; i < from + length; i++) {
        outside += std::abs(mono[i]) > deviation ? 1 : 0;
    }
    return static_cast<double>(outside) / static_cast<double>(length) / std::erfc(1.0 / std::sqrt(2.0));
}

//! The converter's signal-to-noise ratio on a sine of @p amplitude.
double converterSnrDb(double amplitude)
{
    double signal = 0.0;
    double error = 0.0;
    for (int i = 0; i < 34130; i++) {
        const double x = amplitude * std::sin(2.0 * M_PI * 997.0 * i / 34130.0);
        const double y = Reverb224::convert(x);
        signal += x * x;
        error += (y - x) * (y - x);
    }
    return db(signal / error);
}

} // namespace

void Reverb224Test::test_convert_zero_shouldStayZero()
{
    QCOMPARE(Reverb224::convert(0.0), 0.0);
    // Clipped at full scale, as the converter is
    QVERIFY(Reverb224::convert(2.0) <= 1.0);
    QVERIFY(Reverb224::convert(-2.0) >= -1.0 - 1.0 / 2047.0);
}

void Reverb224Test::test_convert_quietSignal_shouldKeepItsResolution()
{
    // The gain ranger amplifies a quiet signal by up to 18 dB before it is converted: a plain
    // 12-bit converter loses 6 dB of SNR for every 6 dB the signal drops, this one only past that
    const double loud = converterSnrDb(0.9);
    const double quiet = converterSnrDb(0.09);
    QVERIFY2(loud > 65.0, qPrintable(QString::number(loud)));
    QVERIFY2(loud - quiet < 5.0, qPrintable(QString { "%1 dB against %2 dB" }.arg(loud).arg(quiet)));
    // And the noise still follows the signal down, which is the converter's sound
    const double quietest = converterSnrDb(0.005);
    QVERIFY2(quietest < loud - 15.0, qPrintable(QString::number(quietest)));
}

void Reverb224Test::test_process_mixZero_shouldLeaveTheSignalAlone()
{
    auto effect = makeReverb(48000.0);
    setParameter(*effect, C::xmlKeyMix(), 0.0f);
    const auto input = noise(4800, 0.1, 1);
    auto output = input;
    AudioContext context { std::span(output.data(), output.size()), 4800, 48000 };
    effect->process(context);
    QCOMPARE(output, input);
}

void Reverb224Test::test_processBlock_beforeThePreDelay_shouldBeSilent()
{
    const double rate = 48000.0;
    auto effect = makeReverb(rate);
    setParameter(*effect, C::xmlKeyPreDelay(), Reverb224::preDelayPosition(30.0));
    const auto out = renderWet(*effect, impulse(48000), rate);
    const auto preDelayFrames = static_cast<size_t>(0.029 * rate);
    QCOMPARE(energyOf(out, 0, preDelayFrames), 0.0);
    QVERIFY(energyOf(out, preDelayFrames) > 0.0);
}

void Reverb224Test::test_processBlock_decay_shouldMatchTheSettingInEveryBand()
{
    // Bass and mid set alike and the treble cutoff out of the way: the same decay in every band of
    // every program. Below the treble cutoff only, as even at its top the cutoff shortens 4 kHz.
    const double rate = 48000.0;
    for (size_t program = 0; program < Reverb224::ProgramCount; program++) {
        for (const double decay : { 2.0, 5.0 }) {
            auto effect = makeReverb(rate);
            setProgram(*effect, static_cast<Program>(program));
            setParameter(*effect, C::xmlKeyBassDecay(), Reverb224::decayPosition(decay));
            setParameter(*effect, C::xmlKeyMidDecay(), Reverb224::decayPosition(decay));
            setParameter(*effect, C::xmlKeyTrebleDecay(), 1.0f);
            setParameter(*effect, C::xmlKeyDepth(), 1.0f);
            setParameter(*effect, C::xmlKeyConverters(), 0.0f);
            const auto response = renderWet(*effect, impulse(static_cast<size_t>(rate * (decay * 1.5 + 0.5))), rate);
            double sum = 0.0;
            const std::vector<double> bands { 125.0, 250.0, 500.0, 1000.0, 2000.0 };
            for (const double hz : bands) {
                const auto ratio = t60Seconds(octaveBand(response, hz, rate), rate) / decay;
                QVERIFY2(std::abs(ratio - 1.0) < 0.1, qPrintable(QString { "program %1, %2 s at %3 Hz: %4" }.arg(program).arg(decay).arg(hz).arg(ratio)));
                sum += ratio;
            }
            const double mean = sum / static_cast<double>(bands.size());
            QVERIFY2(std::abs(mean - 1.0) < 0.05, qPrintable(QString { "program %1, %2 s: %3" }.arg(program).arg(decay).arg(mean)));
        }
    }
}

void Reverb224Test::test_processBlock_decay_bassAndMid_shouldDecayApart()
{
    const double rate = 48000.0;
    const auto measure = [rate](double hz) {
        auto effect = makeReverb(rate);
        setProgram(*effect, Program::SmallHallA);
        setParameter(*effect, C::xmlKeyBassDecay(), Reverb224::decayPosition(6.0));
        setParameter(*effect, C::xmlKeyMidDecay(), Reverb224::decayPosition(1.5));
        setParameter(*effect, C::xmlKeyCrossoverFreq(0), Reverb224::crossoverPosition(250.0));
        setParameter(*effect, C::xmlKeyConverters(), 0.0f);
        setParameter(*effect, C::xmlKeyTrebleDecay(), 1.0f);
        setParameter(*effect, C::xmlKeyDepth(), 1.0f);
        return bandT60(*effect, hz, 8.0, rate);
    };
    const double bass = measure(63.0);
    const double mid = measure(2000.0);
    // Two octaves either side of the crossover, each band has its own decay
    QVERIFY2(std::abs(bass / 6.0 - 1.0) < 0.1, qPrintable(QString::number(bass)));
    QVERIFY2(std::abs(mid / 1.5 - 1.0) < 0.1, qPrintable(QString::number(mid)));
}

void Reverb224Test::test_processBlock_trebleDecay_shouldShortenTheTop()
{
    const double rate = 48000.0;
    const auto measure = [rate](double trebleHz, double hz) {
        auto effect = makeReverb(rate);
        setParameter(*effect, C::xmlKeyBassDecay(), Reverb224::decayPosition(3.0));
        setParameter(*effect, C::xmlKeyMidDecay(), Reverb224::decayPosition(3.0));
        setParameter(*effect, C::xmlKeyTrebleDecay(), Reverb224::treblePosition(trebleHz));
        setParameter(*effect, C::xmlKeyDepth(), 1.0f);
        setParameter(*effect, C::xmlKeyConverters(), 0.0f);
        return bandT60(*effect, hz, 5.0, rate);
    };
    const double open = measure(10900.0, 4000.0);
    const double dark = measure(1000.0, 4000.0);
    QVERIFY2(dark < open * 0.6, qPrintable(QString { "%1 s against %2 s" }.arg(dark).arg(open)));
    // Well below the cutoff the decay is the one set
    const double below = measure(1000.0, 125.0);
    QVERIFY2(std::abs(below / 3.0 - 1.0) < 0.1, qPrintable(QString::number(below)));
}

void Reverb224Test::test_processBlock_bandwidth_shouldStopAboveEightKilohertz()
{
    const double rate = 48000.0;
    const auto wetEnergy = [rate](double hz) {
        auto effect = makeReverb(rate);
        setParameter(*effect, C::xmlKeyConverters(), 0.0f);
        return energyOf(renderWet(*effect, sine(hz, 0.3, 48000, rate), rate), 24000);
    };
    const double ratio = db(wetEnergy(14000.0) / wetEnergy(2000.0));
    QVERIFY2(ratio < -40.0, qPrintable(QString::number(ratio)));
}

void Reverb224Test::test_processBlock_converters_shouldKeepTheLevel()
{
    const double rate = 48000.0;
    const auto render = [rate](bool converters) {
        auto effect = makeReverb(rate);
        setParameter(*effect, C::xmlKeyConverters(), converters ? 1.0f : 0.0f);
        return renderWet(*effect, noise(96000, 0.05, 7), rate);
    };
    const auto with = render(true);
    const auto without = render(false);
    QVERIFY(with != without);
    QVERIFY2(std::abs(db(energyOf(with, 48000) / energyOf(without, 48000))) < 0.2, qPrintable(QString::number(db(energyOf(with, 48000) / energyOf(without, 48000)))));
}

void Reverb224Test::test_processBlock_depth_plates_shouldChangeNothing()
{
    const double rate = 48000.0;
    for (const auto program : { Program::VocalPlate, Program::PercussionPlate, Program::ConstantDensityPlate }) {
        QVERIFY(!Reverb224::hasEarlyReflections(program));
        const auto render = [rate, program](float depth) {
            auto effect = makeReverb(rate);
            setProgram(*effect, program);
            setParameter(*effect, C::xmlKeyDepth(), depth);
            return renderWet(*effect, impulse(24000), rate);
        };
        QCOMPARE(render(0.0f), render(1.0f));
    }
}

void Reverb224Test::test_processBlock_depth_halls_shouldBalanceTheEarlyReflections()
{
    // Near, the early reflections come first and loud; deep, the tank takes over
    const double rate = 48000.0;
    for (const auto program : { Program::SmallHallA, Program::SmallHallB, Program::LargeHall, Program::Chamber, Program::Room }) {
        QVERIFY(Reverb224::hasEarlyReflections(program));
        const auto earlyShare = [rate, program](float depth) {
            auto effect = makeReverb(rate);
            setProgram(*effect, program);
            setParameter(*effect, C::xmlKeyDepth(), depth);
            const auto out = renderWet(*effect, impulse(48000), rate);
            return db(energyOf(out, 0, 4800) / energyOf(out, 4800));
        };
        const double near = earlyShare(0.0f);
        const double deep = earlyShare(1.0f);
        QVERIFY2(near > deep + 6.0, qPrintable(QString { "program %1: %2 dB against %3 dB" }.arg(static_cast<int>(program)).arg(near).arg(deep)));
    }
}

void Reverb224Test::test_processBlock_programs_shouldAllDiffer()
{
    const double rate = 48000.0;
    std::vector<std::vector<double>> responses;
    for (size_t i = 0; i < Reverb224::ProgramCount; i++) {
        auto effect = makeReverb(rate);
        setProgram(*effect, static_cast<Program>(i));
        responses.push_back(renderWet(*effect, impulse(24000), rate));
    }
    for (size_t i = 0; i < responses.size(); i++) {
        for (size_t j = i + 1; j < responses.size(); j++) {
            QVERIFY2(responses[i] != responses[j], qPrintable(QString { "%1 and %2" }.arg(i).arg(j)));
        }
    }
}

void Reverb224Test::test_processBlock_tail_shouldBeDense()
{
    // No distinct echoes once the tank has built up: as noise-like as noise, window after window
    const double rate = 48000.0;
    for (size_t i = 0; i < Reverb224::ProgramCount; i++) {
        auto effect = makeReverb(rate);
        setProgram(*effect, static_cast<Program>(i));
        setParameter(*effect, C::xmlKeyDepth(), 1.0f);
        setParameter(*effect, C::xmlKeyConverters(), 0.0f);
        const auto mono = monoOf(renderWet(*effect, impulse(96000), rate));
        for (const double ms : { 150.0, 300.0, 600.0 }) {
            const auto density = echoDensity(mono, static_cast<size_t>(ms * rate / 1000.0), static_cast<size_t>(0.05 * rate));
            QVERIFY2(density > 0.8, qPrintable(QString { "program %1 at %2 ms: %3" }.arg(i).arg(ms).arg(density)));
        }
    }
}

void Reverb224Test::test_processBlock_longestSettings_shouldStayStable()
{
    // Everything at its top, ten seconds of noise in: nothing may grow, and nothing may stop
    // being a number
    const double rate = 48000.0;
    for (size_t i = 0; i < Reverb224::ProgramCount; i++) {
        auto effect = makeReverb(rate);
        setProgram(*effect, static_cast<Program>(i));
        for (auto && key : { C::xmlKeyBassDecay(), C::xmlKeyMidDecay(), C::xmlKeyTrebleDecay(), C::xmlKeyDiffusion(), C::xmlKeyEnhance(), C::xmlKeyPreDelay() }) {
            setParameter(*effect, key, 1.0f);
        }
        double peak = 0.0;
        for (int second = 0; second < 10; second++) {
            const auto out = renderWet(*effect, noise(48000, 0.1, static_cast<unsigned>(second)), rate);
            for (const double sample : out) {
                QVERIFY(std::isfinite(sample));
                peak = std::max(peak, std::abs(sample));
            }
        }
        QVERIFY2(peak < 4.0, qPrintable(QString { "program %1: %2" }.arg(i).arg(peak)));
    }
}

void Reverb224Test::test_processBlock_44100And96000_shouldSoundTheSame()
{
    // The machine runs at its own rate whatever the host's, so the decay, the level and the tone all
    // have to agree
    struct Reading
    {
        double t60;
        double levelDb;
        double centroidHz;
    };

    const auto measure = [](double rate) {
        auto effect = makeReverb(rate);
        setParameter(*effect, C::xmlKeyConverters(), 0.0f);
        const auto out = renderWet(*effect, impulse(static_cast<size_t>(rate * 4.0)), rate);
        const auto mono = monoOf(out);
        constexpr int n = 131072;
        std::vector<double> padded(n, 0.0);
        std::copy_n(mono.begin(), std::min<size_t>(n, mono.size()), padded.begin());
        std::vector<double> re(n / 2 + 1);
        std::vector<double> im(n / 2 + 1);
        Fft::forwardReal(padded.data(), re.data(), im.data(), n);
        double weighted = 0.0;
        double total = 0.0;
        for (int i = 1; i <= n / 2; i++) {
            const double hz = i * rate / n;
            const double power = re[i] * re[i] + im[i] * im[i];
            weighted += hz * power;
            total += power;
        }
        // Against a tone rather than noise: white noise at 96 kHz puts less of its energy under
        // the machine's band limit than it does at 44.1 kHz
        auto levelEffect = makeReverb(rate);
        const auto input = sine(1000.0, 0.1, static_cast<size_t>(rate * 2.0), rate);
        const auto wet = renderWet(*levelEffect, input, rate);
        const auto settled = static_cast<size_t>(rate);
        return Reading { t60Seconds(out, rate), db(energyOf(wet, settled) / energyOf(input, settled)), weighted / total };
    };
    const auto at44 = measure(44100.0);
    const auto at96 = measure(96000.0);
    QVERIFY2(std::abs(at44.t60 / at96.t60 - 1.0) < 0.03, qPrintable(QString { "%1 s against %2 s" }.arg(at44.t60).arg(at96.t60)));
    QVERIFY2(std::abs(at44.levelDb - at96.levelDb) < 0.5, qPrintable(QString { "%1 dB against %2 dB" }.arg(at44.levelDb).arg(at96.levelDb)));
    QVERIFY2(std::abs(at44.centroidHz / at96.centroidHz - 1.0) < 0.05, qPrintable(QString { "%1 Hz against %2 Hz" }.arg(at44.centroidHz).arg(at96.centroidHz)));
}

void Reverb224Test::test_processBlock_rateChange_shouldStartTheNewStreamClean()
{
    auto effect = makeReverb(48000.0);
    renderWet(*effect, noise(4800, 0.3, 3), 48000.0);
    QVERIFY(!effect->isSettled());
    for (const double rate : { 44100.0, 48000.0 }) {
        effect->setSampleRate(rate);
        const auto out = renderWet(*effect, std::vector<double>(2048, 0.0), rate);
        QCOMPARE(energyOf(out, 0), 0.0);
        QVERIFY(effect->isSettled());
        const auto wet = renderWet(*effect, impulse(static_cast<size_t>(rate)), rate);
        QVERIFY(energyOf(wet, 0) > 0.0);
    }
}

void Reverb224Test::test_isSettled_shouldWaitForTheTail()
{
    const double rate = 48000.0;
    auto effect = makeReverb(rate);
    setParameter(*effect, C::xmlKeyBassDecay(), Reverb224::decayPosition(1.0));
    setParameter(*effect, C::xmlKeyMidDecay(), Reverb224::decayPosition(1.0));
    QVERIFY(effect->isSettled());
    renderWet(*effect, impulse(512), rate);
    QVERIFY(!effect->isSettled());
    // Well inside the decay, and then long past it
    for (int block = 0; block < 20; block++) {
        renderWet(*effect, std::vector<double>(1024, 0.0), rate);
    }
    QVERIFY(!effect->isSettled());
    for (int block = 0; block < 500; block++) {
        renderWet(*effect, std::vector<double>(1024, 0.0), rate);
    }
    QVERIFY(effect->isSettled());
}

void Reverb224Test::test_reset_shouldRenderIdentically()
{
    const double rate = 48000.0;
    auto effect = makeReverb(rate);
    const auto first = renderWet(*effect, noise(24000, 0.1, 9), rate);
    effect->reset();
    QCOMPARE(renderWet(*effect, noise(24000, 0.1, 9), rate), first);
}

void Reverb224Test::test_factoryPresets_shouldNameRealParameters()
{
    auto effect = makeReverb(48000.0);
    const auto & presets = effect->factoryPresets();
    QVERIFY(presets.size() > 1);
    QCOMPARE(presets.front().name, std::string { "Init" });
    for (auto && preset : presets) {
        for (auto && [key, value] : preset.parameters) {
            const auto parameter = effect->parameter(key);
            QVERIFY2(parameter.has_value(), qPrintable(QString::fromStdString(preset.name + ": " + key)));
            const float top = parameter->get().isDiscrete() ? static_cast<float>(parameter->get().xmlMax()) : 1.0f;
            QVERIFY2(value >= 0.0f && value <= top, qPrintable(QString::fromStdString(preset.name + ": " + key)));
        }
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::Reverb224Test)
