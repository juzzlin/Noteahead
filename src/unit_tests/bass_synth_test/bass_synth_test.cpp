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

#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"

#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

void BassSynthTest::test_serialization_shouldRestoreParameters()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.setLpfCutoff(0.75f);
    synth.setWaveform(PolyBlepOscillator::Waveform::Square);
    synth.setSubLevel(0.5f);

    QString xml;
    QXmlStreamWriter writer { &xml };
    synth.serializeToXml(writer);

    BassSynthDevice synth2 { "Restored BassSynth" };
    QXmlStreamReader reader { xml };
    if (reader.readNextStartElement()) {
        synth2.deserializeFromXml(reader);
    }

    QCOMPARE(synth2.lpfCutoff(), 0.75f);
    QCOMPARE(static_cast<int>(synth2.waveform()), static_cast<int>(PolyBlepOscillator::Waveform::Square));
    QCOMPARE(synth2.subLevel(), 0.5f);
}

void BassSynthTest::test_midiProcessing_shouldTriggerAudio()
{
    BassSynthDevice synth { "Test BassSynth" };
    synth.processMidiNoteOn(60, 100);
    QVERIFY(synth.hasActiveAudio());

    synth.processMidiNoteOff(60);
    // Audio might still be active due to release, but let's test all notes off
    synth.processMidiAllNotesOff();
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
    synth.setWaveform(PolyBlepOscillator::Waveform::Square);
    synth.setSubLevel(0.0f);
    synth.setDistDrive(0.0f);
    synth.setLpfCutoff(1.0f); // Wide open

    synth.processMidiNoteOn(60, 100);

    const int frameCount { 1000 };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
    synth.processAudio(context);

    double peak { 0.0 };
    for (double sample : buffer) {
        peak = std::max(peak, std::abs(sample));
    }

    // Square wave peak should be 1.0. Panning center multiplier is 2.0 * 0.5 = 1.0.
    // So peak should be around 1.0.
    // (Previous bug had 0.5 * 0.5 = 0.25)
    // Updated: Diode ladder has some gain even at 0 resonance, peak around 1.1 is fine.
    QVERIFY2(peak > 0.9f && peak < 1.3f, QString("Peak level incorrect: %1").arg(static_cast<double>(peak)).toUtf8().constData());
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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BassSynthTest)
