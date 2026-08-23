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

#include "effects_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/cascaded_svf.hpp"
#include "../../domain/dsp/high_pass_filter.hpp"
#include "../../domain/dsp/low_pass_filter.hpp"
#include "../../domain/dsp/panning.hpp"
#include "../../domain/dsp/volume.hpp"
#include "../../domain/effects/chorus.hpp"
#include "../../domain/effects/clipper.hpp"
#include "../../domain/effects/delay.hpp"
#include "../../domain/effects/endless_reverb.hpp"
#include "../../domain/effects/eq_8_band_parametric.hpp"
#include "../../domain/effects/limiter.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../domain/effects/saturator.hpp"

#include <QTest>

#include <cmath>
#include <numbers>

namespace noteahead {

void EffectsTest::test_volumeEffect_shouldApplyGainToSignal()
{
    Volume effect;
    double left = 1.0;
    double right = 1.0;
    effect.setVolume(0.5f);
    effect.process(left, right);

    QCOMPARE(left, 0.5f);
    QCOMPARE(right, 0.5f);

    effect.setVolume(0.0f);
    left = 1.0f;
    right = 1.0f;
    effect.process(left, right);
    QCOMPARE(left, 0.0f);
    QCOMPARE(right, 0.0f);
}

void EffectsTest::test_panningEffect_shouldDistributeSignalToChannels()
{
    Panning effect;

    // Center: constant-power pan gives cos(π/4) on both channels
    {
        double left = 1.0;
        double right = 1.0;
        effect.setPan(0.5f);
        effect.process(left, right);
        const double angle = static_cast<double>(0.5f) * std::numbers::pi * 0.5;
        QCOMPARE(left, std::cos(angle));
        QCOMPARE(right, std::sin(angle));
    }

    // Full Left
    {
        double left = 1.0;
        double right = 1.0;
        effect.setPan(0.0f);
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 0.0f);
    }

    // Full Right
    {
        double left = 1.0;
        double right = 1.0;
        effect.setPan(1.0f);
        effect.process(left, right);
        QCOMPARE(left, 0.0f);
        QCOMPARE(right, 1.0f);
    }
}

void EffectsTest::test_lowPassFilterEffect_shouldProcessAudioStablely()
{
    LowPassFilter effect;
    double left = 1.0;
    double right = 1.0;
    // Cutoff 1.0 (bypass)
    effect.setCutoff(1.0f);
    effect.process(left, right);
    QCOMPARE(left, 1.0f);
    QCOMPARE(right, 1.0f);

    // Filter processing stability
    effect.setCutoff(0.5f);
    for (int i = 0; i < 100; ++i) {
        left = 1.0f;
        right = 1.0f;
        effect.process(left, right);
        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }
}

void EffectsTest::test_highPassFilterEffect_shouldProcessAudioStablely()
{
    HighPassFilter effect;

    // Cutoff 0.0 (bypass)
    {
        double left = 1.0;
        double right = 1.0;
        effect.setCutoff(0.0f);
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 1.0f);
    }

    // Stability and NaN check
    effect.setCutoff(0.5f);
    for (int i = 0; i < 100; ++i) {
        double left = 1.0;
        double right = 1.0;
        effect.process(left, right);
        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }

    // Extreme cutoff stability
    effect.setCutoff(0.99f);
    for (int i = 0; i < 100; ++i) {
        double left = 1.0;
        double right = 1.0;
        effect.process(left, right);
        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }
}

namespace {

//! An effect that replaces the signal with a constant, so what the Mix law does with it is obvious.
class ConstantEffect : public Effect
{
public:
    explicit ConstantEffect(float mixDefault, MixLaw law)
    {
        addMixParameter(mixDefault, law);
    }

    std::string type() const override
    {
        return "constant";
    }

    std::string typeId() const override
    {
        return "constant";
    }

    void setMix(float value)
    {
        if (auto parameter = this->parameter(Constants::NahdXml::xmlKeyMix().toStdString()); parameter) {
            parameter->get().setValue(value);
        }
    }

protected:
    void processSample(double & left, double & right) override
    {
        left = 1.0;
        right = 1.0;
    }
};

} // namespace

void EffectsTest::test_mixLaw_crossfade_shouldBlendDryAgainstWet()
{
    ConstantEffect effect { 1.0f, Effect::MixLaw::Crossfade };
    effect.setMix(0.25f);

    double left = 0.4;
    double right = 0.4;
    effect.process(left, right);

    // 0.4 * 0.75 + 1.0 * 0.25
    QVERIFY(std::abs(left - 0.55) < 1.0e-9);
    QVERIFY(std::abs(right - 0.55) < 1.0e-9);
}

void EffectsTest::test_mixLaw_additive_shouldKeepTheDryWhole()
{
    ConstantEffect effect { 1.0f, Effect::MixLaw::Additive };
    effect.setMix(0.25f);

    double left = 0.4;
    double right = 0.4;
    effect.process(left, right);

    // 0.4 + 1.0 * 0.25: the dry is untouched, which is what a reverb's Mix has to do.
    QVERIFY(std::abs(left - 0.65) < 1.0e-9);
    QVERIFY(std::abs(right - 0.65) < 1.0e-9);
}

