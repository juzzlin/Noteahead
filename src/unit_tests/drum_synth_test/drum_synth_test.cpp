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
#include "common/constants.hpp"
#include "domain/devices/drum_synth_device.hpp"
#include "domain/dsp/drum/clap_engine.hpp"
#include "domain/dsp/drum/crash_engine.hpp"
#include "domain/dsp/drum/hihat_engine.hpp"
#include "domain/dsp/drum/kick_engine.hpp"
#include "domain/dsp/drum/ride_engine.hpp"
#include "domain/dsp/drum/snare_engine.hpp"
#include "domain/dsp/drum/tom_engine.hpp"
#include "infra/xml/nahd_xml_reader.hpp"
#include "infra/xml/nahd_xml_writer.hpp"
#include "repro_kick_pop.cpp"
#include <QTest>

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

void DrumSynthTest::test_kick_start_shouldNotHaveDiscontinuity()
{
    ReproKickPop::test_kick_start_shouldNotHaveDiscontinuity();
}

void DrumSynthTest::test_kick_retrigger_shouldNotPop()
{
    ReproKickPop::test_kick_retrigger_shouldNotPop();
}

void DrumSynthTest::test_kick_small_attack_shouldNotPop()
{
    ReproKickPop::test_kick_small_attack_shouldNotPop();
}

void DrumSynthTest::test_kickEngine_peakVolume_shouldNotExceedOne()
{
    KickEngine engine;
    engine.setSampleRate(44100);

    for (float attack : { 0.0f, 0.5f, 1.0f }) {
        engine.setAttack(attack);
        engine.trigger(1.0f);
        float peak = 0.0f;
        for (int i = 0; i < 1000; i++) {
            peak = std::max(peak, std::abs(engine.nextSample()));
        }
        QVERIFY(peak <= 1.01f);
    }
}

void DrumSynthTest::test_tomEngine_peakVolume_shouldNotExceedOne()
{
    TomEngine engine;
    engine.setSampleRate(44100);
    engine.trigger(1.0f);
    float peak = 0.0f;
    for (int i = 0; i < 1000; i++) {
        peak = std::max(peak, std::abs(engine.nextSample()));
    }
    QVERIFY(peak <= 1.01f);
}

void DrumSynthTest::test_snareEngine_peakVolume_shouldNotExceedOne()
{
    SnareEngine engine;
    engine.setSampleRate(44100);
    for (float snappy : { 0.0f, 0.5f, 1.0f }) {
        engine.setSnappy(snappy);
        engine.trigger(1.0f);
        float peak = 0.0f;
        for (int i = 0; i < 1000; i++) {
            peak = std::max(peak, std::abs(engine.nextSample()));
        }
        QVERIFY(peak <= 1.01f);
    }
}

void DrumSynthTest::test_hihatEngine_peakVolume_shouldNotExceedOne()
{
    HiHatEngine engine;
    engine.setSampleRate(44100);
    engine.trigger(1.0f);
    float peak = 0.0f;
    for (int i = 0; i < 1000; i++) {
        peak = std::max(peak, std::abs(engine.nextSample()));
    }
    QVERIFY(peak <= 1.01f);
}

void DrumSynthTest::test_crashEngine_peakVolume_shouldNotExceedOne()
{
    CrashEngine engine;
    engine.setSampleRate(44100);
    engine.trigger(1.0f);
    float peak = 0.0f;
    for (int i = 0; i < 1000; i++) {
        peak = std::max(peak, std::abs(engine.nextSample()));
    }
    QVERIFY(peak <= 1.01f);
}

