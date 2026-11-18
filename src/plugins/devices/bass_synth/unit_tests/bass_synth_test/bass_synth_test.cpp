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

#include "bass_synth_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../infra/xml/nahd_xml_reader.hpp"
#include "../../../../../infra/xml/nahd_xml_writer.hpp"
#include "../../bass_synth_device.hpp"

#include <QTest>

#include <cmath>
#include <numbers>

namespace noteahead {

void BassSynthTest::test_serialization_shouldRestoreParameters()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setLpfCutoff(0.75f);
    synth.setWaveform(PolyBlepOscillator::Waveform::Square);
    synth.setSubLevel(0.5f);
    synth.setPitchBendRange(7);

    QString xml;
    NahdXmlWriter writer { xml };
    synth.serializeToXml(writer);

    BassSynthDevice synth2 { "Restored BassSynth" };
    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        synth2.deserializeFromXml(reader);
    }

    QCOMPARE(synth2.lpfCutoff(), 0.75f);
    QCOMPARE(static_cast<int>(synth2.waveform()), static_cast<int>(PolyBlepOscillator::Waveform::Square));
    QCOMPARE(synth2.subLevel(), 0.5f);
    QCOMPARE(synth2.pitchBendRange(), 7);
}

void BassSynthTest::test_midiProcessing_shouldTriggerAudio()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.processMidiNoteOn(60, 100);
    QVERIFY(synth.hasActiveAudio());

    synth.processMidiNoteOff(60);
    synth.processMidiAllNotesOff();
    // Voice is now in release phase — process audio to let the envelope reach Idle
    const int frameCount { 1024 };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
    synth.processAudio(context);
    QVERIFY(!synth.hasActiveAudio());
}

void BassSynthTest::test_legatoSlide_shouldStayActive()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setSlide(0.5f);

    synth.processMidiNoteOn(60, 100);
    QVERIFY(synth.hasActiveAudio());

    // Trigger another note while first is active
    synth.processMidiNoteOn(72, 100);
    // Monophonic, so still active
    QVERIFY(synth.hasActiveAudio());
}

void BassSynthTest::test_velocityAndAccent_shouldTriggerAudio()
{
    BassSynthDevice synth { "Test BassSynth" };

    // Normal velocity
    synth.processMidiNoteOn(60, 80);
    QVERIFY(synth.hasActiveAudio());

    // Accent velocity
    synth.processMidiNoteOn(60, 110);
    QVERIFY(synth.hasActiveAudio());
}

void BassSynthTest::test_retriggerOnSlide_shouldIncreaseVolume()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setSlide(0.5f); // Enable slide
    synth.setDecay(0.1f); // Short decay to see volume drop

    // 1. Trigger first note
    synth.processMidiNoteOn(60, 100);

    // Render some audio to let it reach decay phase
    const int frameCount { 1000 };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
    synth.processAudio(context);

    double peak1 { 0.0 };
    for (double sample : buffer) {
        peak1 = std::max(peak1, std::abs(sample));
    }
    QVERIFY(peak1 > 0.0f);

    // Render more to let it decay a lot
    synth.processAudio(context);
    double lastVal { std::abs(buffer[buffer.size() - 2]) };

    // 2. Trigger second note (legato)
    synth.processMidiNoteOn(62, 100);

    // Render again. Volume should INCREASE because of re-triggering attack
    synth.processAudio(context);
    double peak2 { 0.0 };
    for (double sample : buffer) {
        peak2 = std::max(peak2, std::abs(sample));
    }

    QVERIFY(peak2 > lastVal); // Re-triggering worked!
}

void BassSynthTest::test_noClickOnSlideZero_shouldNotHaveLargeDiscontinuities()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setSlide(0.0f); // Slide off

    // Trigger two overlapping notes rapidly
    synth.processMidiNoteOn(60, 100);

    const int frameCount { 100 };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
    synth.processAudio(context);

    // Trigger next note immediately
    synth.processMidiNoteOn(62, 100);

    synth.processAudio(context);

    // Check for huge jumps (clicks) in the transition
    for (size_t i { 1 }; i < buffer.size(); i++) {
        double diff { std::abs(buffer[i] - buffer[i - 1]) };
        // A click would be a very large jump, e.g., > 0.5 in one sample
        QVERIFY2(diff < 0.5f, QString("Large discontinuity detected: %1").arg(static_cast<double>(diff)).toUtf8().constData());
    }
}

