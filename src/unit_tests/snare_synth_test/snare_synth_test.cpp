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

#include "snare_synth_test.hpp"
#include "domain/devices/snare_synth_device.hpp"
#include "domain/devices/snare_synth_presets.hpp"
#include "infra/midi/midi_cc_mapping.hpp"
#include "infra/xml/nahd_xml_reader.hpp"
#include "infra/xml/nahd_xml_writer.hpp"

#include <QSignalSpy>
#include <QTest>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate = 44100;

std::vector<double> renderDeviceAudio(SnareSynthDevice & device, uint32_t frames)
{
    std::vector<double> buffer(frames * 2, 0.0);
    AudioContext context {
        .buffer = buffer,
        .frameCount = frames,
        .sampleRate = SampleRate
    };
    device.processAudio(context);
    return buffer;
}

} // namespace

void SnareSynthTest::test_midiNoteOn_shouldActivateAudio()
{
    SnareSynthDevice device { "Test Snare" };
    QCOMPARE(device.hasActiveAudio(), false);

    device.processDeviceMidiNoteOn(38, 100);
    QCOMPARE(device.hasActiveAudio(), true);

    const auto buffer = renderDeviceAudio(device, 2048);
    double maxVal = 0.0;
    for (double sample : buffer) {
        maxVal = std::max(maxVal, std::abs(sample));
    }
    QVERIFY(maxVal > 0.001);
}

void SnareSynthTest::test_midiNoteOff_shouldNotStopRinging()
{
    SnareSynthDevice device { "Test Snare" };
    device.processDeviceMidiNoteOn(38, 100);
    QCOMPARE(device.hasActiveAudio(), true);

    device.processDeviceMidiNoteOff(38);
    QCOMPARE(device.hasActiveAudio(), true);
}

void SnareSynthTest::test_allNotesOff_shouldChokeVoice()
{
    SnareSynthDevice device { "Test Snare" };
    device.processDeviceMidiNoteOn(38, 100);
    QCOMPARE(device.hasActiveAudio(), true);

    device.processMidiAllNotesOff();
    QCOMPARE(device.hasActiveAudio(), false);
}

void SnareSynthTest::test_panSpread_zero_shouldBeStrictlyMono()
{
    SnareSynthDevice device { "Test Snare" };
    device.setPanSpread(0.0f);
    device.setSnappy(0.8f);

    device.processDeviceMidiNoteOn(38, 100);
    const auto buffer = renderDeviceAudio(device, 2048);

    for (size_t i = 0; i < 2048; i++) {
        const double left = buffer[i * 2];
        const double right = buffer[i * 2 + 1];
        QVERIFY(std::abs(left - right) < 1.0e-6);
    }
}

void SnareSynthTest::test_panSpread_full_shouldCreateStereoSeparation()
{
    SnareSynthDevice device { "Test Snare" };
    device.setPanSpread(1.0f);
    device.setSnappy(0.8f);

    device.processDeviceMidiNoteOn(38, 100);
    const auto buffer = renderDeviceAudio(device, 2048);

    double maxDiff = 0.0;
    for (size_t i = 0; i < 2048; i++) {
        const double left = buffer[i * 2];
        const double right = buffer[i * 2 + 1];
        maxDiff = std::max(maxDiff, std::abs(left - right));
    }

    QVERIFY(maxDiff > 0.001);
}

void SnareSynthTest::test_keyTrack_shouldFollowPitchOnlyWhenEnabled()
{
    SnareSynthDevice devKeyTrackOn { "KeyTrackOn" };
    devKeyTrackOn.setKeyTrack(true);
    devKeyTrackOn.setSnappy(0.0f);
    devKeyTrackOn.processDeviceMidiNoteOn(38, 100);
    const auto buf38 = renderDeviceAudio(devKeyTrackOn, 1024);

    devKeyTrackOn.resetAudio();
    devKeyTrackOn.processDeviceMidiNoteOn(50, 100);
    const auto buf50 = renderDeviceAudio(devKeyTrackOn, 1024);

    double diffOn = 0.0;
    for (size_t i = 0; i < 1024 * 2; i++) {
        diffOn = std::max(diffOn, std::abs(buf38[i] - buf50[i]));
    }
    QVERIFY(diffOn > 0.01);

    SnareSynthDevice devKeyTrackOff { "KeyTrackOff" };
    devKeyTrackOff.setKeyTrack(false);
    devKeyTrackOff.setSnappy(0.0f);
    devKeyTrackOff.processDeviceMidiNoteOn(38, 100);
    const auto bufOff38 = renderDeviceAudio(devKeyTrackOff, 1024);

    devKeyTrackOff.resetAudio();
    devKeyTrackOff.processDeviceMidiNoteOn(50, 100);
    const auto bufOff50 = renderDeviceAudio(devKeyTrackOff, 1024);

    for (size_t i = 0; i < 1024 * 2; i++) {
        QCOMPARE(bufOff38[i], bufOff50[i]);
    }
}

