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

#include "drum_synth_v2_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/drum_synth_v2_constants.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

constexpr uint32_t frameCount = 4096;
constexpr uint32_t sampleRate = 44100;

//! Triggers one note on a freshly built device and hands back the interleaved output. Templated over
//! the device so that V1 and V2 are driven through exactly the same steps, which is the whole point
//! of the comparison below.
template<typename DeviceT>
std::vector<double> renderNote(uint8_t note, uint32_t blocks = 1)
{
    DeviceT device { "Drum Synth" };
    device.processMidiNoteOn(note, 100);
    std::vector<double> rendered;
    rendered.reserve(static_cast<size_t>(blocks) * frameCount * 2);
    std::vector<double> buffer(frameCount * 2, 0.0);
    for (uint32_t block = 0; block < blocks; block++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
        device.processAudio(context);
        rendered.insert(rendered.end(), buffer.begin(), buffer.end());
    }
    return rendered;
}

//! Long enough for every voice to have run out, cymbals included: the amp envelope's defaults sit at
//! the far end of the tail, so a short render would compare the two devices where they cannot yet
//! differ.
constexpr uint32_t fullTailBlocks = 80; // ~7.4 s

} // namespace

void DrumSynthV2Test::test_drumSynthV2Device_typeId_shouldDifferFromV1()
{
    // A shared type id would have the factory hand back whichever device was registered last, and
    // every V2 in every project would silently load as a V1.
    QVERIFY(DrumSynthV2Device::typeIdString() != DrumSynthDevice::typeIdString());
    QVERIFY(DrumSynthV2Device { "Test" }.typeName() != DrumSynthDevice { "Test" }.typeName());
}

void DrumSynthV2Test::test_drumSynthV2Device_everyVoice_shouldStayCloseToV1()
{
    // V2 shares V1's drum engines and adds an amp envelope whose defaults are parked just past each
    // voice's own tail, so the two are meant to be the same kit with V2 a touch tighter -- not the
    // same waveform. What is asserted is therefore a distance rather than equality: the difference
    // has to stay far enough below the voice itself that nobody would call it a different drum.
    const DrumSynthV2Device notes { "Notes" };
    for (int voice = 0; voice < DrumSynthV2::NumVoices; voice++) {
        const auto note = notes.voiceNote(voice);
        const auto v1 = renderNote<DrumSynthDevice>(note, fullTailBlocks);
        const auto v2 = renderNote<DrumSynthV2Device>(note, fullTailBlocks);
        QCOMPARE(v1.size(), v2.size());

        double signal = 0.0;
        double difference = 0.0;
        double peak1 = 0.0;
        double peak2 = 0.0;
        for (size_t i = 0; i < v1.size(); i++) {
            signal += v1.at(i) * v1.at(i);
            difference += (v2.at(i) - v1.at(i)) * (v2.at(i) - v1.at(i));
            peak1 = std::max(peak1, std::abs(v1.at(i)));
            peak2 = std::max(peak2, std::abs(v2.at(i)));
        }
        const auto db = [](double power) { return 10.0 * std::log10(std::max(power, 1.0e-30)); };
        const double differenceDb = db(difference) - db(signal);

        qInfo("%-14s difference %6.1f dB below the voice, peak %.4f vs %.4f",
              qPrintable(DrumSynthV2::voiceName(voice)), -differenceDb, peak1, peak2);

        QVERIFY2(differenceDb < -20.0,
                 qPrintable(QString { "%1 drifted too far from V1: %2 dB" }.arg(DrumSynthV2::voiceName(voice)).arg(differenceDb)));
        // The transient is what names a drum, and the envelope opens instantly by default, so the
        // peak must not move at all.
        QVERIFY2(std::abs(peak1 - peak2) < 1.0e-9,
                 qPrintable(QString { "%1 changed peak: %2 vs %3" }.arg(DrumSynthV2::voiceName(voice)).arg(peak1).arg(peak2)));
    }
}

void DrumSynthV2Test::test_drumSynthV2Device_midiNoteOn_shouldTriggerVoice()
{
    DrumSynthV2Device device { "Test" };
    QVERIFY(!device.hasActiveAudio());
    device.processMidiNoteOn(static_cast<uint8_t>(DrumSynthV2::MidiNote::Kick), 100);
    QVERIFY(device.hasActiveAudio());
}

//! Where the rendered signal last rises above -60 dBFS, in seconds.
namespace {

double tailLength(const std::vector<double> & rendered, uint32_t rate)
{
    double last = 0.0;
    for (size_t frame = 0; frame * 2 + 1 < rendered.size(); frame++) {
        const auto level = std::max(std::abs(rendered.at(frame * 2)), std::abs(rendered.at(frame * 2 + 1)));
        if (level > 0.001) {
            last = static_cast<double>(frame) / rate;
        }
    }
    return last;
}

} // namespace