void EffectsTest::test_mixLaw_internal_shouldBeLeftToTheEffect()
{
    // An effect that shapes at an oversampled rate has to blend its own Mix, against a dry delayed
    // by the same resampling filters. This class must not blend a second time on top of that.
    ConstantEffect effect { 1.0f, Effect::MixLaw::Internal };
    effect.setMix(0.25f);

    double left = 0.4;
    double right = 0.4;
    effect.process(left, right);

    QVERIFY(std::abs(left - 1.0) < 1.0e-9);
    QVERIFY(std::abs(right - 1.0) < 1.0e-9);
}

void EffectsTest::test_mixLaw_afterCopy_shouldStillBlend()
{
    // Mix and Solo are resolved once and held as pointers into the parameter map, so a copy has to
    // point at its own map rather than at the original's.
    ConstantEffect original { 1.0f, Effect::MixLaw::Additive };
    original.setMix(0.25f);

    ConstantEffect copy { original };
    copy.setMix(0.5f);

    double left = 0.4;
    double right = 0.4;
    copy.process(left, right);

    // The copy reads its own Mix, not the one it was copied from.
    QVERIFY(std::abs(left - 0.9) < 1.0e-9);
    QVERIFY(std::abs(right - 0.9) < 1.0e-9);
}

void EffectsTest::test_mixLaw_blockForm_shouldMatchPerSampleForm()
{
    constexpr uint32_t frameCount = 64;

    ConstantEffect perSample { 1.0f, Effect::MixLaw::Crossfade };
    perSample.setMix(0.25f);
    ConstantEffect block { 1.0f, Effect::MixLaw::Crossfade };
    block.setMix(0.25f);

    std::vector<double> expected;
    std::vector<double> buffer;
    expected.reserve(frameCount * 2);
    buffer.reserve(frameCount * 2);
    for (uint32_t i = 0; i < frameCount; i++) {
        double left = 0.1 * i;
        double right = -0.1 * i;
        buffer.push_back(left);
        buffer.push_back(right);
        perSample.process(left, right);
        expected.push_back(left);
        expected.push_back(right);
    }

    AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, 48000 };
    block.process(context);

    // The block form resolves Mix once for the whole block instead of once per sample, which must
    // not change a sample of what comes out.
    for (uint32_t i = 0; i < frameCount * 2; i++) {
        QCOMPARE(buffer.at(i), expected.at(i));
    }
}

void EffectsTest::test_reverb_mix_shouldApplyEffectBasedOnMixLevel()
{
    Reverb reverb;
    reverb.setSampleRate(44100);
    reverb.setMix(0.0f);
    reverb.setLpfCutoff(1.0f);
    reverb.setHpfCutoff(0.0f);
    reverb.sync();

    double l = 1.0;
    double r = 1.0;

    // Process many samples to ensure any internal state is active
    // Reverb tail needs some samples to build up
    for (int i = 0; i < 5000; i++) {
        double tl = 1.0;
        double tr = 1.0;
        reverb.process(tl, tr);
    }

    reverb.process(l, r);

    // With mix 0, output should be exactly equal to input
    QCOMPARE(l, 1.0f);
    QCOMPARE(r, 1.0f);

    reverb.setMix(1.0f);
    reverb.sync();

    // Reverb tail needs some samples to build up
    for (int i = 0; i < 5000; i++) {
        double tl = 1.0;
        double tr = 1.0;
        reverb.process(tl, tr);
    }

    l = 1.0;
    r = 1.0;
    reverb.process(l, r);

    // With mix 1.0, output should be different from input
    QVERIFY(l != 1.0f || r != 1.0f);

    // With additive mix 1.0 and DC input, at least one decorrelated wet channel should add energy.
    QVERIFY(std::abs(l) > 1.0f || std::abs(r) > 1.0f);
}

void EffectsTest::test_reverb_filters_shouldShapeWetSignal()
{
    auto measureDcWetEnergy = [](float hpfCutoff) {
        Reverb reverb;
        reverb.setSampleRate(44100);
        reverb.setMix(1.0f);
        reverb.setSize(0.6f);
        reverb.setDecay(0.5f);
        reverb.setDamping(0.2f);
        reverb.setPreDelay(0.0f);
        reverb.setLpfCutoff(1.0f);
        reverb.setHpfCutoff(hpfCutoff);
        reverb.sync();

        double energy = 0.0;
        for (int i = 0; i < 12000; ++i) {
            double l = 1.0;
            double r = 1.0;
            reverb.process(l, r);
            if (i > 6000) {
                energy += std::abs(l - 1.0f) + std::abs(r - 1.0f);
            }
        }
        return energy;
    };

    const double openEnergy = measureDcWetEnergy(0.0f);
    const double highPassedEnergy = measureDcWetEnergy(0.8f);

    QVERIFY(openEnergy > 1.0);
    QVERIFY(highPassedEnergy < openEnergy * 0.25);
}