void BassSynthTest::test_outputLevel_shouldBeCorrect()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setVolume(1.0f);
    synth.setGain(0.5f); // 0 dB
    synth.setPan(0.5f); // Center
    synth.setAccent(0.0f); // Ensure no accent boost
    synth.setWaveform(PolyBlepOscillator::Waveform::Square);
    synth.setSubLevel(0.0f);
    synth.setDistDrive(0.0f);
    synth.setLpfCutoff(1.0f); // Wide open

    synth.processMidiNoteOn(60, 127);

    const int frameCount { 1000 };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
    synth.processAudio(context);

    double peak { 0.0 };
    for (double sample : buffer) {
        peak = std::max(peak, std::abs(sample));
    }

    // Square wave base peak is ~1.0. Constant-power center pan gives cos(π/4) ≈ 0.707 multiplier.
    // Diode ladder adds some gain even at 0 resonance, so observed peak is around 0.76.
    QVERIFY2(peak > 0.6f && peak < 1.0f, QString("Peak level incorrect: %1").arg(static_cast<double>(peak)).toUtf8().constData());
}

void BassSynthTest::test_noteOff_shouldCutNoteQuickly()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setDecay(1.0f); // Long decay (mapped to 10s)

    synth.processMidiNoteOn(60, 100);
    QVERIFY(synth.hasActiveAudio());

    // Render 10ms
    const int sampleRate { 44100 };
    const int frameCount { static_cast<int>(sampleRate * 0.01) };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), sampleRate };
    synth.processAudio(context);

    // Note off
    synth.processMidiNoteOff(60);

    // Render another 20ms. Since release should be 5ms (fixed), it should be inactive by now.
    // If it's linked to decay (10s), it will definitely still be active.
    const int frameCount2 { static_cast<int>(sampleRate * 0.02) };
    std::vector<double> buffer2(static_cast<size_t>(frameCount2) * 2, 0.0);
    AudioContext context2 { std::span(buffer2.data(), buffer2.size()), static_cast<uint32_t>(frameCount2), sampleRate };
    synth.processAudio(context2);

    QVERIFY(!synth.hasActiveAudio());
}

void BassSynthTest::test_sineWave_noClickAtAttack()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setWaveform(PolyBlepOscillator::Waveform::Sine);
    synth.setLpfCutoff(0.5f);
    synth.setEnvMod(0.5f);
    synth.setDecay(0.1f);
    synth.setVolume(1.0f);
    synth.setGain(0.5f);

    // Render multiple note ons to check for clicks
    for (int i = 0; i < 5; i++) {
        synth.processMidiAllNotesOff();
        synth.processMidiNoteOn(36, 100); // Low note

        const int frameCount { 128 };
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(context);

        // Check for discontinuity at the very beginning
        // The first few samples should be very small and smooth
        for (size_t j = 1; j < 10; j++) {
            double diff { std::abs(buffer[j * 2] - buffer[(j - 1) * 2]) };
            // A click at attack would be a sudden jump.
            // Since it's a sine at 65Hz (note 36) at 44100Hz, the max slope is small.
            // max slope = 2 * pi * f / fs = 2 * pi * 65 / 44100 = 0.009
            // Any jump > 0.05 is likely a click.
            QVERIFY2(diff < 0.05, QString("Click detected at sample %1: diff %2").arg(j).arg(diff).toUtf8().constData());
        }
    }
}

void BassSynthTest::test_midiVelocity_shouldAffectVolume()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setVolume(1.0f);
    synth.setGain(0.5f);
    synth.setAccent(0.0f); // Disable accent effect to focus on velocity

    auto getPeak = [&](uint8_t velocity) {
        synth.processMidiAllNotesOff();
        synth.processMidiNoteOn(60, velocity);

        const int frameCount { 1000 };
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(context);

        double peak { 0.0 };
        for (const double sample : buffer) {
            peak = std::max(peak, std::abs(sample));
        }
        return peak;
    };

    const double peakLow = getPeak(40);
    const double peakHigh = getPeak(127);

    QVERIFY2(peakHigh > peakLow, QString("Velocity did not affect volume: peakLow=%1, peakHigh=%2").arg(peakLow).arg(peakHigh).toUtf8().constData());
}

