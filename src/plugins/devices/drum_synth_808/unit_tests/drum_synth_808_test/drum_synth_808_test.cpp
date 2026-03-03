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

#include "drum_synth_808_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/drum/clap_808_engine.hpp"
#include "../../../../../domain/dsp/drum/kick_808_engine.hpp"
#include "../../../../../domain/dsp/drum/metal_808_engine.hpp"
#include "../../../../../domain/dsp/drum/rim_808_engine.hpp"
#include "../../../../../domain/dsp/drum/snare_808_engine.hpp"
#include "../../../../../domain/dsp/drum/tom_808_engine.hpp"
#include "../../../../../infra/xml/nahd_xml_reader.hpp"
#include "../../../../../infra/xml/nahd_xml_writer.hpp"
#include "../../drum_synth_808_device.hpp"

#include <QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };
constexpr double MaxSeconds { 8.0 };

using DrumSynthV2::VoiceIndex;

std::vector<double> render(DrumEngine & engine, double seconds)
{
    engine.setSampleRate(SampleRate);
    engine.trigger(1.0f);
    std::vector<double> out;
    for (size_t i = 0; i < static_cast<size_t>(seconds * SampleRate); i++) {
        out.push_back(static_cast<double>(engine.nextSample()));
    }
    return out;
}

//! The level of the component at @p hz over [from, to) seconds, in decibels
double levelDb(const std::vector<double> & x, double from, double to, double hz)
{
    const auto begin = static_cast<size_t>(from * SampleRate);
    const auto end = std::min(x.size(), static_cast<size_t>(to * SampleRate));
    std::complex<double> sum {};
    for (size_t i = begin; i < end; i++) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i - begin) / static_cast<double>(end - begin));
        sum += x.at(i) * window * std::polar(1.0, -2.0 * std::numbers::pi * hz * static_cast<double>(i) / SampleRate);
    }
    return 20.0 * std::log10(std::abs(sum) + 1e-12);
}

//! Where the strongest component between @p lowHz and @p highHz lies over [from, to) seconds
double strongestHz(const std::vector<double> & x, double from, double to, double lowHz, double highHz, double stepHz = 0.1)
{
    double best = -1000.0;
    double bestHz = 0.0;
    for (double hz = lowHz; hz < highHz; hz += stepHz) {
        if (const double db = levelDb(x, from, to, hz); db > best) {
            best = db;
            bestHz = hz;
        }
    }
    return bestHz;
}

//! The RMS over [from, to) seconds, in decibels
double rmsDb(const std::vector<double> & x, double from, double to)
{
    const auto begin = static_cast<size_t>(from * SampleRate);
    const auto end = std::min(x.size(), static_cast<size_t>(to * SampleRate));
    double sum = 0.0;
    for (size_t i = begin; i < end; i++) {
        sum += x.at(i) * x.at(i);
    }
    return 10.0 * std::log10(sum / static_cast<double>(std::max<size_t>(end - begin, 1)) + 1e-24);
}

//! What is above @p hz, by a second-order high pass, for telling noise from tones
std::vector<double> highPassed(const std::vector<double> & x, double hz)
{
    const double omega = 2.0 * std::numbers::pi * hz / SampleRate;
    const double alpha = std::sin(omega) / std::sqrt(2.0);
    const double cosine = std::cos(omega);
    const double a0 = 1.0 + alpha;
    const double b0 = (1.0 + cosine) / 2.0 / a0;
    const double b1 = -(1.0 + cosine) / a0;
    const double a1 = -2.0 * cosine / a0;
    const double a2 = (1.0 - alpha) / a0;
    std::vector<double> y(x.size());
    double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    for (size_t i = 0; i < x.size(); i++) {
        y.at(i) = b0 * x.at(i) + b1 * x1 + b0 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x.at(i);
        y2 = y1;
        y1 = y.at(i);
    }
    return y;
}

std::string key(int pad, const QString & name)
{
    return DrumSynthV2::voiceId(pad) + "_" + name.toStdString();
}

int pad(VoiceIndex index)
{
    return static_cast<int>(index);
}