void EffectsTest::test_reverb_gate_shouldCutTail()
{
    Reverb reverb;
    reverb.setSampleRate(44100.0);

    const auto setParam = [&](const QString & key, float value) {
        if (const auto p = reverb.parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    setParam(Constants::NahdXml::xmlKeyMix(), 1.0f); // Fully wet
    setParam(Constants::NahdXml::xmlKeySize(), 0.6f);
    setParam(Constants::NahdXml::xmlKeyDecay(), 0.8f); // Long natural tail (~8 s)
    setParam(Constants::NahdXml::xmlKeyPreDelay(), 0.0f);
    setParam(Constants::NahdXml::xmlKeyGated(), 1.0f);
    setParam(Constants::NahdXml::xmlKeyThreshold(), 0.333f); // ~ -40 dB
    setParam(Constants::NahdXml::xmlKeyHold(), 0.05f); // 50 ms
    setParam(Constants::NahdXml::xmlKeyRelease(), 0.4f); // ~21 ms (fast chop)
    reverb.sync();

    // A loud impulse opens the gate; then silence.
    double left = 1.0;
    double right = 1.0;
    reverb.process(left, right);

    double earlyEnergy = 0.0;
    double lateEnergy = 0.0;
    for (int i = 0; i < 20000; i++) {
        double l = 0.0;
        double r = 0.0;
        reverb.process(l, r);
        QVERIFY(std::isfinite(l));
        const double e = l * l + r * r;
        if (i >= 200 && i < 2000) {
            earlyEnergy += e; // Gate open (within the 50 ms hold)
        } else if (i >= 12000) {
            lateEnergy += e; // Gate long closed
        }
    }

    QVERIFY(earlyEnergy > 0.0); // The reverb bloomed while the gate was open.
    QVERIFY(lateEnergy < 0.001 * earlyEnergy); // ...and the tail was chopped off.
}

void EffectsTest::test_reverb_gate_disabled_shouldNotCutTail()
{
    Reverb reverb;
    reverb.setSampleRate(44100.0);

    const auto setParam = [&](const QString & key, float value) {
        if (const auto p = reverb.parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    setParam(Constants::NahdXml::xmlKeyMix(), 1.0f);
    setParam(Constants::NahdXml::xmlKeySize(), 0.6f);
    setParam(Constants::NahdXml::xmlKeyDecay(), 0.8f);
    setParam(Constants::NahdXml::xmlKeyPreDelay(), 0.0f);
    // Gate defaults to off.
    reverb.sync();

    double left = 1.0;
    double right = 1.0;
    reverb.process(left, right);

    double lateEnergy = 0.0;
    for (int i = 0; i < 14000; i++) {
        double l = 0.0;
        double r = 0.0;
        reverb.process(l, r);
        if (i >= 12000) {
            lateEnergy += l * l + r * r;
        }
    }

    QVERIFY(lateEnergy > 0.0); // Without the gate, the long tail persists.
}

void EffectsTest::test_delayEffect_shouldProcessSignalAndHandleSampleRateChanges()
{
    Delay effect;
    effect.setSampleRate(44100.0);
    effect.setBpm(120.0);
    effect.setSync(true);
    effect.setSyncDivision(0.25f); // 1/4 note

    // 120 BPM, 1/4 note = 0.5 seconds.
    // At 44100 Hz, 0.5 seconds = 22050 samples.

    // We can't easily check internal state, but we can verify it doesn't crash
    // and produces audio if we feed it something.
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);
    QVERIFY(!std::isnan(left));
    QVERIFY(!std::isnan(right));

    // Test sample rate change
    effect.setSampleRate(48000.0);
    left = 1.0f;
    right = 1.0f;
    effect.process(left, right);
    QCOMPARE(effect.sampleRate(), 48000.0);
}

void EffectsTest::test_delayEffect_shouldProduceDelayedSignal()
{
    Delay effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(0.0f); // No feedback for simplicity
    effect.setTime(0.1f); // 100ms delay = 4410 samples
    effect.setSync(false);

    // Initial output should be silence (buffer is empty)
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);
    QCOMPARE(left, 0.0f);
    QCOMPARE(right, 0.0f);

    // Process enough samples to reach the delay time
    const int delaySamples = static_cast<int>(0.1f * sampleRate);
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    // Now with feedback 0.0, the output should still be silence
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    QCOMPARE(left, 0.0f);
    QCOMPARE(right, 0.0f);

    // Now re-feed with feedback 1.0 to test delayed signal
    effect.reset();
    effect.setFeedback(1.0f);
    left = 1.0f;
    right = 1.0f;
    effect.process(left, right);

    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    QVERIFY(std::abs(left - 1.0f) < 1.0e-3f);
    QVERIFY(std::abs(right - 1.0f) < 1.0e-3f);

    // Test synced delay
    effect.reset();
    effect.setSync(true);
    effect.setFeedback(1.0f);
    effect.setBpm(120.0f);
    effect.setSyncDivision(0.25f); // 120 BPM, 1/4 note = 0.5s = 22050 samples
    const int syncDelaySamples = static_cast<int>(0.5f * sampleRate);

    left = 1.0f;
    right = 1.0f;
    effect.process(left, right);

    for (int i = 0; i + 1 < syncDelaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    QVERIFY(std::abs(left - 1.0f) < 1.0e-3f);
    QVERIFY(std::abs(right - 1.0f) < 1.0e-3f);
}

void EffectsTest::test_delayEffect_shouldMaintainFeedbackLoop()
{
    Delay effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(0.5f); // 50% feedback
    effect.setTime(0.1f); // 100ms delay = 4410 samples
    effect.setSync(false);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse of 1.0
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);
    // Output should be silence (mix is wet, buffer empty)
    QCOMPARE(left, 0.0f);

    // Wait for 1st echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    // 1st echo should be 0.5 (1.0 * feedback)
    QVERIFY(std::abs(left - 0.5f) < 1.0e-3f);

    // Wait for 2nd echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    // 2nd echo should be 0.25 (0.5 * feedback)
    QVERIFY(std::abs(left - 0.25f) < 1.0e-3f);

    // Wait for 3rd echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    // 3rd echo should be 0.125 (0.5 * 0.5 * 0.5)
    QVERIFY(std::abs(left - 0.125f) < 1.0e-3f);
}

void EffectsTest::test_delayEffect_shouldMaintainStereoFeedback()
{
    Delay effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setType(Delay::Type::Stereo);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(0.9f); // 90% feedback
    effect.setTime(0.1f); // 100ms delay = 4410 samples
    effect.setSync(false);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse of 1.0 to LEFT channel only
    double left = 1.0;
    double right = 0.0;
    effect.process(left, right);

    int echoes = 0;

    // Process 1 second (10 echoes expected)
    for (int i = 0; i < 1 * 44100; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);

        if ((i + 1) % delaySamples == 0) {
            if (l > 0.001f) {
                echoes++;
            }
            // In stereo mode, right channel should remain silent if only left was pulsed
            QCOMPARE(r, 0.0f);
        }
    }

    QCOMPARE(echoes, 10);
}

void EffectsTest::test_delayEffect_shouldProduceDecayingSeriesOfEchoes()
{
    Delay effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(0.9f); // 90% feedback
    effect.setTime(0.1f); // 100ms delay = 4410 samples
    effect.setSync(false);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse of 1.0
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);

    int echoes = 0;
    float lastEchoVal = 1.1f;

    // Process 2 seconds
    for (int i = 0; i < 2 * 44100; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);

        // If we see a pulse, count it and verify it's decaying
        if (l > 0.001f) {
            // Pulse should be around delaySamples multiples
            if ((i + 1) % delaySamples == 0) {
                echoes++;
                QVERIFY(l < lastEchoVal); // Decay check
                lastEchoVal = l;
            }
        }
    }

    // We should have seen 20 echoes
    QCOMPARE(echoes, 20);
    // Echo 1 was 1.0, Echo 2 was 0.9, ..., Echo 20 was 0.9^19
    QVERIFY(std::abs(lastEchoVal - std::pow(0.9f, 19.0f)) < 0.05f);
}