void BassSynthTest::test_subOscOptimization_shouldSkipSilentSubOsc()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setVolume(1.0f);
    synth.setGain(0.5f);
    synth.setLpfCutoff(1.0f);
    synth.setSubLevel(0.0f);

    auto getOutput = [&]() {
        synth.resetAudio();
        synth.processMidiNoteOn(60, 100);
        const int frameCount { 100 };
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(context);
        return buffer;
    };

    // 1. Initial output with SubLevel 0
    const auto buffer1 = getOutput();

    // 2. Change Sub-oscillator parameters (should be skipped)
    synth.setSubOctave(2);
    const auto buffer2 = getOutput();

    for (size_t i = 0; i < buffer1.size(); i++) {
        QVERIFY2(std::abs(buffer1[i] - buffer2[i]) < 1e-6,
                 QString("Output changed when Sub-oscillator parameters changed: diff=%1").arg(std::abs(buffer1[i] - buffer2[i])).toUtf8().constData());
    }
}

void BassSynthTest::test_pitchBendRange_shouldScaleBendOffset()
{
    BassSynthDevice synth { "Test BassSynth" };
    QCOMPARE(synth.pitchBendRange(), 2);

    // Default center
    QCOMPARE(synth.currentPitchBendOffset(), 0.0f);

    // Max up
    synth.processMidiPitchBend(16383, 0);
    QCOMPARE(std::round(synth.currentPitchBendOffset()), 2.0f);

    // Max down
    synth.processMidiPitchBend(0, 0);
    QCOMPARE(std::round(synth.currentPitchBendOffset()), -2.0f);

    // Range change
    synth.setPitchBendRange(12);
    synth.processMidiPitchBend(16383, 0);
    QCOMPARE(std::round(synth.currentPitchBendOffset()), 12.0f);
}

void BassSynthTest::test_pitchBendRange_shouldShiftRenderedFrequency()
{
    const uint32_t sampleRate = 44100;
    const uint32_t frameCount = 8192;

    const auto zeroCrossings = [=](uint16_t pitchBend) {
        BassSynthDevice synth { "Test BassSynth" };
        synth.setWaveform(PolyBlepOscillator::Waveform::Sine);
        synth.setSubLevel(0.0f);
        synth.setLpfCutoff(1.0f);
        synth.setDecay(1.0f);
        synth.setPitchBendRange(12);
        synth.processMidiPitchBend(pitchBend, 0);
        synth.processMidiNoteOn(60, 100);

        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
        synth.processAudio(context);

        size_t crossings = 0;
        for (size_t i = 2; i < buffer.size(); i += 2) { // Left channel only
            if ((buffer.at(i - 2) < 0.0) != (buffer.at(i) < 0.0)) {
                crossings++;
            }
        }
        return crossings;
    };

    const auto centered = zeroCrossings(8192);
    QVERIFY(centered > 0);

    // Max bend up with a range of 12 semitones doubles the frequency
    const auto bentUp = zeroCrossings(16383);
    QVERIFY(static_cast<double>(bentUp) > static_cast<double>(centered) * 1.8);
    QVERIFY(static_cast<double>(bentUp) < static_cast<double>(centered) * 2.2);
}

void BassSynthTest::test_tuning_shouldMatchEqualTemperamentA440()
{
    constexpr uint32_t sampleRate = 48000;
    constexpr uint32_t frameCount = 16384;

    for (const uint8_t midiNote : { uint8_t { 24 }, uint8_t { 36 }, uint8_t { 48 }, uint8_t { 60 }, uint8_t { 72 } }) {
        BassSynthDevice synth { "Test BassSynth" };
        synth.setWaveform(PolyBlepOscillator::Waveform::Sine);
        synth.setSubLevel(0.0f);
        synth.setLpfCutoff(1.0f);
        synth.setEnvMod(0.0f);
        synth.setDecay(1.0f);
        synth.processMidiNoteOn(midiNote, 100);

        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
        synth.processAudio(context);

        // Measure positive-going zero crossings in steady state after attack
        int firstCrossing = -1;
        int lastCrossing = -1;
        int periods = 0;
        for (uint32_t i = 1024; i < frameCount; i++) {
            const double prev = buffer[static_cast<size_t>(i - 1) * 2];
            const double curr = buffer[static_cast<size_t>(i) * 2];
            if (prev <= 0.0 && curr > 0.0) {
                if (firstCrossing < 0) {
                    firstCrossing = static_cast<int>(i);
                } else {
                    lastCrossing = static_cast<int>(i);
                    periods++;
                }
            }
        }

        QVERIFY(periods > 4);
        const double measuredFreq = static_cast<double>(periods) * static_cast<double>(sampleRate) / static_cast<double>(lastCrossing - firstCrossing);
        const double expectedFreq = 440.0 * std::pow(2.0, (static_cast<int>(midiNote) - 69) / 12.0);
        const double centsError = std::abs(1200.0 * std::log2(measuredFreq / expectedFreq));
        QVERIFY2(centsError < 5.0, QString("MIDI note %1 tuning error too large: %2 cents").arg(midiNote).arg(centsError).toUtf8().constData());
    }
}