//! Each pad's peak against the kick's, in dB
std::array<double, DrumSynthV2::NumVoices> peaksAgainstKick(DrumSynth808Device & device)
{
    std::array<double, DrumSynthV2::NumVoices> result {};
    double kickDb = 0.0;
    for (int index = 0; index < DrumSynthV2::NumVoices; index++) {
        double peak = 0.0;
        for (const double sample : device.renderVoiceAlone(index, SampleRate, MaxSeconds)) {
            peak = std::max(peak, std::abs(sample));
        }
        const double db = 20.0 * std::log10(peak);
        if (index == 0) {
            kickDb = db;
        }
        result.at(static_cast<size_t>(index)) = db - kickDb;
    }
    return result;
}

//! The mix a new device starts with
constexpr std::array<double, DrumSynthV2::NumVoices> MixTargetDb { 0.0, -2.5, -8.0, -3.0, -7.0, -4.0, -4.0, -4.0, -9.0, -8.0, -10.0, -6.0 };

QString toXml(const DrumSynth808Device & device)
{
    QString xml;
    NahdXmlWriter writer { xml };
    device.serializeToXml(writer);
    return xml;
}

void fromXml(DrumSynth808Device & device, const QString & xml)
{
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    device.deserializeFromXml(reader);
}

} // namespace

void DrumSynth808Test::test_pads_shouldHaveTheRd8Knobs()
{
    namespace NahdXml = Constants::NahdXml;
    const DrumSynth808Device device { "808" };
    const auto count = [](VoiceIndex index) { return DrumSynth808Device::voiceKnobs(pad(index)).size(); };
    QCOMPARE(count(VoiceIndex::Kick), size_t { 3 });
    QCOMPARE(count(VoiceIndex::Snare), size_t { 2 });
    QCOMPARE(count(VoiceIndex::LowTom), size_t { 2 });
    QCOMPARE(count(VoiceIndex::MidTom), size_t { 2 });
    QCOMPARE(count(VoiceIndex::HighTom), size_t { 2 });
    QCOMPARE(count(VoiceIndex::Rim), size_t { 1 });
    QCOMPARE(count(VoiceIndex::Clap), size_t { 2 });
    QCOMPARE(count(VoiceIndex::ClosedHiHat), size_t { 1 });
    QCOMPARE(count(VoiceIndex::OpenHiHat), size_t { 2 });
    QCOMPARE(count(VoiceIndex::Crash), size_t { 2 });
    QCOMPARE(count(VoiceIndex::ReverseCrash), size_t { 2 });
    QCOMPARE(count(VoiceIndex::Ride), size_t { 0 });
    for (int index = 0; index < DrumSynthV2::NumVoices; index++) {
        for (auto && knob : DrumSynth808Device::voiceKnobs(index)) {
            const auto parameter = device.parameter(DrumSynthV2::voiceId(index) + "_" + knob.key);
            QVERIFY2(parameter.has_value(), knob.key.c_str());
            if (knob.isSwitch) {
                // Every switch starts on the 808's own voice
                QCOMPARE(parameter->get().value(), 0.0f);
                QCOMPARE(QString::fromStdString(knob.key), NahdXml::xmlKeyVoiceSwitch());
            }
        }
    }
    // Where the RD-8's panel marks its knobs: noon, but for the kick's Tuning at 3 and the clap's
    // Offset and the hats' Tone a little past noon. The kick's Tone rests where Kick 808's does.
    const auto value = [&device](VoiceIndex index, const QString & name) { return device.parameter(key(pad(index), name))->get().value(); };
    QCOMPARE(value(VoiceIndex::Kick, NahdXml::xmlKeyTune()), 0.3f);
    QCOMPARE(value(VoiceIndex::Kick, NahdXml::xmlKeyTone()), 0.35f);
    QCOMPARE(value(VoiceIndex::Kick, NahdXml::xmlKeyDecay()), 0.5f);
    QCOMPARE(value(VoiceIndex::Clap, NahdXml::xmlKeyOffset()), 0.52f);
    QCOMPARE(value(VoiceIndex::ClosedHiHat, NahdXml::xmlKeyTone()), 0.52f);
    QCOMPARE(value(VoiceIndex::OpenHiHat, NahdXml::xmlKeyTone()), 0.52f);
    QCOMPARE(value(VoiceIndex::OpenHiHat, NahdXml::xmlKeyDecay()), 0.5f);
    QCOMPARE(value(VoiceIndex::Snare, NahdXml::xmlKeyTone()), 0.5f);
    QCOMPARE(value(VoiceIndex::Snare, NahdXml::xmlKeySnappy()), 0.5f);
    QCOMPARE(value(VoiceIndex::LowTom, NahdXml::xmlKeyTune()), 0.5f);
    QCOMPARE(value(VoiceIndex::Crash, NahdXml::xmlKeyTone()), 0.5f);
    QCOMPARE(value(VoiceIndex::Crash, NahdXml::xmlKeyDecay()), 0.5f);
}