void EffectsTest::test_delayEffect_shouldProcessMonoMode()
{
    Delay effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setType(Delay::Type::Mono);
    effect.setMix(1.0f);
    effect.setFeedback(1.0f);
    effect.setTime(0.1f);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse to LEFT channel only
    double left = 1.0;
    double right = 0.0;
    effect.process(left, right);

    // Wait for 1st echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);

    // In Mono mode, the left-only input should be summed and distributed to both channels
    // (1.0 + 0.0) * 0.5 = 0.5 expected on both channels
    QVERIFY(std::abs(left - 0.5f) < 1.0e-3f);
    QVERIFY(std::abs(right - 0.5f) < 1.0e-3f);
}

void EffectsTest::test_delayEffect_shouldProcessPingPongMode()
{
    Delay effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setType(Delay::Type::PingPong);
    effect.setMix(1.0f);
    effect.setFeedback(1.0f);
    effect.setDepth(1.0f); // Max width
    effect.setTime(0.1f);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse to LEFT channel only
    double left = 1.0;
    double right = 0.0;
    effect.process(left, right);

    // Wait for 1st echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);

    // Ping-Pong: Left input should first appear on RIGHT channel?
    // Let's check implementation:
    // inL = inputL + inputR * (1.0 - m_depth) = 1.0 + 0.0 = 1.0
    // inR = inputR * (1.0 - m_depth) = 0.0
    // m_bufferL = inL + fbR * m_feedback = 1.0
    // m_bufferR = inR + fbL * m_feedback = 0.0
    // 1st Read (Stereo-like read): outL = bufL, outR = bufR
    // So 1st echo should be Left=1.0, Right=0.0
    QVERIFY(std::abs(left - 1.0f) < 1.0e-3f);
    QVERIFY(std::abs(right - 0.0f) < 1.0e-3f);

    // 2nd echo should bounce: fbL=1.0, fbR=0.0
    // next bufferL = inL + fbR = 0.0 + 0.0 = 0.0
    // next bufferR = inR + fbL = 0.0 + 1.0 = 1.0
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);

    QVERIFY(std::abs(left - 0.0f) < 1.0e-3f);
    QVERIFY(std::abs(right - 1.0f) < 1.0e-3f);
}

void EffectsTest::test_delayEffect_shouldProcessTapeMode()
{
    Delay effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setType(Delay::Type::Tape);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(1.0f);
    effect.setDepth(1.0f); // High saturation
    effect.setTime(0.1f);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a large signal pulse for 10 samples
    const float pulseVal = 2.0f;
    for (int i = 0; i < 10; i++) {
        double left = pulseVal;
        double right = pulseVal;
        effect.process(left, right);
    }

    bool foundEcho = false;
    // We expect the echo around delaySamples. Let's check a window.
    for (int i = 0; i < delaySamples + 100; i++) {
        double left = 0.0;
        double right = 0.0;
        effect.process(left, right);

        if (left > 0.01f) {
            foundEcho = true;
            if (left >= pulseVal) {
                qDebug() << "Tape mode failed: left =" << left << "pulseVal =" << pulseVal << "i =" << i;
            }
            // In Tape mode, output should be saturated (less than pulseVal)
            QVERIFY(left < pulseVal);
        }
    }

    QVERIFY(foundEcho);
}

