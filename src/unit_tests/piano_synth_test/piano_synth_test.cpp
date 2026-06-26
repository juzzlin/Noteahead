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

#include "piano_synth_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/piano_synth_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>
#include <vector>

namespace noteahead {

namespace {

AudioContext makeContext(std::vector<double> & buffer, uint32_t frameCount, uint32_t sampleRate = 44100)
{
    return AudioContext { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
}

double peakLevel(const std::vector<double> & buffer)
{
    double peak = 0.0;
    for (const double s : buffer) {
        peak = std::max(peak, std::abs(s));
    }
    return peak;
}

void renderFrames(PianoSynthDevice & piano, uint32_t frames, uint32_t sampleRate = 44100)
{
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    auto ctx = makeContext(buffer, frames, sampleRate);
    piano.processAudio(ctx);
}

} // namespace

void PianoSynthTest::test_midiNoteOn_shouldActivateAudio()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.processMidiNoteOn(60, 100);
    QVERIFY(piano.hasActiveAudio());
}

void PianoSynthTest::test_midiNoteOff_shouldDecayToSilence()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.setReleaseTime(0.0f); // Shortest possible damper
    piano.processMidiNoteOn(60, 100);

    // Let the string speak briefly
    renderFrames(piano, 512);
    QVERIFY(piano.hasActiveAudio());

    piano.processMidiNoteOff(60);

    // Render enough for the fast damper to quiet the string
    renderFrames(piano, 44100);
    QVERIFY(!piano.hasActiveAudio());
}

void PianoSynthTest::test_polyphony_shouldSupportMultipleSimultaneousNotes()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.processMidiNoteOn(60, 100);
    piano.processMidiNoteOn(64, 100);
    piano.processMidiNoteOn(67, 100);

    const uint32_t frameCount = 256;
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    auto ctx = makeContext(buffer, frameCount);
    piano.processAudio(ctx);

    // Three simultaneous notes should produce higher output than one
    const double peakThree = peakLevel(buffer);

    PianoSynthDevice pianoOne { "Test Piano One" };
    pianoOne.processMidiNoteOn(60, 100);
    std::vector<double> bufferOne(static_cast<size_t>(frameCount) * 2, 0.0);
    auto ctxOne = makeContext(bufferOne, frameCount);
    pianoOne.processAudio(ctxOne);
    const double peakOne = peakLevel(bufferOne);

    QVERIFY2(peakThree > peakOne,
             QString("Three-note peak (%1) not greater than one-note peak (%2)").arg(peakThree).arg(peakOne).toUtf8().constData());
}

void PianoSynthTest::test_sustainPedal_shouldKeepNoteActiveAfterNoteOff()
{
    PianoSynthDevice piano { "Test Piano" };

    // Pedal down (CC 64 >= 64)
    piano.processMidiCc(64, 127, 0);
    piano.processMidiNoteOn(60, 100);
    renderFrames(piano, 256);

    piano.processMidiNoteOff(60);

    // With pedal held, the voice must still be active immediately after note-off
    QVERIFY(piano.hasActiveAudio());
}

void PianoSynthTest::test_sustainPedal_shouldReleaseNoteWhenPedalLifted()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.setReleaseTime(0.0f);

    piano.processMidiCc(64, 127, 0); // pedal down
    piano.processMidiNoteOn(60, 100);
    renderFrames(piano, 256);
    piano.processMidiNoteOff(60);

    // Lift pedal
    piano.processMidiCc(64, 0, 0);

    // Render enough for the damper to silence the string
    renderFrames(piano, 44100);
    QVERIFY(!piano.hasActiveAudio());
}

void PianoSynthTest::test_allNotesOff_shouldSilenceAllVoices()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.setReleaseTime(0.0f);

    piano.processMidiNoteOn(48, 100);
    piano.processMidiNoteOn(60, 100);
    piano.processMidiNoteOn(72, 100);
    renderFrames(piano, 256);

    piano.processMidiAllNotesOff();
    renderFrames(piano, 44100);

    QVERIFY(!piano.hasActiveAudio());
}

void PianoSynthTest::test_serialization_shouldRestoreParameters()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.setBrightness(0.8f);
    piano.setDecay(0.3f);
    piano.setInharmonicity(0.1f);

    QString xml;
    NahdXmlWriter writer { xml };
    piano.serializeToXml(writer);

    PianoSynthDevice piano2 { "Restored Piano" };
    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        piano2.deserializeFromXml(reader);
    }

    QCOMPARE(piano2.brightness(), 0.8f);
    QCOMPARE(piano2.decay(), 0.3f);
    QCOMPARE(piano2.inharmonicity(), 0.1f);
}

void PianoSynthTest::test_velocity_shouldAffectOutputLevel()
{
    auto getPeak = [](uint8_t velocity) {
        PianoSynthDevice piano { "Test Piano" };
        piano.setVolume(1.0f);
        piano.setGain(0.5f);
        piano.processMidiNoteOn(60, velocity);

        const uint32_t frameCount = 512;
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        auto ctx = makeContext(buffer, frameCount);
        piano.processAudio(ctx);
        return peakLevel(buffer);
    };

    const double peakLow = getPeak(30);
    const double peakHigh = getPeak(127);

    QVERIFY2(peakHigh > peakLow,
             QString("Velocity did not affect level: low=%1 high=%2").arg(peakLow).arg(peakHigh).toUtf8().constData());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PianoSynthTest)
