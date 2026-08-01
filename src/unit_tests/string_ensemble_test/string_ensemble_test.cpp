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

#include "string_ensemble_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/string_ensemble_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QBuffer>
#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 44100 };

std::vector<double> render(StringEnsembleDevice & device, uint32_t frames)
{
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), frames, SampleRate };
    device.processAudio(context);
    return buffer;
}

double peakLevel(const std::vector<double> & buffer)
{
    double peak = 0.0;
    for (const auto sample : buffer) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

double rmsLevel(const std::vector<double> & buffer)
{
    double sum = 0.0;
    for (const auto sample : buffer) {
        sum += sample * sample;
    }
    return std::sqrt(sum / static_cast<double>(buffer.size()));
}

//! Strongest frequency of the left channel, resolved by direct evaluation of a coarse spectrum.
double dominantFrequency(const std::vector<double> & buffer, int bins)
{
    const int frames = static_cast<int>(buffer.size() / 2);
    double bestFrequency = 0.0;
    double bestMagnitude = 0.0;
    for (int bin = 1; bin < bins; bin++) {
        double re = 0.0;
        double im = 0.0;
        for (int frame = 0; frame < frames; frame++) {
            const double angle = 2.0 * std::numbers::pi * bin * frame / frames;
            re += buffer[static_cast<size_t>(frame) * 2] * std::cos(angle);
            im -= buffer[static_cast<size_t>(frame) * 2] * std::sin(angle);
        }
        if (const double magnitude = std::sqrt(re * re + im * im); magnitude > bestMagnitude) {
            bestMagnitude = magnitude;
            bestFrequency = static_cast<double>(bin) * SampleRate / frames;
        }
    }
    return bestFrequency;
}

//! A device with one register selected, no modulation and an instant attack, ready to be measured.
void configureForMeasurement(StringEnsembleDevice & device)
{
    device.setSampleRate(SampleRate);
    device.setViolaEnabled(false);
    device.setViolinEnabled(false);
    device.setModulationEnabled(false);
    device.setCrescendo(0.0f);
}

//! Renders past the attack and returns a settled window.
std::vector<double> renderSettled(StringEnsembleDevice & device, uint32_t frames = 4096)
{
    render(device, 8192);
    return render(device, frames);
}

} // namespace

void StringEnsembleTest::test_midiNoteOn_shouldActivateAudio()
{
    StringEnsembleDevice device { "Test StringEnsemble" };
    device.setSampleRate(SampleRate);
    QVERIFY(!device.hasActiveAudio());

    device.processMidiNoteOn(60, 127);
    QVERIFY(device.hasActiveAudio());

    QVERIFY(peakLevel(renderSettled(device)) > 0.0);
}

void StringEnsembleTest::test_midiNoteOff_shouldDecayToSilence()
{
    StringEnsembleDevice device { "Test StringEnsemble" };
    device.setSampleRate(SampleRate);
    device.setCrescendo(0.0f);
    device.setSustainLength(0.0f);

    device.processMidiNoteOn(60, 127);
    render(device, 8192);
    QVERIFY(peakLevel(render(device, 1024)) > 0.0);

    device.processMidiNoteOff(60);
    render(device, SampleRate);

    QCOMPARE(peakLevel(render(device, 1024)), 0.0);
}

void StringEnsembleTest::test_allNotesOff_shouldSilenceAllVoices()
{
    StringEnsembleDevice device { "Test StringEnsemble" };
    device.setSampleRate(SampleRate);
    device.setCrescendo(0.0f);
    device.setSustainLength(0.0f);

    for (uint8_t note : { 48, 55, 60, 64, 67 }) {
        device.processMidiNoteOn(note, 127);
    }
    render(device, 8192);

    device.processMidiAllNotesOff();
    render(device, SampleRate);

    QCOMPARE(peakLevel(render(device, 1024)), 0.0);
    QVERIFY(!device.hasActiveAudio());
}