void EffectsTest::test_delayEffect_shouldSyncParameters()
{
    Delay effect;
    effect.setSampleRate(44100.0);

    // Initial check (defaults)
    QCOMPARE(effect.feedbackLpf(), 1.0);
    QCOMPARE(effect.feedbackHpf(), 0.0);

    // Test Discrete Type
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p) {
        p->get().setFromXml(2); // PingPong
        effect.sync();
    }

    // Test Continuous Time
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p) {
        p->get().setValue(0.123f); // 1.23 seconds because of * 10.0 scaling
        effect.sync();
    }

    // Test Feedback LPF/HPF which have getters
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p) {
        p->get().setValue(0.456f);
        effect.sync();
        QCOMPARE(effect.feedbackLpf(), 0.456f);
    }

    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p) {
        p->get().setValue(0.789f);
        effect.sync();
        QCOMPARE(effect.feedbackHpf(), 0.789f);
    }
}

void EffectsTest::test_delayEffect_typeParameter_shouldSelectPingPong()
{
    // Regression: selecting a mode via the parameter system (as the UI does) must actually change the mode.
    // delayType is a discrete parameter whose value is the raw enum index; PingPong is index 2.
    Delay effect;
    effect.setSampleRate(44100.0);

    const auto setParam = [&](const QString & key, float value) {
        if (const auto p = effect.parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };

    setParam(Constants::NahdXml::xmlKeyDelayType(), 2.0f); // PingPong
    setParam(Constants::NahdXml::xmlKeyDelayMix(), 1.0f); // Fully wet
    setParam(Constants::NahdXml::xmlKeyDelayFeedback(), 1.0f);
    setParam(Constants::NahdXml::xmlKeyDelayDepth(), 1.0f); // Max width
    setParam(Constants::NahdXml::xmlKeyDelayTime(), 0.01f); // 0.01 * 10s = 0.1s
    effect.sync();

    const int delaySamples = static_cast<int>(0.1 * 44100.0);

    // Feed a pulse to the LEFT channel only.
    double left = 1.0;
    double right = 0.0;
    effect.process(left, right);

    // First echo appears on the LEFT (same side as input).
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    QVERIFY(std::abs(left - 1.0) < 1.0e-3);
    QVERIFY(std::abs(right) < 1.0e-3);

    // Second echo bounces to the RIGHT: this only happens in PingPong mode.
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    QVERIFY(std::abs(left) < 1.0e-3);
    QVERIFY(std::abs(right - 1.0) < 1.0e-3);
}

void EffectsTest::test_limiterEffect_shouldLimitPeaksToCeiling()
{
    Limiter effect;
    effect.setSampleRate(44100.0);

    // Defaults: Threshold 0dB, Ceiling -0.3dB, Boost off, Lookahead 5ms, Release ~100ms.
    const double ceilingLin = Utils::Dsp::dbToLinear(-0.3f);

    // Feed a full-scale signal; the limiter must pull it down to the ceiling. Process long enough for
    // the lookahead delay line to flush and the envelope to settle.
    double left = 0.0;
    double right = 0.0;
    for (int i = 0; i < 4000; i++) {
        left = 1.0;
        right = 1.0;
        effect.process(left, right);
        // The brickwall backstop guarantees the output never exceeds the ceiling.
        QVERIFY(std::abs(left) <= ceilingLin + 1e-6);
        QVERIFY(std::abs(right) <= ceilingLin + 1e-6);
    }

    // After settling the output should sit right at the ceiling and gain reduction should be reported.
    QVERIFY(std::abs(std::abs(left) - ceilingLin) < 1e-3);
    QVERIFY(effect.reductionDb() < 0.0f);

    // Drive it hard: a full-scale signal against a -12dB threshold must show ~12dB of real gain reduction
    // (not a token amount), which confirms the smooth limiter gain — not just the brickwall clamp — is working.
    {
        effect.reset();
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
            p->get().setValue(0.5f); // -12dB
        }
        effect.sync();

        for (int i = 0; i < 4000; i++) {
            left = 1.0;
            right = 1.0;
            effect.process(left, right);
        }

        QVERIFY(effect.reductionDb() < -11.0f);
        QVERIFY(effect.reductionDb() > -13.0f);
        // Output limited to the -12dB threshold (below the ceiling, so no boost make-up).
        QVERIFY(std::abs(std::abs(left) - Utils::Dsp::dbToLinear(-12.0f)) < 1e-2);
    }
}

void EffectsTest::test_limiterEffect_shouldBoostToCeiling()
{
    Limiter effect;
    effect.setSampleRate(44100.0);

    // Threshold -12dB, Boost on: a -12dB signal should be lifted so it peaks at the ceiling (-0.3dB).
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        p->get().setValue(0.5f); // -24 + 0.5 * 24 = -12dB
    }
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBoost().toStdString()); p) {
        p->get().setValue(1.0f); // On
    }
    effect.sync();

    const double ceilingLin = Utils::Dsp::dbToLinear(-0.3f);
    const double val = Utils::Dsp::dbToLinear(-12.0f);

    double left = 0.0;
    double right = 0.0;
    for (int i = 0; i < 2000; i++) {
        left = val;
        right = val;
        effect.process(left, right);
    }

    // Boost make-up brings -12dB up to the -0.3dB ceiling without gain reduction (signal sits at threshold).
    QVERIFY(std::abs(std::abs(left) - ceilingLin) < 1e-3);
    QVERIFY(std::abs(std::abs(right) - ceilingLin) < 1e-3);
}