void BassSynthTest::test_resonanceSweep_shouldBoostResonantPeakWithoutSelfOscillation()
{
    constexpr uint32_t sampleRate = 48000;
    constexpr uint32_t frameCount = 4096;

    const auto measureHarmonicRatio = [&](float resonance) {
        BassSynthDevice synth { "Test BassSynth" };
        synth.setWaveform(PolyBlepOscillator::Waveform::Saw);
        synth.setSubLevel(0.0f);
        synth.setLpfCutoff(0.45f); // ~425 Hz base cutoff
        synth.setLpfResonance(resonance);
        synth.setEnvMod(0.0f);
        synth.setDecay(1.0f);
        synth.setAccent(0.0f);
        synth.processMidiNoteOn(36, 100); // C2 = 65.406 Hz

        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
        synth.processAudio(context);

        const double f0 = 440.0 * std::pow(2.0, (36 - 69) / 12.0);
        const auto harmonicMagnitude = [&](int harmonic) {
            double re = 0.0;
            double im = 0.0;
            const uint32_t start = 1024;
            const uint32_t length = frameCount - start;
            for (uint32_t i = 0; i < length; i++) {
                const double w = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(length - 1)));
                const double x = buffer[static_cast<size_t>(start + i) * 2] * w;
                const double phase = 2.0 * std::numbers::pi * (static_cast<double>(harmonic) * f0) * static_cast<double>(i) / static_cast<double>(sampleRate);
                re += x * std::cos(phase);
                im -= x * std::sin(phase);
            }
            return std::hypot(re, im) / static_cast<double>(length);
        };

        const double h1 = harmonicMagnitude(1);
        double maxResonantHarmonic = 0.0;
        for (int h = 5; h <= 9; h++) {
            maxResonantHarmonic = std::max(maxResonantHarmonic, harmonicMagnitude(h));
        }

        // Verify high frequencies (near Nyquist/4) do not contain runaway unit-delay self-oscillation
        const double hHigh = harmonicMagnitude(120); // ~7.8 kHz
        return std::tuple { h1, maxResonantHarmonic / std::max(h1, 1e-9), hHigh };
    };

    const auto [h1Res0, ratioRes0, highRes0] = measureHarmonicRatio(0.0f);
    const auto [h1Res2, ratioRes2, highRes2] = measureHarmonicRatio(0.2f);
    const auto [h1Res5, ratioRes5, highRes5] = measureHarmonicRatio(0.5f);
    const auto [h1Res8, ratioRes8, highRes8] = measureHarmonicRatio(0.8f);
    const auto [h1Res10, ratioRes10, highRes10] = measureHarmonicRatio(1.0f);

    Q_UNUSED(highRes0);
    Q_UNUSED(highRes2);
    // Resonant harmonic ratio must monotonically increase across TD-3-MO knob positions 0, 2, 5, 8, 10
    QVERIFY(ratioRes2 > ratioRes0);
    QVERIFY(ratioRes5 > ratioRes2);
    QVERIFY(ratioRes8 > ratioRes5);
    QVERIFY(ratioRes10 > ratioRes8);

    // Passband fundamental at max resonance should remain within ~12 dB of zero resonance (TD-3-MO drops ~8.5 dB)
    const double h1DropDb = 20.0 * std::log10(h1Res10 / h1Res0);
    QVERIFY2(h1DropDb > -12.0 && h1DropDb < -4.0, QString("Unexpected H1 drop at max resonance: %1 dB").arg(h1DropDb).toUtf8().constData());

    // No runaway ~8 kHz self-oscillation at high resonance
    QVERIFY(highRes5 < h1Res5 * 0.1);
    QVERIFY(highRes8 < h1Res8 * 0.1);
    QVERIFY(highRes10 < h1Res10 * 0.1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BassSynthTest)