void StringEnsembleTest::test_registers_allDisabledShouldBeSilent()
{
    StringEnsembleDevice device { "Test StringEnsemble" };
    configureForMeasurement(device);

    device.processMidiNoteOn(60, 127);

    QCOMPARE(peakLevel(renderSettled(device)), 0.0);
}

void StringEnsembleTest::test_registers_violinShouldSoundAnOctaveAboveViola()
{
    StringEnsembleDevice viola { "Viola" };
    configureForMeasurement(viola);
    viola.setViolaEnabled(true);
    viola.processMidiNoteOn(60, 127);
    const double violaFrequency = dominantFrequency(renderSettled(viola), 200);

    StringEnsembleDevice violin { "Violin" };
    configureForMeasurement(violin);
    violin.setViolinEnabled(true);
    violin.processMidiNoteOn(60, 127);
    const double violinFrequency = dominantFrequency(renderSettled(violin), 200);

    QVERIFY(violaFrequency > 0.0);
    QVERIFY2(std::abs(violinFrequency - 2.0 * violaFrequency) < 20.0,
             QString("Viola %1 Hz, Violin %2 Hz").arg(violaFrequency).arg(violinFrequency).toUtf8().constData());
}

void StringEnsembleTest::test_registers_hornShouldSoundAnOctaveBelowViola()
{
    StringEnsembleDevice viola { "Viola" };
    configureForMeasurement(viola);
    viola.setViolaEnabled(true);
    viola.processMidiNoteOn(60, 127);
    const double violaFrequency = dominantFrequency(renderSettled(viola), 200);

    StringEnsembleDevice horn { "Horn" };
    configureForMeasurement(horn);
    horn.setHornEnabled(true);
    horn.processMidiNoteOn(60, 127);
    const double hornFrequency = dominantFrequency(renderSettled(horn), 200);

    QVERIFY(hornFrequency > 0.0);
    QVERIFY2(std::abs(hornFrequency - 0.5 * violaFrequency) < 20.0,
             QString("Viola %1 Hz, Horn %2 Hz").arg(violaFrequency).arg(hornFrequency).toUtf8().constData());
}

void StringEnsembleTest::test_bassSplit_belowSplitShouldReachBassSectionOnly()
{
    StringEnsembleDevice bass { "Bass" };
    configureForMeasurement(bass);
    bass.setCelloEnabled(true);
    bass.processMidiNoteOn(StringEnsembleDevice::SplitNote - 1, 127);
    QVERIFY(peakLevel(renderSettled(bass)) > 0.0);

    StringEnsembleDevice upper { "Upper" };
    configureForMeasurement(upper);
    upper.setViolaEnabled(true);
    upper.processMidiNoteOn(StringEnsembleDevice::SplitNote - 1, 127);
    QCOMPARE(peakLevel(renderSettled(upper)), 0.0);
}

void StringEnsembleTest::test_bassSplit_atOrAboveSplitShouldReachUpperSectionOnly()
{
    StringEnsembleDevice upper { "Upper" };
    configureForMeasurement(upper);
    upper.setViolaEnabled(true);
    upper.processMidiNoteOn(StringEnsembleDevice::SplitNote, 127);
    QVERIFY(peakLevel(renderSettled(upper)) > 0.0);

    StringEnsembleDevice bass { "Bass" };
    configureForMeasurement(bass);
    bass.setCelloEnabled(true);
    bass.processMidiNoteOn(StringEnsembleDevice::SplitNote, 127);
    QCOMPARE(peakLevel(renderSettled(bass)), 0.0);
}

void StringEnsembleTest::test_volumeBass_shouldScaleBassSection()
{
    const auto renderAtVolumeBass = [](float volumeBass) {
        StringEnsembleDevice device { "Bass" };
        configureForMeasurement(device);
        device.setCelloEnabled(true);
        device.setVolumeBass(volumeBass);
        device.processMidiNoteOn(StringEnsembleDevice::SplitNote - 12, 127);
        return rmsLevel(renderSettled(device));
    };

    const double full = renderAtVolumeBass(1.0f);
    const double half = renderAtVolumeBass(0.5f);

    QVERIFY(full > 0.0);
    QVERIFY2(std::abs(half - full * 0.5) < full * 0.05,
             QString("Full %1, half %2").arg(full).arg(half).toUtf8().constData());
    QCOMPARE(renderAtVolumeBass(0.0f), 0.0);
}

