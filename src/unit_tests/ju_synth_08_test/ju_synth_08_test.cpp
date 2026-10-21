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

#include "ju_synth_08_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/ju_synth_08_device.hpp"
#include "../../domain/devices/ju_synth_08_presets.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QBuffer>
#include <QSignalSpy>
#include <QTest>
#include <cmath>
#include <numeric>
#include <vector>

using namespace noteahead;

void JuSynth08Test::test_instantiation()
{
    JuSynth08Device synth { "Test JU Synth 08" };
    QCOMPARE(synth.name(), std::string("Test JU Synth 08"));
    QCOMPARE(synth.category(), std::string("Synth"));
    QVERIFY(!synth.typeId().empty());
    QVERIFY(!synth.typeName().empty());
    QVERIFY(!synth.hasActiveAudio());
}

void JuSynth08Test::test_polyModeNoteOnAndOff()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);

    synth.processDeviceMidiNoteOn(60, 100);
    QVERIFY(synth.hasActiveAudio());

    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 256,
        .sampleRate = 44100,
    };

    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);

    synth.processDeviceMidiNoteOff(60);
}

void JuSynth08Test::test_soloModeLegatoGlide()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);
    synth.setVoiceMode(static_cast<int>(JuSynth08Device::VoiceMode::Solo));
    synth.setPortamento(0.5f);

    synth.processDeviceMidiNoteOn(48, 100);
    std::vector<double> buffer(128 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 128,
        .sampleRate = 44100,
    };
    synth.processAudio(ctx);

    synth.processDeviceMidiNoteOn(60, 100); // legato glide
    std::fill(buffer.begin(), buffer.end(), 0.0);
    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);
}

void JuSynth08Test::test_unisonModeDetune()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);
    synth.setVoiceMode(static_cast<int>(JuSynth08Device::VoiceMode::Unison));
    synth.setUnisonDetune(0.5f);

    synth.processDeviceMidiNoteOn(60, 100);

    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 256,
        .sampleRate = 44100,
    };
    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);
}

void JuSynth08Test::test_vcfSlopeSwitching()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);

    synth.setVcfSlope(static_cast<int>(JuSynth08Device::VcfSlope::Slope12dB));
    QCOMPARE(synth.vcfSlope(), static_cast<int>(JuSynth08Device::VcfSlope::Slope12dB));

    synth.setVcfSlope(static_cast<int>(JuSynth08Device::VcfSlope::Slope24dB));
    QCOMPARE(synth.vcfSlope(), static_cast<int>(JuSynth08Device::VcfSlope::Slope24dB));
}

void JuSynth08Test::test_vco2HardSync()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);
    synth.setSync(true);
    QVERIFY(synth.sync());

    synth.processDeviceMidiNoteOn(60, 100);
    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 256,
        .sampleRate = 44100,
    };
    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);
}

void JuSynth08Test::test_crossModulationFm()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);
    synth.setCrossMod(0.8f);
    QCOMPARE(synth.crossMod(), 0.8f);

    synth.processDeviceMidiNoteOn(60, 100);
    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 256,
        .sampleRate = 44100,
    };
    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);
}

void JuSynth08Test::test_presetLoadingDoesNotDeadlock()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);

    QSignalSpy spy { &synth, &Device::dataChanged };

    const auto presetCount = static_cast<int>(JuSynth08Presets::presets().size());
    QVERIFY(presetCount > 0);

    for (int i = 0; i < presetCount; i++) {
        synth.loadPreset(i);
    }

    QCOMPARE(spy.count(), presetCount);
}

void JuSynth08Test::test_advanceIdle_shouldKeepTheLfoRunning()
{
    const auto ledAfterSilence = [](bool idle) {
        JuSynth08Device synth { "JU08" };
        const auto rate = static_cast<uint32_t>(Constants::defaultSampleRate());
        constexpr uint32_t frames { 3000 };
        std::vector<double> silence(static_cast<size_t>(frames) * 2, 0.0);
        AudioContext context { .buffer = silence, .frameCount = frames, .sampleRate = rate };
        if (idle) {
            synth.advanceIdle(context);
        } else {
            synth.processAudio(context);
        }
        return synth.lfoLed(0);
    };
    const float rendered = ledAfterSilence(false);
    QVERIFY2(rendered > -0.99f, qPrintable(QString::number(rendered)));
    QVERIFY2(std::abs(ledAfterSilence(true) - rendered) < 1.0e-3f, qPrintable(QString::number(ledAfterSilence(true))));
}

void JuSynth08Test::test_xmlSerialization()
{
    JuSynth08Device synth { "JuSynth08Source" };
    synth.setVcfSlope(static_cast<int>(JuSynth08Device::VcfSlope::Slope12dB));
    synth.setCrossMod(0.65f);
    synth.setUnisonDetune(0.42f);

    QString xml;
    {
        NahdXmlWriter writer { xml };
        synth.serializeToXml(writer);
    }

    JuSynth08Device restored { "JuSynth08Restored" };
    NahdXmlReader reader { xml };
    QVERIFY(reader.readNextStartElement());
    QCOMPARE(reader.name(), Constants::NahdXml::xmlKeyDevice());
    restored.deserializeFromXml(reader);

    QCOMPARE(restored.vcfSlope(), static_cast<int>(JuSynth08Device::VcfSlope::Slope12dB));
    QCOMPARE(restored.crossMod(), 0.65f);
    QCOMPARE(restored.unisonDetune(), 0.42f);
}

QTEST_GUILESS_MAIN(JuSynth08Test)