void SnareSynthTest::test_filters_lpf_and_hpf_shouldAttenuate()
{
    SnareSynthDevice devFull { "Full" };
    devFull.setLpfCutoff(1.0f);
    devFull.setHpfCutoff(0.0f);
    devFull.processDeviceMidiNoteOn(38, 100);
    const auto bufFull = renderDeviceAudio(devFull, 2048);

    SnareSynthDevice devFiltered { "Filtered" };
    devFiltered.setLpfCutoff(0.3f);
    devFiltered.setHpfCutoff(0.2f);
    devFiltered.processDeviceMidiNoteOn(38, 100);
    const auto bufFiltered = renderDeviceAudio(devFiltered, 2048);

    double energyFull = 0.0;
    double energyFiltered = 0.0;
    for (size_t i = 0; i < 2048 * 2; i++) {
        energyFull += bufFull[i] * bufFull[i];
        energyFiltered += bufFiltered[i] * bufFiltered[i];
    }

    QVERIFY(energyFiltered < energyFull);
}

void SnareSynthTest::test_snareBuzz_shouldAffectSustainRattle()
{
    SnareSynthDevice devClean { "Clean" };
    devClean.setSnareBuzz(0.0f);
    devClean.setSnareDecay(0.1f);
    devClean.setSnappy(0.8f);
    devClean.processDeviceMidiNoteOn(38, 100);
    const auto bufClean = renderDeviceAudio(devClean, 8192);

    SnareSynthDevice devBuzzy { "Buzzy" };
    devBuzzy.setSnareBuzz(1.0f);
    devBuzzy.setSnareDecay(0.1f);
    devBuzzy.setSnappy(0.8f);
    devBuzzy.processDeviceMidiNoteOn(38, 100);
    const auto bufBuzzy = renderDeviceAudio(devBuzzy, 8192);

    double cleanTailEnergy = 0.0;
    double buzzyTailEnergy = 0.0;
    for (size_t i = 4410 * 2; i < 8192 * 2; i++) {
        cleanTailEnergy += bufClean[i] * bufClean[i];
        buzzyTailEnergy += bufBuzzy[i] * bufBuzzy[i];
    }

    QVERIFY(buzzyTailEnergy > cleanTailEnergy * 1.5);
}

void SnareSynthTest::test_click_shouldAffectAttackTransient()
{
    SnareSynthDevice devSoft { "Soft" };
    devSoft.setClick(0.0f);
    devSoft.setSnappy(0.0f);
    devSoft.processDeviceMidiNoteOn(38, 100);
    const auto bufSoft = renderDeviceAudio(devSoft, 256);

    SnareSynthDevice devPunch { "Punch" };
    devPunch.setClick(1.0f);
    devPunch.setSnappy(0.0f);
    devPunch.processDeviceMidiNoteOn(38, 100);
    const auto bufPunch = renderDeviceAudio(devPunch, 256);

    double maxDiff = 0.0;
    for (size_t i = 0; i < 256 * 2; i++) {
        maxDiff = std::max(maxDiff, std::abs(bufPunch[i] - bufSoft[i]));
    }

    QVERIFY(maxDiff > 0.2);
}

void SnareSynthTest::test_hardness_shouldAffectTimbreAndOvertones()
{
    SnareSynthDevice devFelt { "Felt" };
    devFelt.setHardness(0.0f);
    devFelt.setSnappy(0.0f);
    devFelt.processDeviceMidiNoteOn(38, 100);
    const auto bufFelt = renderDeviceAudio(devFelt, 1024);

    SnareSynthDevice devHard { "Hard" };
    devHard.setHardness(1.0f);
    devHard.setSnappy(0.0f);
    devHard.processDeviceMidiNoteOn(38, 100);
    const auto bufHard = renderDeviceAudio(devHard, 1024);

    double maxDiff = 0.0;
    for (size_t i = 0; i < 1024 * 2; i++) {
        maxDiff = std::max(maxDiff, std::abs(bufFelt[i] - bufHard[i]));
    }

    QVERIFY(maxDiff > 0.03);
}

void SnareSynthTest::test_body_shouldAffectShellWeightAndSustain()
{
    SnareSynthDevice devPiccolo { "Piccolo" };
    devPiccolo.setBody(0.0f);
    devPiccolo.setSnappy(0.0f);
    devPiccolo.processDeviceMidiNoteOn(38, 100);
    const auto bufPiccolo = renderDeviceAudio(devPiccolo, 2048);

    SnareSynthDevice devDeep { "Deep" };
    devDeep.setBody(1.0f);
    devDeep.setSnappy(0.0f);
    devDeep.processDeviceMidiNoteOn(38, 100);
    const auto bufDeep = renderDeviceAudio(devDeep, 2048);

    double maxDiff = 0.0;
    double piccoloTailEnergy = 0.0;
    double deepTailEnergy = 0.0;
    for (size_t i = 0; i < 2048 * 2; i++) {
        maxDiff = std::max(maxDiff, std::abs(bufDeep[i] - bufPiccolo[i]));
        if (i >= 512 * 2) {
            piccoloTailEnergy += bufPiccolo[i] * bufPiccolo[i];
            deepTailEnergy += bufDeep[i] * bufDeep[i];
        }
    }

    QVERIFY(maxDiff > 0.1);
    QVERIFY(deepTailEnergy > piccoloTailEnergy * 1.5);
}