void EffectsTest::test_endlessReverb_shouldProduceStableWetTail()
{
    EndlessReverb effect;
    effect.setSampleRate(44100.0);

    const auto setParam = [&](const QString & key, float value) {
        if (const auto p = effect.parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    setParam(Constants::NahdXml::xmlKeyMix(), 1.0f); // Fully wet
    setParam(Constants::NahdXml::xmlKeySize(), 0.5f);
    setParam(Constants::NahdXml::xmlKeyDecay(), 0.6f);
    effect.sync();

    // Feed a single impulse, then process silence and collect the tail.
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);

    double energy = 0.0;
    double maxAbs = 0.0;
    for (int i = 0; i < 20000; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
        QVERIFY(std::isfinite(l));
        QVERIFY(std::isfinite(r));
        energy += l * l + r * r;
        maxAbs = std::max(maxAbs, std::max(std::abs(l), std::abs(r)));
    }

    QVERIFY(energy > 0.0); // A reverberant tail was produced.
    QVERIFY(maxAbs < 10.0); // ...and it stayed bounded/stable.
}

void EffectsTest::test_endlessReverb_mixZero_shouldPassDrySignal()
{
    EndlessReverb effect;
    effect.setSampleRate(44100.0);
    effect.sync(); // Mix defaults to 0 (fully dry).

    double left = 0.7;
    double right = -0.3;
    effect.process(left, right);

    QVERIFY(std::abs(left - 0.7) < 1.0e-9);
    QVERIFY(std::abs(right + 0.3) < 1.0e-9);
}

void EffectsTest::test_endlessReverb_freeze_shouldSustainTail()
{
    EndlessReverb effect;
    effect.setSampleRate(44100.0);

    const auto setParam = [&](const QString & key, float value) {
        if (const auto p = effect.parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    setParam(Constants::NahdXml::xmlKeyMix(), 1.0f);
    setParam(Constants::NahdXml::xmlKeySize(), 0.5f);
    setParam(Constants::NahdXml::xmlKeyDecay(), 0.9f);
    effect.sync();

    // Build energy in the network.
    for (int i = 0; i < 8000; i++) {
        double l = (i % 97 < 3) ? 0.8 : 0.0;
        double r = (i % 89 < 3) ? 0.8 : 0.0;
        effect.process(l, r);
    }

    // Engage freeze: the tail should now sustain instead of decaying.
    setParam(Constants::NahdXml::xmlKeyFreeze(), 1.0f);
    effect.sync();

    const auto windowEnergy = [&](int frames) {
        double e = 0.0;
        for (int i = 0; i < frames; i++) {
            double l = 0.0;
            double r = 0.0;
            effect.process(l, r);
            e += l * l + r * r;
        }
        return e;
    };

    const double firstEnergy = windowEnergy(4000);
    for (int i = 0; i < 40000; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
        QVERIFY(std::isfinite(l));
    }
    const double lastEnergy = windowEnergy(4000);

    QVERIFY(firstEnergy > 0.0);
    // A frozen (lossless) network keeps most of its energy rather than decaying toward silence.
    QVERIFY(lastEnergy > 0.25 * firstEnergy);
}

void EffectsTest::test_eq8BandParametricEffect_shouldApplyBandsAndBeStable()
{
    Eq8BandParametric effect;
    effect.setSampleRate(44100.0);

    // Test defaults
    {
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandQ(0).toStdString()); p) {
            // Default should be 0.5f (maps to 1.0)
            QCOMPARE(p->get().value(), 0.5f);
        }
    }

    // Test bypass (all bands default to bypass)
    {
        double left = 1.0;
        double right = 1.0;
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 1.0f);
    }

    // Test Bell filter
    {
        effect.reset();
        // Band 1: Bell, 1000Hz, +12dB, Q=1.0
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandType(0).toStdString()); p) {
            p->get().setValue(1.0f); // Bell
        }
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandFreq(0).toStdString()); p) {
            p->get().setValue(0.5f); // 1000Hz approx
        }
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandGain(0).toStdString()); p) {
            p->get().setValue(0.75f); // +12dB
        }
        effect.sync();

        double left = 1.0;
        double right = 1.0;
        effect.process(left, right);

        // At 0Hz (DC), a bell filter at 1000Hz with Q=1.0 should have some gain
        // but not the full +12dB. Output should be > 1.0.
        QVERIFY(left > 1.0f);
        QVERIFY(right > 1.0f);
    }

    // Test stability
    {
        for (int i = 0; i < 1000; i++) {
            double left = 1.0;
            double right = 1.0;
            effect.process(left, right);
            QVERIFY(!std::isnan(left));
            QVERIFY(!std::isinf(left));
        }
    }
}