void DrumSynth808Test::test_voiceName_shouldNameThe808sPads()
{
    const DrumSynth808Device device { "808" };
    QCOMPARE(device.voiceName(pad(VoiceIndex::Crash)), std::string { "Cymbal" });
    QCOMPARE(device.voiceName(pad(VoiceIndex::Ride)), std::string { "Cowbell" });
    QCOMPARE(device.voiceName(pad(VoiceIndex::ReverseCrash)), std::string { "Reverse Cymbal" });
    QCOMPARE(device.voiceName(pad(VoiceIndex::Kick)), std::string { "Kick" });
    bool found = false;
    for (auto && controller : device.deviceMidiCcControllers()) {
        found |= controller.name == "Cowbell Pan";
    }
    QVERIFY(found);
}

void DrumSynth808Test::test_everyPad_shouldSoundAndThenStop()
{
    DrumSynth808Device device { "808" };
    for (int index = 0; index < DrumSynthV2::NumVoices; index++) {
        const auto frames = device.renderVoiceAlone(index, SampleRate, MaxSeconds);
        QVERIFY2(!frames.empty(), qPrintable(QString::number(index)));
        QVERIFY2(frames.size() < static_cast<size_t>(MaxSeconds * SampleRate * 2), qPrintable(QString::number(index)));
        double peak = 0.0;
        for (const double sample : frames) {
            QVERIFY(std::isfinite(sample));
            peak = std::max(peak, std::abs(sample));
        }
        QVERIFY2(peak > 0.01, qPrintable(QString::number(index)));
    }
}

void DrumSynth808Test::test_everyPad_shouldPeakAtItsTargetUnderTheKick()
{
    DrumSynth808Device device { "808" };
    const auto peaks = peaksAgainstKick(device);
    if (std::getenv("PRINT_808_PEAKS")) {
        for (int index = 0; index < DrumSynthV2::NumVoices; index++) {
            qInfo("pad %d %.2f dB", index, peaks.at(static_cast<size_t>(index)));
        }
    }
    for (int index = 1; index < DrumSynthV2::NumVoices; index++) {
        const auto i = static_cast<size_t>(index);
        QVERIFY2(std::abs(peaks.at(i) - MixTargetDb.at(i)) < 1.5, qPrintable(QString::number(index) + ": " + QString::number(peaks.at(i))));
    }
}

void DrumSynth808Test::test_loadKit_shouldMixAsNamed()
{
    namespace NahdXml = Constants::NahdXml;
    using Kit = DrumSynth808Device::Kit;
    for (const auto kit : { Kit::Mono, Kit::Equal, Kit::MonoEqual }) {
        DrumSynth808Device device { "808" };
        device.loadKit(kit);
        const bool mono = kit != Kit::Equal;
        const bool equal = kit != Kit::Mono;
        const auto peaks = peaksAgainstKick(device);
        for (int index = 0; index < DrumSynthV2::NumVoices; index++) {
            const auto i = static_cast<size_t>(index);
            const double target = equal ? 0.0 : MixTargetDb.at(i);
            QVERIFY2(std::abs(peaks.at(i) - target) < 1.5, qPrintable(QString::fromStdString(DrumSynth808Device::kitNames().at(static_cast<size_t>(kit))) + " " + QString::number(index) + ": " + QString::number(peaks.at(i))));
            if (mono) {
                QCOMPARE(device.voiceParameterValue(index, NahdXml::xmlKeyPan().toStdString()), 0.5f);
            }
        }
    }
}

