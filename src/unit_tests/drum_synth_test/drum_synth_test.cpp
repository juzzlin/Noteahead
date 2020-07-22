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

#include "drum_synth_test.hpp"
#include "repro_kick_pop.cpp"
#include "../../domain/dsp/drum/kick_engine.hpp"
#include "../../domain/dsp/drum/snare_engine.hpp"
#include "../../domain/dsp/drum/hihat_engine.hpp"
#include "../../domain/dsp/drum/crash_engine.hpp"
#include "../../domain/dsp/drum/ride_engine.hpp"
#include "../../domain/dsp/drum/tom_engine.hpp"
#include "../../domain/dsp/drum/clap_engine.hpp"
#include "../../domain/devices/drum_synth_device.hpp"

namespace noteahead {

void DrumSynthTest::test_kickEngine_attack_shouldAddClick()
{
    KickEngine engine;
    engine.setSampleRate(44100.0);
    
    // Test with no attack (no click)
    engine.setAttack(0.0f);
    engine.trigger(1.0f);
    engine.nextSample(); // Skip first (zero) sample
    float sampleNoClick = engine.nextSample();
    
    KickEngine engine2;
    engine2.setSampleRate(44100.0);
    engine2.setAttack(1.0f);
    engine2.trigger(1.0f);
    engine2.nextSample(); // Skip first (zero) sample
    float sampleWithClick = engine2.nextSample();
    
    QVERIFY(std::abs(sampleWithClick - sampleNoClick) > 0.001f);
}

void DrumSynthTest::test_kickEngine_trigger_shouldBeActive()
{
    KickEngine engine;
    QVERIFY(!engine.isActive());
    engine.trigger(1.0f);
    QVERIFY(engine.isActive());
}

void DrumSynthTest::test_kickEngine_nextSample_shouldEventuallyDeactivate()
{
    KickEngine engine;
    engine.setSampleRate(44100.0);
    engine.setDecay(0.01f); // Very fast decay for test
    engine.trigger(1.0f);
    
    int iterations { 0 };
    while (engine.isActive() && iterations < 100000) {
        engine.nextSample();
        iterations++;
    }
    QVERIFY(!engine.isActive());
}

void DrumSynthTest::test_kick_start_discontinuity()
{
    ReproKickPop::test_kick_start_discontinuity();
}

void DrumSynthTest::test_kick_retrigger_pop()
{
    ReproKickPop::test_kick_retrigger_pop();
}

void DrumSynthTest::test_kick_small_attack_pop()
{
    ReproKickPop::test_kick_small_attack_pop();
}

void DrumSynthTest::test_snareEngine_trigger_shouldBeActive()
{
    SnareEngine engine;
    engine.trigger(1.0f);
    QVERIFY(engine.isActive());
}

void DrumSynthTest::test_hihatEngine_trigger_shouldBeActive()
{
    HiHatEngine engine;
    engine.trigger(1.0f);
    QVERIFY(engine.isActive());
}

void DrumSynthTest::test_crashEngine_trigger_shouldBeActive()
{
    CrashEngine engine;
    engine.trigger(1.0f);
    QVERIFY(engine.isActive());
}

void DrumSynthTest::test_rideEngine_trigger_shouldBeActive()
{
    RideEngine engine;
    engine.trigger(1.0f);
    QVERIFY(engine.isActive());
}

void DrumSynthTest::test_tomEngine_trigger_shouldBeActive()
{
    TomEngine engine;
    engine.trigger(1.0f);
    QVERIFY(engine.isActive());
}

void DrumSynthTest::test_clapEngine_trigger_shouldBeActive()
{
    ClapEngine engine;
    engine.trigger(1.0f);
    QVERIFY(engine.isActive());
}

void DrumSynthTest::test_drumSynthDevice_midiNoteOn_shouldTriggerPad()
{
    DrumSynthDevice device("Test");
    device.processMidiNoteOn(36, 127); // Kick
    std::vector<float> buffer(20, 0.0f);
    device.processAudio(buffer.data(), 10, 44100);
    bool foundSound = false;
    for (size_t i = 0; i < buffer.size(); ++i) {
        if (buffer.at(i) != 0.0f) {
            foundSound = true;
            break;
        }
    }
    QVERIFY(foundSound);
}

void DrumSynthTest::test_drumSynthDevice_xmlSerialization_shouldRestoreParameters()
{
    DrumSynthDevice device("Test");
    
    if (auto p = device.parameter("Pad0_tune"); p) p->get().setValue(0.75f);
    if (auto p = device.parameter("Pad1_snappy"); p) p->get().setValue(0.25f);

    QString xml;
    QXmlStreamWriter writer(&xml);
    device.serializeToXml(writer);

    DrumSynthDevice restored("Restored");
    QXmlStreamReader reader(xml);
    while (!reader.atEnd() && !reader.isStartElement()) reader.readNext();
    restored.deserializeFromXml(reader);

    if (auto p = restored.parameter("Pad0_tune"); p) QCOMPARE(p->get().value(), 0.75f);
    if (auto p = restored.parameter("Pad1_snappy"); p) QCOMPARE(p->get().value(), 0.25f);
}

void DrumSynthTest::test_drumSynthDevice_midiCc_shouldUpdateSelectedPadParameters()
{
    DrumSynthDevice device("Test");
    
    device.processMidiCc(70, 0, 0);
    QCOMPARE(device.selectedPad(), 0);

    device.processMidiCc(75, 127, 0);
    if (auto p = device.parameter("Pad0_tune"); p) QCOMPARE(p->get().value(), 1.0f);

    device.processMidiCc(70, 1, 0);
    QCOMPARE(device.selectedPad(), 1);

    device.processMidiCc(77, 64, 0);
    if (auto p = device.parameter("Pad1_snappy"); p) QVERIFY(std::abs(p->get().value() - 0.5f) < 0.01f);
}

void DrumSynthTest::test_drumSynthDevice_toms_shouldHaveDifferentDefaultTunes()
{
    const DrumSynthDevice device("Test");
    
    const auto getTune = [&](int padIndex) {
        const std::string prefix { "Pad" + std::to_string(padIndex) + "_" };
        const auto p { device.parameter(prefix + "tune") };
        return p ? p->get().value() : -1.0f;
    };

    const float lowTomTune { getTune(5) };
    const float midTomTune { getTune(6) };
    const float hiTomTune { getTune(7) };

    QVERIFY(lowTomTune != midTomTune);
    QVERIFY(midTomTune != hiTomTune);
    QVERIFY(lowTomTune != hiTomTune);
}

void DrumSynthTest::test_tomEngine_tunes_shouldSoundDifferent()
{
    TomEngine engine;
    engine.setSampleRate(44100);
    
    engine.setTune(0.2f);
    engine.trigger(1.0f);
    for (int i { 0 }; i < 10; ++i) {
        engine.nextSample();
    }
    const float sample1 { engine.nextSample() };

    engine.reset();
    engine.setTune(0.8f);
    engine.trigger(1.0f);
    for (int i { 0 }; i < 10; ++i) {
        engine.nextSample();
    }
    const float sample2 { engine.nextSample() };

    QVERIFY(sample1 != sample2);
}

} // namespace noteahead

QTEST_MAIN(noteahead::DrumSynthTest)