void DrumSynthTest::test_rideEngine_peakVolume_shouldNotExceedOne()
{
    RideEngine engine;
    engine.setSampleRate(44100);
    engine.trigger(1.0f);
    float peak = 0.0f;
    for (int i = 0; i < 1000; i++) {
        peak = std::max(peak, std::abs(engine.nextSample()));
    }
    QVERIFY(peak <= 1.01f);
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

void DrumSynthTest::test_drumSynthDevice_midiNoteOn_shouldTriggerVoice()
{
    DrumSynthDevice device("Test");
    device.processMidiNoteOn(36, 127); // Kick
    std::vector<double> buffer(20, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 10, 44100 };
    device.processAudio(context);
    bool foundSound = false;
    for (size_t i = 0; i < buffer.size(); i++) {
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

    if (auto p = device.parameter("Voice0_" + Constants::NahdXml::xmlKeyTune().toStdString()); p)
        p->get().setValue(0.75f);
    if (auto p = device.parameter("Voice1_" + Constants::NahdXml::xmlKeySnappy().toStdString()); p)
        p->get().setValue(0.25f);

    QString xml;
    NahdXmlWriter writer { xml };
    device.serializeToXml(writer);

    DrumSynthDevice restored("Restored");
    NahdXmlReader reader { xml };
    while (!reader.atEnd() && !reader.isStartElement())
        reader.readNext();
    restored.deserializeFromXml(reader);

    if (auto p = restored.parameter("Voice0_" + Constants::NahdXml::xmlKeyTune().toStdString()); p)
        QCOMPARE(p->get().value(), 0.75f);
    if (auto p = restored.parameter("Voice1_" + Constants::NahdXml::xmlKeySnappy().toStdString()); p)
        QCOMPARE(p->get().value(), 0.25f);
}

void DrumSynthTest::test_processMidiCc_shouldUpdateVoicePanLpfHpf()
{
    DrumSynthDevice device("Test");

    // Kick (Voice 0) Pan (CC 14)
    device.processMidiCc(14, 127, 0);
    if (auto p = device.parameter("Voice0_" + Constants::NahdXml::xmlKeyPan().toStdString()); p)
        QCOMPARE(p->get().value(), 1.0f);

    // Kick (Voice 0) LPF (CC 15)
    device.processMidiCc(15, 64, 0);
    if (auto p = device.parameter("Voice0_" + Constants::NahdXml::xmlKeyCutoff().toStdString()); p)
        QVERIFY(std::abs(p->get().value() - 0.5f) < 0.01f);

    // Kick (Voice 0) HPF (CC 16)
    device.processMidiCc(16, 0, 0);
    if (auto p = device.parameter("Voice0_" + Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
        QCOMPARE(p->get().value(), 0.0f);

    // Low Tom (Voice 5) HPF (CC 14 + 5*3 + 2 = CC 31)
    device.processMidiCc(31, 127, 0);
    if (auto p = device.parameter("Voice5_" + Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
        QCOMPARE(p->get().value(), 1.0f);

    // Mid Tom (Voice 6) Pan (CC 102)
    device.processMidiCc(102, 127, 0);
    if (auto p = device.parameter("Voice6_" + Constants::NahdXml::xmlKeyPan().toStdString()); p)
        QCOMPARE(p->get().value(), 1.0f);

    // Reverse Crash (Voice 10) HPF (CC 102 + 4*3 + 2 = CC 116)
    device.processMidiCc(116, 127, 0);
    if (auto p = device.parameter("Voice10_" + Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
        QCOMPARE(p->get().value(), 1.0f);
}

void DrumSynthTest::test_drumSynthDevice_toms_shouldHaveDifferentDefaultTunes()
{
    const DrumSynthDevice device("Test");

    const auto getTune = [&](int voiceIndex) {
        const std::string prefix { DrumSynth::voiceId(voiceIndex) + "_" };
        const auto p { device.parameter(prefix + Constants::NahdXml::xmlKeyTune().toStdString()) };
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
    for (int i { 0 }; i < 10; i++) {
        engine.nextSample();
    }
    const float sample1 { engine.nextSample() };

    engine.reset();
    engine.setTune(0.8f);
    engine.trigger(1.0f);
    for (int i { 0 }; i < 10; i++) {
        engine.nextSample();
    }
    const float sample2 { engine.nextSample() };

    QVERIFY(sample1 != sample2);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DrumSynthTest)