void DrumSynth808Test::test_kick_shouldPlayWhatKick808Plays()
{
    // The pad's kick is Kick808Engine, tuned where the RD-8's panel marks its Tuning
    DrumSynth808Device device { "808" };
    const auto frames = device.renderVoiceAlone(pad(VoiceIndex::Kick), SampleRate, 1.0);
    std::vector<double> pad;
    for (size_t i = 0; i < frames.size(); i += 2) {
        pad.push_back(frames.at(i));
    }
    Kick808Engine engine;
    engine.setTone(0.35f);
    engine.setDecay(0.5f);
    // 3.885 semitones under Kick808Engine's noon, out of its 24 either way
    engine.setTune(0.5f - 3.885f / 48.0f);
    const auto reference = render(engine, static_cast<double>(pad.size()) / SampleRate);
    // At the best lag: the device's oversampling delays what it plays by a little
    double correlation = 0.0;
    const auto length = static_cast<size_t>(0.3 * SampleRate);
    for (size_t lag = 0; lag < 200; lag++) {
        double cross = 0.0, a = 0.0, b = 0.0;
        for (size_t i = 0; i < length; i++) {
            cross += pad.at(i + lag) * reference.at(i);
            a += pad.at(i + lag) * pad.at(i + lag);
            b += reference.at(i) * reference.at(i);
        }
        correlation = std::max(correlation, cross / std::sqrt(a * b));
    }
    QVERIFY2(correlation > 0.98, qPrintable(QString::number(correlation)));
    // Where the RD-8's kick is with its Tuning at 3
    QVERIFY2(std::abs(strongestHz(reference, 0.15, 0.6, 40.0, 70.0) - 52.26) < 1.0, qPrintable(QString::number(strongestHz(reference, 0.15, 0.6, 40.0, 70.0))));
}

void DrumSynth808Test::test_kick_tuning_shouldSpanTheRd8sRange()
{
    namespace NahdXml = Constants::NahdXml;
    // The RD-8's kick runs from 40 Hz to 86 Hz, nothing like Kick 808's four octaves
    for (auto && [tuning, hz] : { std::pair { 0.0f, 40.01 }, std::pair { 0.5f, 64.72 }, std::pair { 1.0f, 85.68 } }) {
        DrumSynth808Device device { "808" };
        device.updateVoiceParameter(pad(VoiceIndex::Kick), NahdXml::xmlKeyTune().toStdString(), tuning);
        const auto frames = device.renderVoiceAlone(pad(VoiceIndex::Kick), SampleRate, 1.0);
        std::vector<double> left;
        for (size_t i = 0; i < frames.size(); i += 2) {
            left.push_back(frames.at(i));
        }
        const double measured = strongestHz(left, 0.15, 0.6, 30.0, 100.0);
        QVERIFY2(std::abs(measured / hz - 1.0) < 0.02, qPrintable(QString::number(measured) + " vs " + QString::number(hz)));
    }
}

void DrumSynth808Test::test_tom_noon_shouldSettleAtTheRd8sPitch()
{
    // What the RD-8's toms and congas settle at with Tuning at noon. Its mid conga was not swept.
    struct Case
    {
        Tom808Engine::Position position;
        bool conga;
        double hz;
    };

    using P = Tom808Engine::Position;
    for (auto && c : { Case { P::Low, false, 97.59 }, Case { P::Mid, false, 146.58 }, Case { P::High, false, 198.06 }, Case { P::Low, true, 215.09 }, Case { P::High, true, 432.64 } }) {
        Tom808Engine engine { c.position };
        engine.setConga(c.conga);
        const double hz = strongestHz(render(engine, 0.2), 0.04, 0.2, c.hz * 0.9, c.hz * 1.1);
        QVERIFY2(std::abs(hz / c.hz - 1.0) < 0.01, qPrintable(QString::number(hz) + " vs " + QString::number(c.hz)));
    }
}

void DrumSynth808Test::test_tom_tune_shouldSpanTheRd8sRange()
{
    // On the RD-8 Tuning moves a tom 1.7 semitones down and 2.2 up, and it is louder up there
    Tom808Engine noon { Tom808Engine::Position::Mid };
    Tom808Engine low { Tom808Engine::Position::Mid };
    low.setTune(0.0f);
    Tom808Engine high { Tom808Engine::Position::Mid };
    high.setTune(1.0f);
    const auto noonHit = render(noon, 0.2);
    const auto highHit = render(high, 0.2);
    const double noonHz = strongestHz(noonHit, 0.04, 0.2, 120.0, 190.0);
    const double lowHz = strongestHz(render(low, 0.2), 0.04, 0.2, 120.0, 190.0);
    const double highHz = strongestHz(highHit, 0.04, 0.2, 120.0, 190.0);
    QVERIFY2(std::abs(lowHz / noonHz - 0.905) < 0.01, qPrintable(QString::number(lowHz / noonHz)));
    QVERIFY2(std::abs(highHz / noonHz - 1.138) < 0.01, qPrintable(QString::number(highHz / noonHz)));
    QVERIFY(rmsDb(highHit, 0.0, 0.1) > rmsDb(noonHit, 0.0, 0.1) + 2.0);
}