void StringEnsembleTest::test_velocity_shouldScaleLevel()
{
    const auto renderAtVelocity = [](uint8_t velocity) {
        StringEnsembleDevice device { "Test" };
        configureForMeasurement(device);
        device.setViolaEnabled(true);
        device.processMidiNoteOn(60, velocity);
        return rmsLevel(renderSettled(device));
    };

    const double loud = renderAtVelocity(127);
    const double soft = renderAtVelocity(64);

    QVERIFY(loud > 0.0);
    QVERIFY(soft < loud * 0.6);
}

void StringEnsembleTest::test_velocitySensitivity_zeroShouldIgnoreVelocity()
{
    const auto renderAtVelocity = [](uint8_t velocity) {
        StringEnsembleDevice device { "Test" };
        configureForMeasurement(device);
        device.setViolaEnabled(true);
        device.setVelocitySensitivity(0.0f);
        device.processMidiNoteOn(60, velocity);
        return rmsLevel(renderSettled(device));
    };

    QCOMPARE(renderAtVelocity(64), renderAtVelocity(127));
}

void StringEnsembleTest::test_crescendo_longAttackShouldRampUp()
{
    StringEnsembleDevice device { "Test" };
    device.setSampleRate(SampleRate);
    device.setModulationEnabled(false);
    device.setCrescendo(1.0f);

    device.processMidiNoteOn(60, 127);

    const double early = rmsLevel(render(device, 4096));
    render(device, SampleRate);
    const double late = rmsLevel(render(device, 4096));

    QVERIFY(late > early * 2.0);
}

void StringEnsembleTest::test_polyphony_shouldNotClipWithManyNotes()
{
    StringEnsembleDevice device { "Test" };
    device.setSampleRate(SampleRate);
    device.setCrescendo(0.0f);
    device.setContrabassEnabled(true);
    device.setCelloEnabled(true);
    device.setTrumpetEnabled(true);
    device.setHornEnabled(true);
    device.setPhaserEnabled(true);

    for (uint8_t note = 36; note < 84; note += 3) {
        device.processMidiNoteOn(note, 127);
    }

    QVERIFY(peakLevel(renderSettled(device)) < 1.0);
}

void StringEnsembleTest::test_modulation_shouldCreateStereoSeparation()
{
    StringEnsembleDevice device { "Test" };
    device.setSampleRate(SampleRate);
    device.setCrescendo(0.0f);
    device.setModulationEnabled(true);
    device.processMidiNoteOn(60, 127);

    const auto buffer = renderSettled(device);

    double difference = 0.0;
    for (size_t frame = 0; frame < buffer.size() / 2; frame++) {
        difference = std::max(difference, std::abs(buffer[frame * 2] - buffer[frame * 2 + 1]));
    }

    QVERIFY(difference > 0.001);
}

void StringEnsembleTest::test_phaser_shouldAlterSignal()
{
    const auto renderWithPhaser = [](bool enabled) {
        StringEnsembleDevice device { "Test" };
        configureForMeasurement(device);
        device.setViolaEnabled(true);
        device.setPhaserEnabled(enabled);
        device.processMidiNoteOn(60, 127);
        return renderSettled(device);
    };

    const auto dry = renderWithPhaser(false);
    const auto wet = renderWithPhaser(true);

    double difference = 0.0;
    for (size_t i = 0; i < dry.size(); i++) {
        difference = std::max(difference, std::abs(dry[i] - wet[i]));
    }

    QVERIFY(difference > 0.001);
}

