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

#include "ambience_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/fft.hpp"
#include "../../domain/effects/ambience.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

void setParameter(Ambience & effect, const QString & key, float value)
{
    if (const auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

std::unique_ptr<Ambience> makeAmbience(double rate)
{
    auto effect = std::make_unique<Ambience>();
    effect->setSampleRate(rate);
    return effect;
}

//! The effect's wet output for @p input, which is what it hands the rack before the Mix blend.
std::vector<double> renderWet(Ambience & effect, std::vector<double> stereo, double rate)
{
    const auto frames = static_cast<uint32_t>(stereo.size() / 2);
    AudioContext context { std::span(stereo.data(), stereo.size()), frames, static_cast<uint32_t>(rate) };
    effect.processBlock(context);
    return stereo;
}

std::vector<double> impulse(size_t frames)
{
    std::vector<double> stereo(frames * 2, 0.0);
    stereo[0] = stereo[1] = 1.0;
    return stereo;
}

//! Decay time from the Schroeder integral of the summed energy, extrapolated from -5 to -25 dB.
double t60Seconds(const std::vector<double> & stereo, double rate)
{
    const size_t frames = stereo.size() / 2;
    std::vector<double> energy(frames);
    for (size_t i = 0; i < frames; i++) {
        energy[i] = stereo[i * 2] * stereo[i * 2] + stereo[i * 2 + 1] * stereo[i * 2 + 1];
    }
    std::vector<double> integral(frames);
    double sum = 0.0;
    for (size_t i = frames; i-- > 0;) {
        sum += energy[i];
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

//! How peaky the spectrum of a stretch is against its own local average: about 2 for noise, and
//! higher for a tail that rings at a handful of frequencies.
double spectralPeakiness(const std::vector<double> & mono, size_t from, double rate)
{
    constexpr int n = 16384;
    std::vector<double> windowed(n);
    for (int i = 0; i < n; i++) {
        const double hann = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (n - 1));
        windowed[i] = mono[from + static_cast<size_t>(i)] * hann;
    }
    std::vector<double> re(n / 2 + 1);
    std::vector<double> im(n / 2 + 1);
    Fft::forwardReal(windowed.data(), re.data(), im.data(), n);
    std::vector<double> power(n / 2 + 1);
    for (int i = 0; i <= n / 2; i++) {
        power[i] = re[i] * re[i] + im[i] * im[i];
    }
    const auto bin = [&](double hz) { return static_cast<int>(hz * n / rate); };
    double sum = 0.0;
    int count = 0;
    for (int i = bin(200.0); i < bin(4000.0); i++) {
        const int half = std::max(8, i / 12);
        double local = 0.0;
        for (int j = i - half; j <= i + half; j++) {
            local += power[j];
        }
        local /= 2 * half + 1;
        const double ratio = power[i] / local;
        sum += ratio * ratio;
        count++;
    }
    return sum / count;
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

double energyOf(const std::vector<double> & stereo, size_t fromFrame)
{
    double energy = 0.0;
    for (size_t i = fromFrame * 2; i < stereo.size(); i++) {
        energy += stereo[i] * stereo[i];
    }
    return energy;
}

double db(double ratio)
{
    return 10.0 * std::log10(ratio);
}

} // namespace

void AmbienceTest::test_process_mixZero_shouldLeaveTheSignalAlone()
{
    auto effect = makeAmbience(48000.0);
    setParameter(*effect, C::xmlKeyMix(), 0.0f);
    const auto input = noise(4800, 0.1, 1);
    auto output = input;
    AudioContext context { std::span(output.data(), output.size()), 4800, 48000 };
    effect->process(context);
    QCOMPARE(output, input);
}

void AmbienceTest::test_processBlock_beforeThePreDelay_shouldBeSilent()
{
    const double rate = 48000.0;
    auto effect = makeAmbience(rate);
    setParameter(*effect, C::xmlKeyPreDelay(), Ambience::preDelayPosition(30.0));
    const auto out = renderWet(*effect, impulse(48000), rate);
    const auto preDelayFrames = static_cast<size_t>(0.030 * rate);
    for (size_t i = 0; i < preDelayFrames * 2; i++) {
        QCOMPARE(out[i], 0.0);
    }
    QVERIFY(energyOf(out, preDelayFrames) > 0.0);
}

void AmbienceTest::test_processBlock_decay_shouldMatchTheSettingAtEveryRate()
{
    // Measured with the lines drifting at their furthest, which is what a linear read lost 13 % of
    // every decay time to
    for (const double rate : { 44100.0, 96000.0 }) {
        for (const double decay : { 0.5, 1.0, 2.0 }) {
            auto effect = makeAmbience(rate);
            setParameter(*effect, C::xmlKeyDecay(), Ambience::decayPosition(decay));
            setParameter(*effect, C::xmlKeyDepth(), 1.0f);
            setParameter(*effect, C::xmlKeyDamping(), 0.0f);
            setParameter(*effect, C::xmlKeyEarlyLate(), 1.0f);
            const auto measured = t60Seconds(renderWet(*effect, impulse(static_cast<size_t>(rate * 4.0)), rate), rate);
            QVERIFY2(std::abs(measured / decay - 1.0) < 0.05, qPrintable(QString { "%1 Hz, %2 s: %3 s" }.arg(rate).arg(decay).arg(measured)));
        }
    }
}

void AmbienceTest::test_processBlock_tail_shouldBeDenseAfterTwentyMilliseconds()
{
    // No distinct echoes once the space has built up: as noise-like as noise, window after window
    const double rate = 48000.0;
    auto effect = makeAmbience(rate);
    setParameter(*effect, C::xmlKeyPreDelay(), 0.0f);
    const auto mono = monoOf(renderWet(*effect, impulse(24000), rate));
    for (const double ms : { 20.0, 40.0, 60.0, 100.0, 150.0 }) {
        const auto density = echoDensity(mono, static_cast<size_t>(ms * rate / 1000.0), static_cast<size_t>(0.02 * rate));
        QVERIFY2(density > 0.85, qPrintable(QString { "%1 ms: %2" }.arg(ms).arg(density)));
    }
}

void AmbienceTest::test_processBlock_modulation_shouldBreakUpTheRinging()
{
    const double rate = 48000.0;
    const auto peakiness = [rate](float depth) {
        auto effect = makeAmbience(rate);
        setParameter(*effect, C::xmlKeyDepth(), depth);
        setParameter(*effect, C::xmlKeyDecay(), Ambience::decayPosition(1.5));
        return spectralPeakiness(monoOf(renderWet(*effect, impulse(72000), rate)), 24000, rate);
    };
    const auto still = peakiness(0.0f);
    const auto drifting = peakiness(1.0f);
    QVERIFY2(drifting < still * 0.9, qPrintable(QString { "%1 against %2" }.arg(drifting).arg(still)));
}

void AmbienceTest::test_processBlock_stereo_shouldBeWideButMonoSafe()
{
    const double rate = 48000.0;
    auto effect = makeAmbience(rate);
    const auto out = renderWet(*effect, noise(192000, 0.1, 2), rate);
    double lr = 0.0;
    double ll = 0.0;
    double rr = 0.0;
    double mono = 0.0;
    for (size_t i = out.size() / 4; i < out.size() / 2; i++) {
        lr += out[i * 2] * out[i * 2 + 1];
        ll += out[i * 2] * out[i * 2];
        rr += out[i * 2 + 1] * out[i * 2 + 1];
        mono += 0.5 * (out[i * 2] + out[i * 2 + 1]) * (out[i * 2] + out[i * 2 + 1]);
    }
    const double correlation = lr / std::sqrt(ll * rr);
    QVERIFY2(correlation > 0.05 && correlation < 0.5, qPrintable(QString::number(correlation)));
    QVERIFY2(db(mono / (ll + rr)) > -3.0, qPrintable(QString::number(db(mono / (ll + rr)))));
}

void AmbienceTest::test_processBlock_lowCut_shouldTakeTheLowsOutOfTheSpace()
{
    const double rate = 48000.0;
    const auto wetEnergy = [rate](double lowCut) {
        auto effect = makeAmbience(rate);
        setParameter(*effect, C::xmlKeyHpfCutoff(), Ambience::lowCutPosition(lowCut));
        return energyOf(renderWet(*effect, sine(60.0, 0.5, 48000, rate), rate), 24000);
    };
    QVERIFY2(db(wetEnergy(800.0) / wetEnergy(20.0)) < -15.0, qPrintable(QString::number(db(wetEnergy(800.0) / wetEnergy(20.0)))));

    // The dry is not the space's: it passes whole whatever the space does
    auto effect = makeAmbience(rate);
    setParameter(*effect, C::xmlKeyHpfCutoff(), Ambience::lowCutPosition(1000.0));
    setParameter(*effect, C::xmlKeyMix(), 1.0f);
    const auto input = sine(60.0, 0.5, 48000, rate);
    auto output = input;
    AudioContext context { std::span(output.data(), output.size()), 48000, 48000 };
    effect->process(context);
    QVERIFY(std::abs(db(energyOf(output, 24000) / energyOf(input, 24000))) < 0.5);
}

void AmbienceTest::test_processBlock_highCut_shouldTakeTheTopOutOfTheSpace()
{
    const double rate = 48000.0;
    const auto wetEnergy = [rate](double highCut) {
        auto effect = makeAmbience(rate);
        setParameter(*effect, C::xmlKeyDamping(), 0.0f);
        setParameter(*effect, C::xmlKeyLpfCutoff(), Ambience::highCutPosition(highCut));
        return energyOf(renderWet(*effect, sine(8000.0, 0.5, 48000, rate), rate), 24000);
    };
    QVERIFY2(db(wetEnergy(1000.0) / wetEnergy(20000.0)) < -15.0, qPrintable(QString::number(db(wetEnergy(1000.0) / wetEnergy(20000.0)))));
}

void AmbienceTest::test_processBlock_longestSettings_shouldStayStable()
{
    // Everything at its top, a minute of noise in: nothing may grow, and nothing may stop being a number
    const double rate = 48000.0;
    auto effect = makeAmbience(rate);
    for (auto && key : { C::xmlKeySize(), C::xmlKeyDecay(), C::xmlKeyDiffusion(), C::xmlKeyDepth(), C::xmlKeyRate(), C::xmlKeyWidth(), C::xmlKeyShape() }) {
        setParameter(*effect, key, 1.0f);
    }
    setParameter(*effect, C::xmlKeyDamping(), 0.0f);
    double peak = 0.0;
    for (int second = 0; second < 60; second++) {
        const auto out = renderWet(*effect, noise(48000, 0.1, static_cast<unsigned>(second)), rate);
        for (const double sample : out) {
            QVERIFY(std::isfinite(sample));
            peak = std::max(peak, std::abs(sample));
        }
    }
    QVERIFY2(peak < 4.0, qPrintable(QString::number(peak)));
}

void AmbienceTest::test_processBlock_44100And48000_shouldSoundTheSame()
{
    // Rendering at one rate and playing at the other must not change the space: every length is in
    // milliseconds, so the decay, the level and the tone all have to agree between the two
    struct Reading
    {
        double t60;
        double levelDb;
        double centroidHz;
    };

    const auto measure = [](double rate) {
        auto effect = makeAmbience(rate);
        const auto frames = static_cast<size_t>(rate * 3.0);
        const auto out = renderWet(*effect, impulse(frames), rate);
        const auto mono = monoOf(out);
        // The tone, as the centroid of the whole response
        constexpr int n = 65536;
        std::vector<double> padded(n, 0.0);
        std::copy_n(mono.begin(), std::min<size_t>(n, mono.size()), padded.begin());
        std::vector<double> re(n / 2 + 1);
        std::vector<double> im(n / 2 + 1);
        Fft::forwardReal(padded.data(), re.data(), im.data(), n);
        double weighted = 0.0;
        double total = 0.0;
        for (int i = 1; i <= n / 2; i++) {
            const double hz = i * rate / n;
            if (hz > 20000.0) {
                break;
            }
            const double power = re[i] * re[i] + im[i] * im[i];
            weighted += hz * power;
            total += power;
        }
        // The level against the same noise, once the space has filled up: an impulse response's
        // energy says little here, as one sample of impulse is a different signal at each rate
        auto levelEffect = makeAmbience(rate);
        const auto input = noise(static_cast<size_t>(rate * 2.0), 0.1, 5);
        const auto wet = renderWet(*levelEffect, input, rate);
        const auto settled = static_cast<size_t>(rate);
        return Reading { t60Seconds(out, rate), db(energyOf(wet, settled) / energyOf(input, settled)), weighted / total };
    };
    const auto at44 = measure(44100.0);
    const auto at48 = measure(48000.0);
    QVERIFY2(std::abs(at44.t60 / at48.t60 - 1.0) < 0.05, qPrintable(QString { "%1 s against %2 s" }.arg(at44.t60).arg(at48.t60)));
    QVERIFY2(std::abs(at44.levelDb - at48.levelDb) < 0.5, qPrintable(QString { "%1 dB against %2 dB" }.arg(at44.levelDb).arg(at48.levelDb)));
    QVERIFY2(std::abs(at44.centroidHz / at48.centroidHz - 1.0) < 0.05, qPrintable(QString { "%1 Hz against %2 Hz" }.arg(at44.centroidHz).arg(at48.centroidHz)));
}

void AmbienceTest::test_processBlock_rateChange_shouldStartTheNewStreamClean()
{
    // Played at 48 kHz, then rendered at 44.1 kHz and back: the tail of one stream must not leak
    // into the next, wherever a buffer happened to keep its size
    auto effect = makeAmbience(48000.0);
    setParameter(*effect, C::xmlKeyDecay(), 1.0f);
    renderWet(*effect, noise(4800, 0.3, 3), 48000.0);
    QVERIFY(!effect->isSettled());

    for (const double rate : { 44100.0, 48000.0 }) {
        effect->setSampleRate(rate);
        const auto out = renderWet(*effect, std::vector<double>(2048, 0.0), rate);
        QCOMPARE(energyOf(out, 0), 0.0);
        QVERIFY(effect->isSettled());
        // And it still works at the new rate
        const auto wet = renderWet(*effect, impulse(static_cast<size_t>(rate)), rate);
        QVERIFY(energyOf(wet, 0) > 0.0);
        for (const double sample : wet) {
            QVERIFY(std::isfinite(sample));
        }
    }
}

void AmbienceTest::test_processBlock_highCutNearNyquist_shouldStayStableAt44100()
{
    // At 44.1 kHz the top of High Cut is within a few kilohertz of Nyquist, where a filter design
    // that is fine at 48 kHz can blow up
    const double rate = 44100.0;
    for (const float position : { 0.95f, 0.98f, 0.995f }) {
        auto effect = makeAmbience(rate);
        setParameter(*effect, C::xmlKeyLpfCutoff(), position);
        setParameter(*effect, C::xmlKeyDamping(), 0.0f);
        double peak = 0.0;
        for (const double sample : renderWet(*effect, noise(44100, 0.2, 4), rate)) {
            QVERIFY(std::isfinite(sample));
            peak = std::max(peak, std::abs(sample));
        }
        QVERIFY2(peak < 4.0, qPrintable(QString { "%1: %2" }.arg(position).arg(peak)));
    }
}

void AmbienceTest::test_isSettled_shouldWaitForTheTail()
{
    const double rate = 48000.0;
    auto effect = makeAmbience(rate);
    QVERIFY(effect->isSettled());
    renderWet(*effect, impulse(512), rate);
    QVERIFY(!effect->isSettled());
    // Well inside the default decay of 0.8 s, and then long past it
    for (int block = 0; block < 28; block++) {
        renderWet(*effect, std::vector<double>(1024, 0.0), rate);
    }
    QVERIFY(!effect->isSettled());
    for (int block = 0; block < 300; block++) {
        renderWet(*effect, std::vector<double>(1024, 0.0), rate);
    }
    QVERIFY(effect->isSettled());
}

void AmbienceTest::test_reset_shouldSilenceTheTail()
{
    const double rate = 48000.0;
    auto effect = makeAmbience(rate);
    renderWet(*effect, impulse(512), rate);
    effect->reset();
    QVERIFY(effect->isSettled());
    QCOMPARE(energyOf(renderWet(*effect, std::vector<double>(9600, 0.0), rate), 0), 0.0);
}

void AmbienceTest::test_factoryPresets_shouldNameRealParameters()
{
    auto effect = makeAmbience(48000.0);
    const auto & presets = effect->factoryPresets();
    QVERIFY(presets.size() > 1);
    QCOMPARE(presets.front().name, std::string { "Init" });
    for (auto && preset : presets) {
        for (auto && [key, value] : preset.parameters) {
            QVERIFY2(effect->parameter(key).has_value(), qPrintable(QString::fromStdString(preset.name + ": " + key)));
            QVERIFY2(value >= 0.0f && value <= 1.0f, qPrintable(QString::fromStdString(preset.name + ": " + key)));
        }
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AmbienceTest)