void DrumSynth808Test::test_tom_switched_shouldPlayACongaWithoutNoise()
{
    // A tom carries a low noise past its tone; a conga is the tone alone
    Tom808Engine tom { Tom808Engine::Position::Low };
    Tom808Engine conga { Tom808Engine::Position::Low };
    conga.setConga(true);
    const auto tomHit = render(tom, 0.5);
    const auto congaHit = render(conga, 0.5);
    QVERIFY(std::abs(strongestHz(congaHit, 0.04, 0.2, 190.0, 260.0) - 215.09) < 2.5);
    // Between the tom's harmonics, 160 ms in: noise, 30 dB over the conga's silence there
    QVERIFY2(levelDb(tomHit, 0.16, 0.3, 380.0) - levelDb(congaHit, 0.16, 0.3, 380.0) > 30.0,
             qPrintable(QString::number(levelDb(tomHit, 0.16, 0.3, 380.0)) + " vs " + QString::number(levelDb(congaHit, 0.16, 0.3, 380.0))));
}

void DrumSynth808Test::test_rim_switched_shouldPlayTheClaves()
{
    Rim808Engine rim;
    Rim808Engine claves;
    claves.setClaves(true);
    const auto rimHit = render(rim, 0.06);
    const auto clavesHit = render(claves, 0.06);
    QVERIFY(std::abs(strongestHz(clavesHit, 0.002, 0.03, 2000.0, 3500.0, 1.0) - 2533.5) < 15.0);
    // The rim's VCA shuts after 11 ms; the claves ring on
    QVERIFY(rmsDb(rimHit, 0.014, 0.02) < rmsDb(rimHit, 0.002, 0.008) - 40.0);
    QVERIFY(rmsDb(clavesHit, 0.014, 0.02) > rmsDb(clavesHit, 0.002, 0.008) - 30.0);
}

void DrumSynth808Test::test_clap_switched_shouldPlayShortMaracas()
{
    Clap808Engine clap;
    Clap808Engine maracas;
    maracas.setMaracas(true);
    const auto clapHit = render(clap, 0.5);
    const auto maracasHit = render(maracas, 0.5);
    // Cut 20 ms in, and 40 dB down 10 ms after that, when the clap's tail is at its loudest
    QVERIFY(rmsDb(maracasHit, 0.03, 0.1) < rmsDb(maracasHit, 0.002, 0.018) - 40.0);
    QVERIFY(rmsDb(clapHit, 0.03, 0.06) > rmsDb(clapHit, 0.0, 0.03));
    // The maracas are high, the clap is not
    QVERIFY(rmsDb(highPassed(maracasHit, 4000.0), 0.002, 0.018) > rmsDb(maracasHit, 0.002, 0.018) - 6.0);
    QVERIFY(rmsDb(highPassed(clapHit, 4000.0), 0.0, 0.03) < rmsDb(clapHit, 0.0, 0.03) - 6.0);
}

void DrumSynth808Test::test_clap_offset_shouldSetLevelAndLength()
{
    // On the RD-8 Offset is as much the clap's level as its length: 19 dB down at zero
    Clap808Engine none;
    none.setOffset(0.0f);
    Clap808Engine noon;
    Clap808Engine full;
    full.setOffset(1.0f);
    const auto noneHit = render(none, 1.0);
    const auto noonHit = render(noon, 1.0);
    const auto fullHit = render(full, 1.0);
    QVERIFY(rmsDb(noneHit, 0.0, 0.1) < rmsDb(noonHit, 0.0, 0.1) - 15.0);
    QVERIFY(rmsDb(fullHit, 0.08, 0.15) > rmsDb(noonHit, 0.08, 0.15) + 15.0);
}