namespace {
// Configure band 0 as a Low Shelf boosting DC (~10 kHz corner) by +12 dB and select the given stereo mode,
// so a constant (DC) input reaches a clean, boosted steady state on the processed channel.
void configureShelfEq(Eq8BandParametric & eq, float stereoModeValue)
{
    if (auto p = eq.parameter(Constants::NahdXml::xmlKeyBandType(0).toStdString()); p) {
        p->get().setValue(2.0f); // Low Shelf
    }
    if (auto p = eq.parameter(Constants::NahdXml::xmlKeyBandFreq(0).toStdString()); p) {
        p->get().setValue(0.9f); // ~10 kHz, so DC sits well inside the shelf
    }
    if (auto p = eq.parameter(Constants::NahdXml::xmlKeyBandGain(0).toStdString()); p) {
        p->get().setValue(0.75f); // +12 dB
    }
    if (auto p = eq.parameter(Constants::NahdXml::xmlKeyStereoMode().toStdString()); p) {
        p->get().setValue(stereoModeValue);
    }
    eq.sync();
}
} // namespace

void EffectsTest::test_eq8BandParametricEffect_stereoMode_shouldDefaultToMidSide()
{
    Eq8BandParametric effect;
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyStereoMode().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.0f); // 0 == Mid + Side
    }
}

void EffectsTest::test_eq8BandParametricEffect_midMode_shouldAffectMidOnly()
{
    Eq8BandParametric effect;
    effect.setSampleRate(44100.0);
    configureShelfEq(effect, 1.0f); // Mid

    // Pure mid content (L == R) is processed and boosted.
    double left = 0.0;
    double right = 0.0;
    for (int i = 0; i < 4000; i++) {
        left = 0.2;
        right = 0.2;
        effect.process(left, right);
    }
    QVERIFY(left > 0.5); // Boosted from 0.2 (+12 dB ~= 0.79)
    QVERIFY(std::abs(left - right) < 1e-9); // Stays balanced (pure mid)

    // Pure side content (L == -R) has zero mid, so it passes through untouched.
    effect.reset();
    for (int i = 0; i < 4000; i++) {
        left = 0.2;
        right = -0.2;
        effect.process(left, right);
    }
    QVERIFY(std::abs(left - 0.2) < 1e-3);
    QVERIFY(std::abs(right + 0.2) < 1e-3);
}

void EffectsTest::test_eq8BandParametricEffect_sideMode_shouldAffectSideOnly()
{
    Eq8BandParametric effect;
    effect.setSampleRate(44100.0);
    configureShelfEq(effect, 2.0f); // Side

    // Pure side content (L == -R) is processed and boosted.
    double left = 0.0;
    double right = 0.0;
    for (int i = 0; i < 4000; i++) {
        left = 0.2;
        right = -0.2;
        effect.process(left, right);
    }
    QVERIFY(std::abs(left) > 0.5); // Boosted side
    QVERIFY(std::abs(left + right) < 1e-9); // Stays anti-symmetric (pure side)

    // Pure mid content (L == R) has zero side, so it passes through untouched.
    effect.reset();
    for (int i = 0; i < 4000; i++) {
        left = 0.2;
        right = 0.2;
        effect.process(left, right);
    }
    QVERIFY(std::abs(left - 0.2) < 1e-3);
    QVERIFY(std::abs(right - 0.2) < 1e-3);
}

void EffectsTest::test_eq8BandParametricEffect_midSideMode_shouldAffectBothChannels()
{
    Eq8BandParametric effect;
    effect.setSampleRate(44100.0);
    configureShelfEq(effect, 0.0f); // Mid + Side (default)

    // Pure mid content is boosted.
    double left = 0.0;
    double right = 0.0;
    for (int i = 0; i < 4000; i++) {
        left = 0.2;
        right = 0.2;
        effect.process(left, right);
    }
    QVERIFY(left > 0.5);
    QVERIFY(right > 0.5);

    // Pure side content is also boosted (unlike Mid or Side modes, both are processed).
    effect.reset();
    for (int i = 0; i < 4000; i++) {
        left = 0.2;
        right = -0.2;
        effect.process(left, right);
    }
    QVERIFY(std::abs(left) > 0.5);
    QVERIFY(std::abs(right) > 0.5);
}

void EffectsTest::test_clipperEffect_shouldClipSignal()
{
    Clipper effect;

    // Test Hard Clipping
    {
        effect.reset();
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
            p->get().setValue(0.0f); // Hard
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
            p->get().setValue(0.5f); // -12dB approx 0.2511
        }
        effect.sync();

        const auto threshold = Utils::Dsp::dbToLinear(-12.0f);
        auto left = 1.0;
        auto right = 1.0;
        effect.process(left, right);

        QCOMPARE(static_cast<float>(left), threshold);
        QCOMPARE(static_cast<float>(right), threshold);

        left = -1.0;
        right = -1.0;
        effect.process(left, right);
        QCOMPARE(static_cast<float>(left), -threshold);
        QCOMPARE(static_cast<float>(right), -threshold);
    }

    // Test Soft Clipping (Tanh)
    {
        effect.reset();
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
            p->get().setValue(1.0f); // Soft
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
            p->get().setValue(1.0f); // 0dB = 1.0
        }
        effect.sync();

        auto left = 1.0;
        auto right = 1.0;
        effect.process(left, right);

        // tanh(1.0) is approx 0.7615
        QVERIFY(left < 1.0);
        QVERIFY(left > 0.76);
    }

    // Test Gain
    {
        effect.reset();
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
            p->get().setValue(0.0f); // Hard
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
            p->get().setValue(1.0f); // 0dB = 1.0
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
            p->get().setValue(0.75f); // +12dB = 3.98 approx
        }
        effect.sync();

        auto left = 0.5;
        auto right = 0.5;
        effect.process(left, right);

        const auto expected = 0.5 * Utils::Dsp::dbToLinear(12.0f);
        QCOMPARE(static_cast<float>(left), expected);
    }
}

