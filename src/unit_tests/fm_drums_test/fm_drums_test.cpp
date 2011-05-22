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

#include "fm_drums_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"
#include "../../domain/devices/fm_drums_device.hpp"
#include "../../domain/devices/fm_drums_presets.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QRegularExpression>
#include <QTest>

#include <algorithm>
#include <cmath>
#include <set>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };
constexpr double MaxSeconds { 8.0 };

std::string key(int pad, const QString & name)
{
    return DrumSynthV2::voiceId(pad) + "_" + name.toStdString();
}

double peak(const std::vector<double> & frames)
{
    double result = 0.0;
    for (const double sample : frames) {
        result = std::max(result, std::abs(sample));
    }
    return result;
}

//! RMS of the strike, which is what sets how loud a drum sits in a beat.
double strikeRms(const std::vector<double> & frames)
{
    const auto count = std::min(frames.size(), static_cast<size_t>(0.1 * SampleRate) * 2);
    double sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        sum += frames.at(i) * frames.at(i);
    }
    return count ? std::sqrt(sum / static_cast<double>(count)) : 0.0;
}

QString toXml(const FmDrumsDevice & device)
{
    QString xml;
    NahdXmlWriter writer { xml };
    device.serializeToXml(writer);
    return xml;
}

void fromXml(FmDrumsDevice & device, const QString & xml)
{
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    device.deserializeFromXml(reader);
}

} // namespace

void FmDrumsTest::test_typeId_shouldDifferFromV2()
{
    const FmDrumsDevice device { "FM Drums" };

    QCOMPARE(device.typeId(), FmDrumsDevice::typeIdString());
    QVERIFY(device.typeId() != DrumSynthV2Device::typeIdString());
    QCOMPARE(device.typeName(), Constants::fmDrumsDeviceName().toStdString());
    QCOMPARE(device.category(), Constants::NahdXml::xmlValueDrums().toStdString());
}

void FmDrumsTest::test_pads_shouldPlayOnTheNotesV2Does()
{
    // The same layout, so that a beat written for one plays on the other
    const FmDrumsDevice fm { "FM Drums" };
    const DrumSynthV2Device v2 { "Drum Synth V2" };

    for (int pad = 0; pad < DrumSynthV2::NumVoices; pad++) {
        QCOMPARE(fm.voiceNote(pad), v2.voiceNote(pad));
    }
}

void FmDrumsTest::test_everyPad_shouldSoundAndThenStop()
{
    FmDrumsDevice device { "FM Drums" };

    for (int pad = 0; pad < DrumSynthV2::NumVoices; pad++) {
        const auto frames = device.renderVoiceAlone(pad, SampleRate, MaxSeconds);
        const auto name = DrumSynthV2::voiceName(pad);
        QVERIFY2(peak(frames) > 0.1, qPrintable(name + QString { " peaks at %1" }.arg(peak(frames))));
        QVERIFY2(peak(frames) < 0.7, qPrintable(name + QString { " peaks at %1" }.arg(peak(frames))));
        QVERIFY2(frames.size() < static_cast<size_t>(MaxSeconds * SampleRate) * 2, qPrintable(name + " never stopped"));
    }
}

void FmDrumsTest::test_everyPad_shouldSitLevelWithV2()
{
    // Swapping one kit for the other should not move the mix. The two crashes are left out: V2's
    // blooms after the strike and its reverse one has barely started by then, so the strike is not
    // where either is heard.
    FmDrumsDevice fm { "FM Drums" };
    DrumSynthV2Device v2 { "Drum Synth V2" };

    for (int pad = 0; pad < DrumSynthV2::NumVoices; pad++) {
        if (pad == static_cast<int>(DrumSynthV2::VoiceIndex::Crash) || pad == static_cast<int>(DrumSynthV2::VoiceIndex::ReverseCrash)) {
            continue;
        }
        const double fmRms = strikeRms(fm.renderVoiceAlone(pad, SampleRate, MaxSeconds));
        const double v2Rms = strikeRms(v2.renderVoiceAlone(pad, SampleRate, MaxSeconds));
        const double difference = 20.0 * std::log10(fmRms / v2Rms);
        QVERIFY2(std::abs(difference) < 3.0, qPrintable(DrumSynthV2::voiceName(pad) + QString { " is %1 dB off V2" }.arg(difference)));
    }
}

void FmDrumsTest::test_renderVoiceAlone_shouldBeDeterministic()
{
    FmDrumsDevice device { "FM Drums" };
    const int snare = static_cast<int>(DrumSynthV2::VoiceIndex::Snare);

    QCOMPARE(device.renderVoiceAlone(snare, SampleRate, MaxSeconds), device.renderVoiceAlone(snare, SampleRate, MaxSeconds));
}