void DrumSynth808Test::test_snare_tone_shouldTradeTheTwoTones()
{
    // On the RD-8 Tone turns the 334 Hz tone down by 11 dB and the 174 Hz one up by 7 dB at zero,
    // and the other way, by 8 and 7 dB, at full
    Snare808Engine noon;
    Snare808Engine none;
    none.setTone(0.0f);
    Snare808Engine full;
    full.setTone(1.0f);
    const auto noonHit = render(noon, 0.15);
    const auto noneHit = render(none, 0.15);
    const auto fullHit = render(full, 0.15);
    const auto high = [](const std::vector<double> & x) { return levelDb(x, 0.0, 0.05, 334.2); };
    const auto low = [](const std::vector<double> & x) { return levelDb(x, 0.0, 0.1, 173.7); };
    QVERIFY(high(noneHit) < high(noonHit) - 8.0);
    QVERIFY(low(noneHit) > low(noonHit) + 5.0);
    QVERIFY(high(fullHit) > high(noonHit) + 5.0);
    QVERIFY(low(fullHit) < low(noonHit) - 5.0);
}

void DrumSynth808Test::test_snare_snappyZero_shouldLeaveOnlyTheTones()
{
    Snare808Engine noon;
    Snare808Engine dry;
    dry.setSnappy(0.0f);
    const auto noonHit = render(noon, 0.15);
    const auto dryHit = render(dry, 0.15);
    // The noise is around 2.5 kHz; the tones are 174 and 334 Hz
    QVERIFY(levelDb(dryHit, 0.02, 0.1, 2500.0) < levelDb(noonHit, 0.02, 0.1, 2500.0) - 30.0);
    QVERIFY(std::abs(levelDb(dryHit, 0.0, 0.05, 334.2) - levelDb(noonHit, 0.0, 0.05, 334.2)) < 1.0);
}

void DrumSynth808Test::test_cowbell_shouldBeTheRd8sTwoSquares()
{
    Metal808Engine cowbell { Metal808Engine::Sound::Cowbell };
    const auto hit = render(cowbell, 0.3);
    QVERIFY(std::abs(strongestHz(hit, 0.02, 0.3, 500.0, 600.0) - 540.2) < 1.0);
    QVERIFY(std::abs(strongestHz(hit, 0.02, 0.3, 750.0, 850.0) - 799.0) < 1.0);
    // As the RD-8's fades: 20 dB down 0.13 s in and 40 dB down by 0.3 s
    QVERIFY(std::abs(rmsDb(hit, 0.12, 0.14) - rmsDb(hit, 0.0, 0.003) + 20.4) < 3.0);
    QVERIFY(std::abs(rmsDb(hit, 0.25, 0.3) - rmsDb(hit, 0.0, 0.003) + 39.8) < 3.0);
}

void DrumSynth808Test::test_openHat_shouldHoldAndThenClose()
{
    Metal808Engine hat { Metal808Engine::Sound::OpenHat };
    const auto hit = render(hat, 0.5);
    // The RD-8's open hat at noon: within 9 dB of its start for most of 0.3 s, then shut
    QVERIFY(rmsDb(hit, 0.24, 0.27) > rmsDb(hit, 0.01, 0.04) - 9.0);
    QVERIFY(rmsDb(hit, 0.36, 0.4) < rmsDb(hit, 0.01, 0.04) - 30.0);
    // Held for 0.57 s at full Decay, and gone in a tenth of a second at none
    Metal808Engine longer { Metal808Engine::Sound::OpenHat };
    longer.setDecay(1.0f);
    QVERIFY(rmsDb(render(longer, 0.5), 0.36, 0.4) > rmsDb(hit, 0.36, 0.4) + 20.0);
    Metal808Engine shorter { Metal808Engine::Sound::OpenHat };
    shorter.setDecay(0.0f);
    const auto shortHit = render(shorter, 0.5);
    QVERIFY(rmsDb(shortHit, 0.15, 0.2) < rmsDb(shortHit, 0.01, 0.04) - 35.0);
}

