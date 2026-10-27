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

//! Seconds until the left channel last reaches 40 dB under its peak.
double decaySeconds(const std::vector<double> & buffer)
{
    double peak = 0.0;
    for (size_t i = 0; i < buffer.size(); i += 2) {
        peak = std::max(peak, std::abs(buffer[i]));
    }
    size_t last = 0;
    for (size_t i = 0; i < buffer.size(); i += 2) {
        if (std::abs(buffer[i]) > peak * 0.01) {
            last = i / 2;
        }
    }
    return static_cast<double>(last) / SampleRate;
}

//! RMS of the first difference over the RMS of the left channel: a brightness measure.
double brightness(const std::vector<double> & buffer)
{
    double sum = 0.0;
    double differenceSum = 0.0;
    for (size_t i = 2; i < buffer.size(); i += 2) {
        sum += buffer[i] * buffer[i];
        const double difference = buffer[i] - buffer[i - 2];
        differenceSum += difference * difference;
    }
    return std::sqrt(differenceSum / sum);
}

std::vector<double> renderChromaticHit(uint8_t note, bool chromatic, uint32_t frames)
{
    SnareSynthDevice device { "Chromatic" };
    device.setChromatic(chromatic);
    device.processDeviceMidiNoteOn(note, 100);
    return renderDeviceAudio(device, frames);
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

void SnareSynthTest::test_chromatic_shouldScaleDecayWithNote_data()
{
    QTest::addColumn<bool>("chromatic");
    QTest::addColumn<int>("note");
    QTest::addColumn<double>("expectedRatio");

    QTest::newRow("off, octave up") << false << 48 << 1.0;
    QTest::newRow("on, octave up") << true << 48 << 0.5;
    QTest::newRow("on, octave down") << true << 24 << 2.0;
}

void SnareSynthTest::test_chromatic_shouldScaleDecayWithNote()
{
    QFETCH(bool, chromatic);
    QFETCH(int, note);
    QFETCH(double, expectedRatio);

    const double reference = decaySeconds(renderChromaticHit(36, chromatic, SampleRate * 3));
    const double played = decaySeconds(renderChromaticHit(static_cast<uint8_t>(note), chromatic, SampleRate * 3));
    const double ratio = played / reference;
    QVERIFY2(std::abs(ratio / expectedRatio - 1.0) < 0.15, qPrintable(QString { "%1 s -> %2 s" }.arg(reference).arg(played)));
}

void SnareSynthTest::test_chromatic_shouldScaleBrightnessWithNote()
{
    const double reference = brightness(renderChromaticHit(36, true, SampleRate / 4));
    const double octaveUp = brightness(renderChromaticHit(48, true, SampleRate / 4));
    QVERIFY2(octaveUp / reference > 1.5, qPrintable(QString { "%1 -> %2" }.arg(reference).arg(octaveUp)));
}

void SnareSynthTest::test_chromatic_c3_shouldMatchOff()
{
    QCOMPARE(renderChromaticHit(36, true, 4096), renderChromaticHit(36, false, 4096));
}

void SnareSynthTest::test_chromatic_octave_shouldBeIgnored()
{
    SnareSynthDevice shifted { "Shifted" };
    shifted.setChromatic(true);
    shifted.setOctave(2);
    QCOMPARE(shifted.octave(), 0);
    shifted.processMidiNoteOn(36, 100);
    QCOMPARE(renderDeviceAudio(shifted, 4096), renderChromaticHit(36, false, 4096));
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
    device.setChromatic(true);
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
    QCOMPARE(restored.chromatic(), true);
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