void FmDrumsTest::test_closedHiHat_shouldChokeTheOpenOne()
{
    FmDrumsDevice device { "FM Drums" };
    const int openHat = static_cast<int>(DrumSynthV2::VoiceIndex::OpenHiHat);

    std::vector<double> buffer(512 * 2, 0.0);
    const auto run = [&](double seconds) {
        for (int block = 0; block < static_cast<int>(seconds * SampleRate / 512.0); block++) {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            AudioContext context { std::span(buffer.data(), buffer.size()), 512, static_cast<uint32_t>(SampleRate) };
            device.processAudio(context);
        }
    };

    device.processMidiNoteOn(static_cast<uint8_t>(DrumSynthV2::MidiNote::OpenHiHat), 100);
    run(0.05);
    QVERIFY(device.voiceElapsedSeconds(openHat).has_value());

    device.processMidiNoteOn(static_cast<uint8_t>(DrumSynthV2::MidiNote::ClosedHiHat), 100);
    run(0.05);
    QVERIFY(!device.voiceElapsedSeconds(openHat).has_value());
}

void FmDrumsTest::test_algorithm_shouldReachTheEngine()
{
    FmDrumsDevice device { "FM Drums" };
    const int kick = static_cast<int>(DrumSynthV2::VoiceIndex::Kick);
    const auto serial = device.renderVoiceAlone(kick, SampleRate, 1.0);

    // Additive: the kick's modulator becomes a carrier of its own
    QVERIFY(device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAlgorithm().toStdString(), 7.0f));

    QVERIFY(device.renderVoiceAlone(kick, SampleRate, 1.0) != serial);
}

void FmDrumsTest::test_automatableParameters_shouldOfferEveryContinuousControl()
{
    const FmDrumsDevice device { "FM Drums" };
    std::set<std::string> automatable;
    for (auto && entry : device.automatableParameters()) {
        automatable.insert(entry.key);
        // Every drum's own, and the device's channel strip besides
        const bool channelStrip = entry.key == Constants::NahdXml::xmlKeyFader().toStdString() || entry.key == Constants::NahdXml::xmlKeyGain().toStdString() || entry.key == Constants::NahdXml::xmlKeyPan().toStdString();
        QVERIFY2(!entry.groupName.empty() || channelStrip, entry.key.c_str());
    }

    namespace NahdXml = Constants::NahdXml;
    for (int pad = 0; pad < DrumSynthV2::NumVoices; pad++) {
        std::vector<QString> names { NahdXml::xmlKeyFrequency(), NahdXml::xmlKeyFeedback(), NahdXml::xmlKeyPitchDepth(), NahdXml::xmlKeyPitchDecay(),
                                     NahdXml::xmlKeyNoiseLevel(), NahdXml::xmlKeyNoiseModulation(), NahdXml::xmlKeyNoiseAttack(), NahdXml::xmlKeyNoiseDecay(),
                                     NahdXml::xmlKeyNoiseCurve(), NahdXml::xmlKeyNoiseCutoff(), NahdXml::xmlKeyNoiseResonance(),
                                     NahdXml::xmlKeyVelocitySensitivity(), NahdXml::xmlKeyModVelocitySensitivity() };
        for (size_t op = 0; op < FmDrumEngine::OperatorCount; op++) {
            for (auto && name : { NahdXml::xmlKeyOperatorRatio(op), NahdXml::xmlKeyOperatorLevel(op), NahdXml::xmlKeyOperatorAttack(op), NahdXml::xmlKeyOperatorDecay(op), NahdXml::xmlKeyOperatorCurve(op) }) {
                names.push_back(name);
            }
        }
        for (auto && name : names) {
            QVERIFY2(automatable.contains(key(pad, name)), key(pad, name).c_str());
        }
    }

    // Read out in the units the drum is played in, rather than as a share of the knob
    const auto pitch = device.parameter(key(0, NahdXml::xmlKeyFrequency()));
    QVERIFY(pitch && pitch->get().display());
    QCOMPARE(QString::fromStdString(pitch->get().display()->unit), QString { "Hz" });
    const auto depth = device.parameter(key(0, NahdXml::xmlKeyPitchDepth()));
    QVERIFY(depth && depth->get().display());
    QCOMPARE(QString::fromStdString(depth->get().display()->unit), QString { "st" });
}

void FmDrumsTest::test_xmlSerialization_shouldRestoreTheSound()
{
    namespace NahdXml = Constants::NahdXml;
    FmDrumsDevice original { "FM Drums" };
    const int snare = static_cast<int>(DrumSynthV2::VoiceIndex::Snare);
    original.updateVoiceParameter(snare, NahdXml::xmlKeyAlgorithm().toStdString(), 2.0f);
    original.updateVoiceParameter(snare, NahdXml::xmlKeyOperatorWaveform(1).toStdString(), 5.0f);
    original.updateVoiceParameter(snare, NahdXml::xmlKeyOperatorRatio(2).toStdString(), 0.61f);
    original.updateVoiceParameter(snare, NahdXml::xmlKeyOperatorLevel(2).toStdString(), 0.7f);
    original.updateVoiceParameter(snare, NahdXml::xmlKeyNoiseTarget().toStdString(), 2.0f);
    original.updateVoiceParameter(snare, NahdXml::xmlKeyNoiseFilterMode().toStdString(), 2.0f);
    original.updateVoiceParameter(snare, NahdXml::xmlKeyFrequency().toStdString(), 0.33f);

    FmDrumsDevice restored { "FM Drums" };
    fromXml(restored, toXml(original));

    const auto restoredFrames = restored.renderVoiceAlone(snare, SampleRate, MaxSeconds);
    const auto originalFrames = original.renderVoiceAlone(snare, SampleRate, MaxSeconds);
    QCOMPARE(restoredFrames, originalFrames);
    QCOMPARE(toXml(restored), toXml(original));
    QVERIFY(restored.renderVoiceAlone(snare, SampleRate, MaxSeconds) != FmDrumsDevice { "FM Drums" }.renderVoiceAlone(snare, SampleRate, MaxSeconds));
}