void DrumSynthV2Test::test_ampEnvelope_shortHold_shouldTightenTheVoice()
{
    // The point of the stage: the kick's own engine still rings for well over a second, and pulling
    // Hold down has to cut that short rather than merely turn it down.
    const DrumSynthV2Device notes { "Notes" };
    const auto kickNote = notes.voiceNote(static_cast<int>(DrumSynthV2::VoiceIndex::Kick));

    DrumSynthV2Device device { "Test" };
    const auto asRendered = [&](uint32_t blocks) {
        device.resetAudio();
        device.processMidiNoteOn(kickNote, 100);
        std::vector<double> rendered;
        std::vector<double> buffer(frameCount * 2, 0.0);
        for (uint32_t block = 0; block < blocks; block++) {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
            device.processAudio(context);
            rendered.insert(rendered.end(), buffer.begin(), buffer.end());
        }
        return rendered;
    };

    const auto asShipped = tailLength(asRendered(fullTailBlocks), sampleRate);
    QVERIFY2(asShipped > 0.9, qPrintable(QString { "the kick is already short: %1 s" }.arg(asShipped)));

    // Hold is mapped cubically over eight seconds, so this is 64 ms.
    const std::string holdKey { Constants::NahdXml::xmlKeyAmpHold().toStdString() };
    device.updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), holdKey, 0.2f);
    const std::string decayKey { Constants::NahdXml::xmlKeyAmpDecay().toStdString() };
    device.updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), decayKey, 0.2f);

    const auto tightened = tailLength(asRendered(fullTailBlocks), sampleRate);
    QVERIFY2(tightened < 0.2, qPrintable(QString { "Hold did not tighten the kick: %1 s, was %2 s" }.arg(tightened).arg(asShipped)));
}

void DrumSynthV2Test::test_ampEnvelope_closed_shouldStopTheVoiceRendering()
{
    // A closed envelope has to take the voice out of hasActiveAudio(), or the device keeps being
    // rendered for the whole of an engine tail nobody can hear any more.
    const DrumSynthV2Device notes { "Notes" };
    DrumSynthV2Device device { "Test" };
    const auto kick = static_cast<int>(DrumSynthV2::VoiceIndex::Kick);
    device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpHold().toStdString(), 0.1f);
    device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpDecay().toStdString(), 0.1f);

    device.processMidiNoteOn(notes.voiceNote(kick), 100);
    QVERIFY(device.hasActiveAudio());

    std::vector<double> buffer(frameCount * 2, 0.0);
    for (int block = 0; block < 10; block++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
        device.processAudio(context);
    }

    QVERIFY2(!device.hasActiveAudio(), "the voice kept rendering after its envelope had closed");
}

void DrumSynthV2Test::test_ampEnvelope_curve_shouldBendTheVoicesDecay()
{
    // The envelope's own curve is covered in drum_amp_envelope_test. What this covers is the wiring:
    // that the per-voice parameter actually reaches the envelope the voice is played through, which
    // no amount of the DSP being correct would tell us.
    const DrumSynthV2Device notes { "Notes" };
    const auto kick = static_cast<int>(DrumSynthV2::VoiceIndex::Kick);
    const auto kickNote = notes.voiceNote(kick);

    const auto levelPartWayIntoTheDecay = [&](float curve) {
        DrumSynthV2Device device { "Test" };
        // A hold short enough that the decay is under way well inside the render, and a decay long
        // enough that a straight one is still going at the end of it.
        device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpHold().toStdString(), 0.05f);
        device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpDecay().toStdString(), 0.75f);
        device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpCurve().toStdString(), curve);
        device.processMidiNoteOn(kickNote, 100);

        std::vector<double> buffer(frameCount * 2, 0.0);
        double peak = 0.0;
        for (int block = 0; block < 4; block++) {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
            device.processAudio(context);
        }
        // The last block, by which point the decay has had a while to separate the two shapes.
        for (uint32_t i = 0; i < frameCount; i++) {
            peak = std::max(peak, std::max(std::abs(buffer[i * 2]), std::abs(buffer[i * 2 + 1])));
        }
        return peak;
    };

    const auto straight = levelPartWayIntoTheDecay(0.0f);
    const auto bent = levelPartWayIntoTheDecay(1.0f);

    QVERIFY2(straight > 0.0, "the straight decay had already finished, so there is nothing to compare");
    QVERIFY2(bent < straight * 0.5,
             qPrintable(QString { "the voice's curve did not reach its envelope: %1 vs %2" }.arg(bent).arg(straight)));
}

void DrumSynthV2Test::test_drumSynthV2Device_xmlSerialization_shouldRestoreParameters()
{
    DrumSynthV2Device device { "Test" };

    const std::string tuneKey { DrumSynthV2::voiceId(0) + "_" + Constants::NahdXml::xmlKeyTune().toStdString() };
    const std::string snappyKey { DrumSynthV2::voiceId(1) + "_" + Constants::NahdXml::xmlKeySnappy().toStdString() };
    if (auto p = device.parameter(tuneKey); p) {
        p->get().setValue(0.75f);
    }
    if (auto p = device.parameter(snappyKey); p) {
        p->get().setValue(0.25f);
    }

    QString xml;
    NahdXmlWriter writer { xml };
    device.serializeToXml(writer);

    DrumSynthV2Device restored { "Restored" };
    NahdXmlReader reader { xml };
    while (!reader.atEnd() && !reader.isStartElement()) {
        reader.readNext();
    }
    restored.deserializeFromXml(reader);

    const auto tune = restored.parameter(tuneKey);
    QVERIFY(tune.has_value());
    QCOMPARE(tune->get().value(), 0.75f);
    const auto snappy = restored.parameter(snappyKey);
    QVERIFY(snappy.has_value());
    QCOMPARE(snappy->get().value(), 0.25f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DrumSynthV2Test)