void DrumSynth808Test::test_hats_tone_shouldThinThemOutAboveNoon()
{
    // The RD-8's hat Tone takes up to 15 dB off below its corner, where most of a hat is
    for (const auto sound : { Metal808Engine::Sound::ClosedHat, Metal808Engine::Sound::OpenHat }) {
        Metal808Engine noon { sound };
        Metal808Engine full { sound };
        full.setTone(1.0f);
        Metal808Engine none { sound };
        none.setTone(0.0f);
        const double noonDb = rmsDb(render(noon, 0.05), 0.0, 0.05);
        const double fullDb = rmsDb(render(full, 0.05), 0.0, 0.05);
        QVERIFY(fullDb < noonDb - 8.0);
        // Below noon it lifts the shelf a little, which the hats do not have much under
        QVERIFY(rmsDb(render(none, 0.05), 0.0, 0.05) > fullDb + 8.0);
    }
}

void DrumSynth808Test::test_cymbal_lowBand_shouldSwellAfterTheHit()
{
    // Around 2.2 kHz the cymbal grows for a fifth of a second after the hit, while the top dies
    Metal808Engine cymbal { Metal808Engine::Sound::Cymbal };
    const auto hit = render(cymbal, 1.0);
    const auto low = highPassed(hit, 1500.0);
    double early = 0.0, late = 0.0;
    for (double hz : { 2161.0, 2210.5, 2261.1 }) {
        early += std::pow(10.0, levelDb(hit, 0.005, 0.04, hz) / 10.0);
        late += std::pow(10.0, levelDb(hit, 0.15, 0.185, hz) / 10.0);
    }
    QVERIFY2(late > early, qPrintable(QString::number(10.0 * std::log10(late / early))));
    QVERIFY(rmsDb(highPassed(hit, 8000.0), 0.6, 0.8) < rmsDb(highPassed(hit, 8000.0), 0.01, 0.05) - 15.0);
    QVERIFY(!low.empty());
}

void DrumSynth808Test::test_reverseCymbal_shouldEndOnTheHit()
{
    Metal808Engine reverse { Metal808Engine::Sound::Cymbal, true };
    reverse.setSampleRate(SampleRate);
    const double length = reverse.reversedSeconds();
    const auto hit = render(reverse, length + 0.1);
    // Loudest in its last tenth, silent once it has reached the hit
    QVERIFY(rmsDb(hit, length * 0.9, length) > rmsDb(hit, 0.0, length * 0.3) + 10.0);
    QVERIFY(rmsDb(hit, length + 0.01, length + 0.1) < -150.0);
}

void DrumSynth808Test::test_xmlSerialization_shouldRestoreTheSound()
{
    namespace NahdXml = Constants::NahdXml;
    DrumSynth808Device device { "808" };
    device.updateVoiceParameter(pad(VoiceIndex::LowTom), NahdXml::xmlKeyVoiceSwitch().toStdString(), 1.0f);
    device.updateVoiceParameter(pad(VoiceIndex::Clap), NahdXml::xmlKeyOffset().toStdString(), 0.8f);
    device.updateVoiceParameter(pad(VoiceIndex::Snare), NahdXml::xmlKeySnappy().toStdString(), 0.2f);
    device.updateVoiceParameter(pad(VoiceIndex::Kick), NahdXml::xmlKeyTune().toStdString(), 0.7f);
    DrumSynth808Device restored { "808" };
    fromXml(restored, toXml(device));
    QCOMPARE(restored.voiceParameterValue(pad(VoiceIndex::LowTom), NahdXml::xmlKeyVoiceSwitch().toStdString()), 1.0f);
    QCOMPARE(restored.voiceParameterValue(pad(VoiceIndex::Clap), NahdXml::xmlKeyOffset().toStdString()), 0.8f);
    QCOMPARE(restored.voiceParameterValue(pad(VoiceIndex::Snare), NahdXml::xmlKeySnappy().toStdString()), 0.2f);
    QCOMPARE(restored.voiceParameterValue(pad(VoiceIndex::Kick), NahdXml::xmlKeyTune().toStdString()), 0.7f);
    // And it is the conga that plays
    const auto original = device.renderVoiceAlone(pad(VoiceIndex::LowTom), SampleRate, 1.0);
    const auto copy = restored.renderVoiceAlone(pad(VoiceIndex::LowTom), SampleRate, 1.0);
    QCOMPARE(copy.size(), original.size());
    for (size_t i = 0; i < copy.size(); i++) {
        QVERIFY(std::abs(copy.at(i) - original.at(i)) < 1e-9);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DrumSynth808Test)