void StringEnsembleTest::test_hpfAndLpf_shouldAttenuateSignal()
{
    StringEnsembleDevice open { "Open" };
    configureForMeasurement(open);
    open.setViolaEnabled(true);
    open.processMidiNoteOn(60, 127);
    const double openLevel = rmsLevel(renderSettled(open));

    StringEnsembleDevice lowPassed { "LowPassed" };
    configureForMeasurement(lowPassed);
    lowPassed.setViolaEnabled(true);
    lowPassed.setLpfCutoff(0.1f);
    lowPassed.processMidiNoteOn(60, 127);
    QVERIFY(rmsLevel(renderSettled(lowPassed)) < openLevel);

    StringEnsembleDevice highPassed { "HighPassed" };
    configureForMeasurement(highPassed);
    highPassed.setViolaEnabled(true);
    highPassed.setHpfCutoff(0.9f);
    highPassed.processMidiNoteOn(60, 127);
    QVERIFY(rmsLevel(renderSettled(highPassed)) < openLevel);
}

void StringEnsembleTest::test_midiCc_shouldUpdateVolumeAndPan()
{
    StringEnsembleDevice device { "Test StringEnsemble" };
    device.setSampleRate(SampleRate);

    const auto controllers = device.availableMidiCcControllers();
    QCOMPARE(controllers.size(), size_t { 2 });
    QCOMPARE(controllers.at(0).name, std::string { "Volume" });
    QCOMPARE(controllers.at(1).name, std::string { "Pan" });

    device.processMidiCc(controllers.at(0).number, 64, 0);
    QVERIFY(std::abs(device.volume() - Device::faderPositionFromMidiCc(64)) < 0.001f);

    device.processMidiCc(controllers.at(1).number, 127, 0);
    QVERIFY(std::abs(device.pan() - 1.0f) < 0.001f);
}

void StringEnsembleTest::test_serialization_shouldRestoreParameters()
{
    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);

    {
        NahdXmlWriter writer { buffer };
        StringEnsembleDevice device { "Test StringEnsemble" };
        device.setContrabassEnabled(true);
        device.setCelloEnabled(true);
        device.setViolaEnabled(false);
        device.setViolinEnabled(false);
        device.setTrumpetEnabled(true);
        device.setHornEnabled(true);
        device.setModulationEnabled(false);
        device.setPhaserEnabled(true);
        device.setVolumeBass(0.42f);
        device.setCrescendo(0.66f);
        device.setSustainLength(0.77f);
        device.setPhaserColor(0.25f);
        device.setPhaserRate(0.8f);
        device.setVelocitySensitivity(0.3f);
        device.setLpfCutoff(0.65f);
        device.setHpfCutoff(0.15f);
        device.serializeToXml(writer);
    }

    buffer.close();
    buffer.open(QIODevice::ReadOnly);

    {
        NahdXmlReader reader { buffer };
        StringEnsembleDevice device { "Restored StringEnsemble" };

        QVERIFY(reader.readNextStartElement());
        QCOMPARE(reader.name(), Constants::NahdXml::xmlKeyDevice());

        device.deserializeFromXml(reader);

        QCOMPARE(device.contrabassEnabled(), true);
        QCOMPARE(device.celloEnabled(), true);
        QCOMPARE(device.violaEnabled(), false);
        QCOMPARE(device.violinEnabled(), false);
        QCOMPARE(device.trumpetEnabled(), true);
        QCOMPARE(device.hornEnabled(), true);
        QCOMPARE(device.modulationEnabled(), false);
        QCOMPARE(device.phaserEnabled(), true);
        QCOMPARE(device.volumeBass(), 0.42f);
        QCOMPARE(device.crescendo(), 0.66f);
        QCOMPARE(device.sustainLength(), 0.77f);
        QCOMPARE(device.phaserColor(), 0.25f);
        QCOMPARE(device.phaserRate(), 0.8f);
        QCOMPARE(device.velocitySensitivity(), 0.3f);
        QCOMPARE(device.lpfCutoff(), 0.65f);
        QCOMPARE(device.hpfCutoff(), 0.15f);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::StringEnsembleTest)