void EffectsTest::test_saturatorEffect_shouldShapeSignalPerMode()
{
    Saturator effect;

    // Tape mode: unity drive, full wet, open tone -> plain tanh shaping
    {
        effect.reset();
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
            p->get().setValue(0.0f); // Tape
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString()); p) {
            p->get().setValue(0.0f); // 0dB
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyTone().toStdString()); p) {
            p->get().setValue(1.0f); // Fully open, filter bypassed
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
            p->get().setValue(1.0f); // Fully wet
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
            p->get().setValue(0.5f); // 0dB
        }
        effect.sync();

        auto left = 1.0;
        auto right = 1.0;
        effect.process(left, right);

        QCOMPARE(static_cast<float>(left), static_cast<float>(std::tanh(1.0)));
        QCOMPARE(static_cast<float>(right), static_cast<float>(std::tanh(1.0)));
    }

    // Diode mode: heavy drive should saturate to unity
    {
        effect.reset();
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
            p->get().setValue(2.0f); // Diode
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString()); p) {
            p->get().setValue(1.0f); // +24dB
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyTone().toStdString()); p) {
            p->get().setValue(1.0f);
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
            p->get().setValue(1.0f);
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
            p->get().setValue(0.5f);
        }
        effect.sync();

        auto left = 1.0;
        auto right = -1.0;
        effect.process(left, right);

        QCOMPARE(static_cast<float>(left), 1.0f);
        QCOMPARE(static_cast<float>(right), -1.0f);
    }
}

void EffectsTest::test_saturatorEffect_shouldRespectMix()
{
    Saturator effect;
    effect.reset();

    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString()); p) {
        p->get().setValue(1.0f); // Heavy drive so wet/dry clearly differ
    }
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        p->get().setValue(0.0f); // Fully dry
    }
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        p->get().setValue(0.5f); // 0dB
    }
    effect.sync();

    auto left = 0.3;
    auto right = -0.4;
    effect.process(left, right);

    QCOMPARE(static_cast<float>(left), 0.3f);
    QCOMPARE(static_cast<float>(right), -0.4f);
}

void EffectsTest::test_saturatorEffect_shouldReportSaturationMeter()
{
    Saturator effect;
    effect.reset();

    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        p->get().setValue(2.0f); // Diode
    }
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString()); p) {
        p->get().setValue(1.0f); // +24dB, drives well past the shaping knee
    }
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        p->get().setValue(1.0f);
    }
    effect.sync();

    QCOMPARE(effect.saturationDb(), 0.0f);

    auto left = 1.0;
    auto right = 1.0;
    effect.process(left, right);

    QVERIFY(effect.saturationDb() < 0.0f);
}

void EffectsTest::test_filterStability_shouldHandleChangingCutoff()
{
    LowPassFilter lp;
    HighPassFilter hp;

    for (int i = 0; i < 1000; ++i) {
        double left = 1.0;
        double right = 1.0;
        const float cutoff = 0.5f + 0.49f * std::sin(i * 0.1f);

        lp.setCutoff(cutoff);
        hp.setCutoff(cutoff);

        lp.process(left, right);
        hp.process(left, right);

        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }
}

void EffectsTest::test_cascadedSvfStability_shouldHandleRapidParameterChanges()
{
    CascadedSvf filter {};
    filter.setSampleRate(static_cast<uint32_t>(Constants::defaultSampleRate()));

    // Stress test: Rapidly change parameters
    for (int i = 0; i < 1000; ++i) {
        filter.setCutoff(0.5 + 0.49 * std::sin(i * 0.1));
        filter.setResonance(0.5 + 0.49 * std::cos(i * 0.05));

        double out = filter.process(1.0);
        QVERIFY(!std::isnan(out));
        QVERIFY(!std::isinf(out));
    }

    // Check for NaN recovery
    filter.setCutoff(0.5);
    filter.setResonance(0.5);
    double out = filter.process(1.0);
    QVERIFY(!std::isnan(out));
}

void EffectsTest::test_chorusEffect_shouldProcessAudio()
{
    Chorus effect;
    effect.setSampleRate(44100.0);
    effect.setRate(1.0);
    effect.setDepth(0.5);
    effect.setDelay(20.0);
    effect.setMix(1.0); // Wet only

    // Process some silence to initialize
    for (int i = 0; i < 1000; i++) {
        double l = 0.0, r = 0.0;
        effect.process(l, r);
    }

    bool signalDetected = false;
    // Process enough samples to reach the delay (~20ms @ 44.1kHz = 882 samples)
    // plus some extra for modulation and filters
    for (int i = 0; i < 5000; i++) {
        // Use a sine wave to avoid DC issues with HPF
        double input = std::sin(2.0 * M_PI * 440.0 * i / 44100.0);
        double l = input, r = input;
        effect.process(l, r);
        if (i > 1500) {
            if (std::abs(l) > 0.1 || std::abs(r) > 0.1) {
                signalDetected = true;
                break;
            }
        }
    }
    QVERIFY(signalDetected);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectsTest)