void SnareSynthTest::test_midiCc_shouldReachParameters()
{
    SnareSynthDevice device { "Test Snare" };
    QSignalSpy spy { &device, &Device::parametersChanged };

    device.processDeviceMidiCc(static_cast<uint8_t>(MidiCcMapping::Controller::SoundController4), 100, 0); // Snappy
    QCOMPARE(spy.count(), 1);
    QVERIFY(device.snappy() > 0.7f);
}

void SnareSynthTest::test_serialization_shouldRestoreParameters()
{
    SnareSynthDevice device { "Test Snare" };
    device.setTune(0.75f);
    device.setTone(0.65f);
    device.setDecay(0.45f);
    device.setHeadTension(0.6f);
    device.setPitchDepth(0.4f);
    device.setPitchDecay(0.3f);
    device.setSnappy(0.85f);
    device.setSnareDecay(0.55f);
    device.setSnareTension(0.7f);
    device.setSnareBuzz(0.6f);
    device.setClick(0.5f);
    device.setHardness(0.65f);
    device.setPosition(0.35f);
    device.setBody(0.5f);
    device.setDrive(0.3f);
    device.setPanSpread(0.8f);
    device.setKeyTrack(true);
    device.setVelocitySensitivity(0.9f);
    device.setLpfCutoff(0.8f);
    device.setHpfCutoff(0.15f);

    QString xml;
    NahdXmlWriter writer { xml };
    device.serializeToXml(writer);

    SnareSynthDevice restored { "Restored Snare" };
    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        restored.deserializeFromXml(reader);
    }

    QCOMPARE(restored.tune(), 0.75f);
    QCOMPARE(restored.tone(), 0.65f);
    QCOMPARE(restored.decay(), 0.45f);
    QCOMPARE(restored.headTension(), 0.6f);
    QCOMPARE(restored.pitchDepth(), 0.4f);
    QCOMPARE(restored.pitchDecay(), 0.3f);
    QCOMPARE(restored.snappy(), 0.85f);
    QCOMPARE(restored.snareDecay(), 0.55f);
    QCOMPARE(restored.snareTension(), 0.7f);
    QCOMPARE(restored.snareBuzz(), 0.6f);
    QCOMPARE(restored.click(), 0.5f);
    QCOMPARE(restored.hardness(), 0.65f);
    QCOMPARE(restored.position(), 0.35f);
    QCOMPARE(restored.body(), 0.5f);
    QCOMPARE(restored.drive(), 0.3f);
    QCOMPARE(restored.panSpread(), 0.8f);
    QCOMPARE(restored.keyTrack(), true);
    QCOMPARE(restored.velocitySensitivity(), 0.9f);
    QCOMPARE(restored.lpfCutoff(), 0.8f);
    QCOMPARE(restored.hpfCutoff(), 0.15f);
}

void SnareSynthTest::test_presetInit_shouldResetParameters()
{
    SnareSynthDevice device { "Test Snare" };
    const float defaultTune = device.tune();
    const float defaultSnappy = device.snappy();

    device.setTune(0.9f);
    device.setSnappy(0.1f);
    QCOMPARE(device.tune(), 0.9f);
    QCOMPARE(device.snappy(), 0.1f);

    device.loadPreset(0); // "Init"
    QCOMPARE(device.tune(), defaultTune);
    QCOMPARE(device.snappy(), defaultSnappy);
}

void SnareSynthTest::test_presets_hit_shouldBeAudibleAndUnclipped()
{
    const auto & presets = SnareSynthPresets::presets();
    QVERIFY(presets.size() > 1);
    QCOMPARE(presets.front().name, std::string { "Init" });
    for (size_t i = 0; i < presets.size(); i++) {
        const auto name = QString::fromStdString(presets[i].name);
        SnareSynthDevice device { "Test Snare" };
        for (auto && [key, value] : presets[i].parameters) {
            QVERIFY2(device.parameter(key).has_value(), qPrintable(name + ": " + QString::fromStdString(key)));
            QVERIFY2(value >= 0.0f && value <= 1.0f, qPrintable(name + ": " + QString::fromStdString(key)));
        }
        device.loadPreset(static_cast<int>(i));
        device.processDeviceMidiNoteOn(38, 127);
        double peak = 0.0;
        for (double sample : renderDeviceAudio(device, SampleRate)) {
            QVERIFY2(std::isfinite(sample), qPrintable(name));
            peak = std::max(peak, std::abs(sample));
        }
        const double db = 20.0 * std::log10(peak);
        QVERIFY2(db > -40.0 && db < -1.0, qPrintable(name + ": " + QString::number(db)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SnareSynthTest)