void FmDrumsTest::test_loadKit_everyKit_shouldVoiceEveryPad()
{
    FmDrumsDevice device { "FM Drums" };
    const auto & kits = FmDrumsPresets::kits();
    QVERIFY(kits.size() > 1);

    for (size_t kit = 0; kit < kits.size(); kit++) {
        device.loadKit(kit);
        for (int pad = 0; pad < DrumSynthV2::NumVoices; pad++) {
            const auto frames = device.renderVoiceAlone(pad, SampleRate, MaxSeconds);
            const auto name = QString::fromStdString(kits.at(kit).name) + " " + DrumSynthV2::voiceName(pad);
            QVERIFY2(peak(frames) > 0.05, qPrintable(name + QString { " peaks at %1" }.arg(peak(frames))));
            QVERIFY2(peak(frames) < 0.7, qPrintable(name + QString { " peaks at %1" }.arg(peak(frames))));
            QVERIFY2(frames.size() < static_cast<size_t>(MaxSeconds * SampleRate) * 2, qPrintable(name + " never stopped"));
        }
    }
}

void FmDrumsTest::test_loadKit_default_shouldBeANewDevice()
{
    // A kit is the whole device: whatever was changed before it goes, mix and envelopes included
    namespace NahdXml = Constants::NahdXml;
    FmDrumsDevice device { "FM Drums" };
    device.loadKit(1);
    device.updateVoiceParameter(0, NahdXml::xmlKeyFader().toStdString(), 0.3f);
    device.updateVoiceParameter(4, NahdXml::xmlKeyAmpHold().toStdString(), 0.6f);
    device.setLpfSlope(1);

    device.loadKit(0);

    const auto strip = [](QString xml) {
        return xml.remove(QRegularExpression { " uuid=\"[^\"]*\"" });
    };
    QCOMPARE(strip(toXml(device)), strip(toXml(FmDrumsDevice { "FM Drums" })));
}

void FmDrumsTest::test_loadPadPreset_shouldChangeOnlyThatPadsVoice()
{
    namespace NahdXml = Constants::NahdXml;
    FmDrumsDevice device { "FM Drums" };
    const int snare = static_cast<int>(DrumSynthV2::VoiceIndex::Snare);
    device.updateVoiceParameter(snare, NahdXml::xmlKeyFader().toStdString(), 0.42f);
    device.updateVoiceParameter(snare, NahdXml::xmlKeyAmpHold().toStdString(), 0.33f);
    const auto kickBefore = device.renderVoiceAlone(0, SampleRate, MaxSeconds);

    // The Percussion kit's cowbell, onto the snare's pad
    const auto & presets = FmDrumsPresets::pads();
    const auto cowbell = std::ranges::find_if(presets, [](auto && preset) { return preset.name == "Percussion Cowbell"; });
    QVERIFY(cowbell != presets.end());
    device.loadPadPreset(snare, static_cast<size_t>(cowbell - presets.begin()));

    QCOMPARE(device.voiceParameterValue(snare, NahdXml::xmlKeyAlgorithm().toStdString()), 7.0f);
    QCOMPARE(device.voiceParameterValue(snare, NahdXml::xmlKeyOperatorWaveform(0).toStdString()), static_cast<float>(FmOperator::Waveform::Square));
    QCOMPARE(device.voiceParameterValue(snare, NahdXml::xmlKeyFader().toStdString()), 0.42f);
    QCOMPARE(device.voiceParameterValue(snare, NahdXml::xmlKeyAmpHold().toStdString()), 0.33f);
    QCOMPARE(device.renderVoiceAlone(0, SampleRate, MaxSeconds), kickBefore);
}

void FmDrumsTest::test_presets_everyOperator_shouldStayBelowAliasing()
{
    // Above about 12 kHz an operator's partials fold back differently at each oversampling factor,
    // so a preset would sound one way at one quality setting and another way at the next
    for (auto && preset : FmDrumsPresets::pads()) {
        for (auto && op : preset.voicing.operators) {
            if (op.level > 0.02) {
                const double frequency = preset.voicing.frequency * op.ratio;
                QVERIFY2(frequency <= 12000.0, qPrintable(QString::fromStdString(preset.name) + QString { " has an operator at %1 Hz" }.arg(frequency)));
            }
        }
    }
}

void FmDrumsTest::test_presets_padNames_shouldBeUnique()
{
    std::set<std::string> names;
    for (auto && preset : FmDrumsPresets::pads()) {
        QVERIFY2(names.insert(preset.name).second, preset.name.c_str());
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FmDrumsTest)