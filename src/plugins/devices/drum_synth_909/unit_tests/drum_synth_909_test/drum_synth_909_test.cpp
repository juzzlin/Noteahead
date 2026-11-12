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

#include "drum_synth_909_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/drum/metal_909_engine.hpp"
#include "../../../../../domain/dsp/drum/snare_909_engine.hpp"
#include "../../../../../domain/dsp/drum/tom_909_engine.hpp"
#include "../../../../../infra/xml/nahd_xml_reader.hpp"
#include "../../../../../infra/xml/nahd_xml_writer.hpp"
#include "../../drum_synth_909_device.hpp"

#include <QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };
constexpr double MaxSeconds { 8.0 };

template<typename Engine>
std::vector<double> render(Engine & engine, double seconds)
{
    engine.setSampleRate(SampleRate);
    engine.trigger(1.0f);
    std::vector<double> out;
    for (size_t i = 0; i < static_cast<size_t>(seconds * SampleRate); i++) {
        out.push_back(static_cast<double>(engine.nextSample()));
    }
    return out;
}

//! Where the strongest component between @p lowHz and @p highHz lies over [from, to) seconds, to a
//! tenth of a hertz
double strongestHz(const std::vector<double> & x, double from, double to, double lowHz, double highHz)
{
    const auto begin = static_cast<size_t>(from * SampleRate);
    const auto end = std::min(x.size(), static_cast<size_t>(to * SampleRate));
    double best = 0.0;
    double bestHz = 0.0;
    for (double hz = lowHz; hz < highHz; hz += 0.1) {
        std::complex<double> sum {};
        for (size_t i = begin; i < end; i++) {
            const double window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i - begin) / static_cast<double>(end - begin));
            sum += x.at(i) * window * std::polar(1.0, -2.0 * std::numbers::pi * hz * static_cast<double>(i) / SampleRate);
        }
        if (std::abs(sum) > best) {
            best = std::abs(sum);
            bestHz = hz;
        }
    }
    return bestHz;
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

//! When the 10 ms RMS falls @p db under its peak for good
double fallSeconds(const std::vector<double> & x, double db)
{
    const auto window = static_cast<size_t>(0.01 * SampleRate);
    std::vector<double> rms;
    for (size_t i = 0; i + window < x.size(); i += window / 2) {
        double sum = 0.0;
        for (size_t j = i; j < i + window; j++) {
            sum += x.at(j) * x.at(j);
        }
        rms.push_back(std::sqrt(sum / static_cast<double>(window)));
    }
    const double top = *std::ranges::max_element(rms);
    for (size_t i = rms.size(); i-- > 0;) {
        if (20.0 * std::log10(rms.at(i) / top + 1e-12) > db) {
            return static_cast<double>((i + 1) * window / 2) / SampleRate;
        }
    }
    return 0.0;
}

std::string key(int pad, const QString & name)
{
    return DrumSynthV2::voiceId(pad) + "_" + name.toStdString();
}

QString toXml(const DrumSynth909Device & device)
{
    QString xml;
    NahdXmlWriter writer { xml };
    device.serializeToXml(writer);
    return xml;
}

void fromXml(DrumSynth909Device & device, const QString & xml)
{
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    device.deserializeFromXml(reader);
}

} // namespace

void DrumSynth909Test::test_pads_shouldHaveTheRd9Knobs()
{
    namespace NahdXml = Constants::NahdXml;
    using DrumSynthV2::VoiceIndex;
    const DrumSynth909Device device { "909" };
    const auto count = [](VoiceIndex pad) { return DrumSynth909Device::voiceKnobs(static_cast<int>(pad)).size(); };
    QCOMPARE(count(VoiceIndex::Kick), size_t { 5 });
    QCOMPARE(count(VoiceIndex::Snare), size_t { 3 });
    QCOMPARE(count(VoiceIndex::LowTom), size_t { 2 });
    QCOMPARE(count(VoiceIndex::ClosedHiHat), size_t { 2 });
    QCOMPARE(count(VoiceIndex::OpenHiHat), size_t { 2 });
    QCOMPARE(count(VoiceIndex::Crash), size_t { 1 });
    QCOMPARE(count(VoiceIndex::ReverseCrash), size_t { 1 });
    QCOMPARE(count(VoiceIndex::Ride), size_t { 1 });
    QCOMPARE(count(VoiceIndex::Clap), size_t { 0 });
    QCOMPARE(count(VoiceIndex::Rim), size_t { 0 });
    for (int pad = 0; pad < DrumSynthV2::NumVoices; pad++) {
        for (auto && knob : DrumSynth909Device::voiceKnobs(pad)) {
            const auto parameter = device.parameter(DrumSynthV2::voiceId(pad) + "_" + knob.key);
            QVERIFY2(parameter.has_value(), knob.key.c_str());
            // Every knob rests at noon, as the RD-9's do
            QCOMPARE(parameter->get().value(), 0.5f);
        }
    }
    QVERIFY(!device.parameter(key(static_cast<int>(VoiceIndex::Kick), NahdXml::xmlKeyClickTune())).has_value());
}

void DrumSynth909Test::test_everyPad_shouldSoundAndThenStop()
{
    DrumSynth909Device device { "909" };
    for (int pad = 0; pad < DrumSynthV2::NumVoices; pad++) {
        const auto frames = device.renderVoiceAlone(pad, SampleRate, MaxSeconds);
        QVERIFY2(!frames.empty(), qPrintable(QString::number(pad)));
        QVERIFY2(frames.size() < static_cast<size_t>(MaxSeconds * SampleRate * 2), qPrintable(QString::number(pad)));
        double peak = 0.0;
        for (const double sample : frames) {
            QVERIFY(std::isfinite(sample));
            peak = std::max(peak, std::abs(sample));
        }
        QVERIFY2(peak > 0.01, qPrintable(QString::number(pad)));
    }
}

namespace {

//! Each pad's peak against the kick's, in dB
std::array<double, DrumSynthV2::NumVoices> peaksAgainstKick(DrumSynth909Device & device)
{
    std::array<double, DrumSynthV2::NumVoices> result {};
    double kickDb = 0.0;
    for (int pad = 0; pad < DrumSynthV2::NumVoices; pad++) {
        double peak = 0.0;
        for (const double sample : device.renderVoiceAlone(pad, SampleRate, MaxSeconds)) {
            peak = std::max(peak, std::abs(sample));
        }
        const double db = 20.0 * std::log10(peak);
        if (pad == 0) {
            kickDb = db;
        }
        result.at(static_cast<size_t>(pad)) = db - kickDb;
    }
    return result;
}

//! The mix a new device starts with: Drum Synth V2's targets, the crash louder
constexpr std::array<double, DrumSynthV2::NumVoices> MixTargetDb { 0.0, -2.5, -8.0, -3.0, -7.0, -4.0, -4.0, -4.0, -9.0, -10.0, -10.0, -6.0 };

} // namespace

void DrumSynth909Test::test_everyPad_shouldPeakAtItsTargetUnderTheKick()
{
    DrumSynth909Device device { "909" };
    const auto peaks = peaksAgainstKick(device);
    for (int pad = 1; pad < DrumSynthV2::NumVoices; pad++) {
        const auto i = static_cast<size_t>(pad);
        QVERIFY2(std::abs(peaks.at(i) - MixTargetDb.at(i)) < 1.5, qPrintable(QString::number(pad) + ": " + QString::number(peaks.at(i))));
    }
}

void DrumSynth909Test::test_loadKit_shouldMixAsNamed()
{
    namespace NahdXml = Constants::NahdXml;
    using Kit = DrumSynth909Device::Kit;
    for (const auto kit : { Kit::Mono, Kit::Equal, Kit::MonoEqual }) {
        DrumSynth909Device device { "909" };
        device.loadKit(kit);
        const bool mono = kit != Kit::Equal;
        const bool equal = kit != Kit::Mono;
        const auto peaks = peaksAgainstKick(device);
        for (int pad = 0; pad < DrumSynthV2::NumVoices; pad++) {
            const auto i = static_cast<size_t>(pad);
            const double target = equal ? 0.0 : MixTargetDb.at(i);
            QVERIFY2(std::abs(peaks.at(i) - target) < 1.5, qPrintable(QString::fromStdString(DrumSynth909Device::kitNames().at(static_cast<size_t>(kit))) + " " + QString::number(pad) + ": " + QString::number(peaks.at(i))));
            if (mono) {
                QCOMPARE(device.voiceParameterValue(pad, NahdXml::xmlKeyPan().toStdString()), 0.5f);
            }
        }
    }
}

void DrumSynth909Test::test_tom_tune_shouldFollowTheRd9()
{
    // The RD-9's mid tom at noon: 79.8 Hz and 126.5 Hz once settled; Tune 10 takes both up by 1.32
    Tom909Engine noon { Tom909Engine::Tom::Mid };
    Tom909Engine high { Tom909Engine::Tom::Mid };
    high.setTune(1.0f);
    const double noonHz = strongestHz(render(noon, 0.4), 0.15, 0.35, 100.0, 140.0);
    const double highHz = strongestHz(render(high, 0.4), 0.15, 0.35, 140.0, 190.0);
    QVERIFY2(std::abs(noonHz - 126.5) < 1.5, qPrintable(QString::number(noonHz)));
    QVERIFY2(std::abs(highHz / noonHz - 1.3168) < 0.02, qPrintable(QString::number(highHz / noonHz)));
}

void DrumSynth909Test::test_tom_decay_shouldMatchTheRd9()
{
    // The RD-9's low tom falls 40 dB in 0.29 s at Decay 0 and 0.47 s at 10, and hardly any longer
    // past noon, 0.43 s
    Tom909Engine shortest { Tom909Engine::Tom::Low };
    shortest.setDecay(0.0f);
    Tom909Engine noon { Tom909Engine::Tom::Low };
    Tom909Engine longest { Tom909Engine::Tom::Low };
    longest.setDecay(1.0f);
    const double shortSeconds = fallSeconds(render(shortest, 1.0), -40.0);
    const double noonSeconds = fallSeconds(render(noon, 1.0), -40.0);
    const double longSeconds = fallSeconds(render(longest, 1.0), -40.0);
    QVERIFY2(shortSeconds > 0.24 && shortSeconds < 0.34, qPrintable(QString::number(shortSeconds)));
    QVERIFY2(noonSeconds > 0.37 && noonSeconds < 0.49, qPrintable(QString::number(noonSeconds)));
    QVERIFY2(longSeconds > 0.41 && longSeconds < 0.55, qPrintable(QString::number(longSeconds)));
}

void DrumSynth909Test::test_tom_resonance_shouldLiftTheLowTomsThirdHarmonic()
{
    // At Tune 0 the RD-9's low tom has its upper oscillator's third harmonic, at 173 Hz where the
    // resonance is, 8 dB over that oscillator's fundamental
    Tom909Engine engine { Tom909Engine::Tom::Low };
    engine.setTune(0.0f);
    const auto hit = render(engine, 0.4);
    const double fundamentalHz = 57.07 * 0.6704 * 1.488;
    const double over = levelDb(hit, 0.15, 0.35, 3.0 * fundamentalHz) - levelDb(hit, 0.15, 0.35, fundamentalHz);
    QVERIFY2(over > 3.0 && over < 13.0, qPrintable(QString::number(over)));
}

void DrumSynth909Test::test_snare_tune_shouldFollowTheRd9()
{
    // The RD-9's snare at noon: its lower tone settles at 194 Hz; Tune 0 takes it to 126 Hz
    Snare909Engine noon;
    noon.setSnappy(0.0f);
    Snare909Engine low;
    low.setSnappy(0.0f);
    low.setTune(0.0f);
    QVERIFY(std::abs(strongestHz(render(noon, 0.2), 0.03, 0.15, 150.0, 230.0) - 194.1) < 2.0);
    QVERIFY(std::abs(strongestHz(render(low, 0.2), 0.03, 0.15, 100.0, 160.0) - 128.0) < 3.0);
}

void DrumSynth909Test::test_snare_tone_shouldHoldTheNoiseLonger()
{
    // Tone is how long the noise is held: on the RD-9 it is shut 130 ms in at Tone 0 and still open
    // at 190 ms at Tone 10
    const auto noiseAt = [](float tone, double seconds) {
        Snare909Engine engine;
        engine.setTone(tone);
        engine.setSnappy(1.0f);
        const auto hit = render(engine, 0.4);
        return levelDb(hit, seconds, seconds + 0.02, 3000.0);
    };
    QVERIFY(noiseAt(0.0f, 0.13) < noiseAt(1.0f, 0.13) - 20.0);
    QVERIFY(noiseAt(1.0f, 0.17) > noiseAt(1.0f, 0.05) - 6.0);
}

void DrumSynth909Test::test_snare_snappy_shouldTradeBodyForNoise()
{
    // Snappy at 0 is the body alone; at full the noise is in and the body 5 dB down
    Snare909Engine dry;
    dry.setSnappy(0.0f);
    Snare909Engine snappy;
    snappy.setSnappy(1.0f);
    const auto dryHit = render(dry, 0.2);
    const auto snappyHit = render(snappy, 0.2);
    QVERIFY(levelDb(snappyHit, 0.03, 0.1, 3000.0) > levelDb(dryHit, 0.03, 0.1, 3000.0) + 30.0);
    const double bodyDrop = levelDb(snappyHit, 0.03, 0.15, 194.08) - levelDb(dryHit, 0.03, 0.15, 194.08);
    QVERIFY2(bodyDrop < -4.0 && bodyDrop > -6.5, qPrintable(QString::number(bodyDrop)));
}

void DrumSynth909Test::test_hat_tune_shouldPlayTheSpectrumFaster()
{
    // On the RD-9 Tune moves every partial of a hat by the same factor: 1.51 times at full
    Metal909Engine noon { Metal909Engine::Sound::OpenHat };
    Metal909Engine high { Metal909Engine::Sound::OpenHat };
    high.setTune(1.0f);
    const auto noonHit = render(noon, 0.2);
    const auto highHit = render(high, 0.2);
    // The open hat's strongest low partial, 1643.5 Hz at noon
    QVERIFY(std::abs(strongestHz(noonHit, 0.01, 0.15, 1600.0, 1700.0) - 1643.5) < 2.0);
    QVERIFY(std::abs(strongestHz(highHit, 0.01, 0.15, 2400.0, 2560.0) - 1643.5 * 1.5092) < 3.0);
}

void DrumSynth909Test::test_hat_decay_shouldMatchTheRd9()
{
    // When the RD-9's hats have fallen 40 dB for good: the closed one 0.04 s at Decay 0 and 0.14 s
    // at 10, the open one 0.25 s and 0.55 s, all above 1 kHz
    const auto fall = [](Metal909Engine::Sound sound, float decay) {
        Metal909Engine engine { sound };
        engine.setDecay(decay);
        return fallSeconds(render(engine, 1.0), -40.0);
    };
    const double closedShort = fall(Metal909Engine::Sound::ClosedHat, 0.0f);
    const double closedLong = fall(Metal909Engine::Sound::ClosedHat, 1.0f);
    const double openShort = fall(Metal909Engine::Sound::OpenHat, 0.0f);
    const double openLong = fall(Metal909Engine::Sound::OpenHat, 1.0f);
    QVERIFY2(closedShort > 0.02 && closedShort < 0.07, qPrintable(QString::number(closedShort)));
    QVERIFY2(closedLong > 0.11 && closedLong < 0.18, qPrintable(QString::number(closedLong)));
    QVERIFY2(openShort > 0.19 && openShort < 0.31, qPrintable(QString::number(openShort)));
    QVERIFY2(openLong > 0.44 && openLong < 0.66, qPrintable(QString::number(openLong)));
}

void DrumSynth909Test::test_cymbal_tune_shouldShortenTheSample()
{
    // On the RD-9 the crash's sample ends 0.92 s in at noon and 0.61 s at Tune 10: Tune plays it faster
    Metal909Engine noon { Metal909Engine::Sound::Crash };
    Metal909Engine high { Metal909Engine::Sound::Crash };
    high.setTune(1.0f);
    const double noonEnd = fallSeconds(render(noon, 1.5), -40.0);
    const double highEnd = fallSeconds(render(high, 1.5), -40.0);
    QVERIFY2(noonEnd > 0.82 && noonEnd < 0.98, qPrintable(QString::number(noonEnd)));
    QVERIFY2(std::abs(noonEnd / highEnd - 1.52) < 0.12, qPrintable(QString::number(noonEnd / highEnd)));
}

void DrumSynth909Test::test_crash_shouldBloomFromTheStrikeAndKeepItsSplashPartials()
{
    // On the RD-9 the crash starts on a low-mid strike near 540 Hz while the 4 to 7 kHz splash
    // opens out over the first 80 ms, its main partials at 4342.8 Hz and 6378.5 Hz standing clear
    // of the noise around them
    Metal909Engine noon { Metal909Engine::Sound::Crash };
    const auto hit = render(noon, 1.0);
    const double strikeEarly = levelDb(hit, 0.0, 0.04, 540.0);
    const double strikeLate = levelDb(hit, 0.15, 0.19, 540.0);
    QVERIFY2(strikeEarly > strikeLate + 6.0, qPrintable(QString::number(strikeEarly - strikeLate)));

    const double splashAttack = levelDb(hit, 0.0, 0.04, 6379.1);
    const double splashBloom = levelDb(hit, 0.08, 0.12, 6379.1);
    QVERIFY2(splashBloom > splashAttack + 1.5, qPrintable(QString::number(splashBloom - splashAttack)));

    const double partialHz = strongestHz(hit, 0.08, 0.50, 6350.0, 6400.0);
    QVERIFY2(std::abs(partialHz - 6378.5) < 2.0, qPrintable(QString::number(partialHz)));
    const double partialOverNoise = levelDb(hit, 0.08, 0.50, partialHz) - levelDb(hit, 0.08, 0.50, 6320.0);
    QVERIFY2(partialOverNoise > 12.0, qPrintable(QString::number(partialOverNoise)));
}

void DrumSynth909Test::test_reverseCrash_shouldEndOnTheStrike()
{
    // Backwards, the crash swells to its loudest at the very end
    Metal909Engine engine { Metal909Engine::Sound::Crash, true };
    const auto hit = render(engine, 1.0);
    const double early = levelDb(hit, 0.05, 0.15, 4000.0);
    const double late = levelDb(hit, 0.70, 0.80, 4000.0);
    QVERIFY2(late > early + 6.0, qPrintable(QString::number(late - early)));
    QVERIFY(!engine.isActive());
}

void DrumSynth909Test::test_xmlSerialization_shouldRestoreTheSound()
{
    namespace NahdXml = Constants::NahdXml;
    const int tom = static_cast<int>(DrumSynthV2::VoiceIndex::MidTom);
    DrumSynth909Device original { "909" };
    original.updateVoiceParameter(tom, NahdXml::xmlKeyTune().toStdString(), 0.8f);
    original.updateVoiceParameter(tom, NahdXml::xmlKeyDecay().toStdString(), 0.1f);

    DrumSynth909Device restored { "909" };
    fromXml(restored, toXml(original));

    QCOMPARE(restored.renderVoiceAlone(tom, SampleRate, MaxSeconds), original.renderVoiceAlone(tom, SampleRate, MaxSeconds));
    QCOMPARE(toXml(restored), toXml(original));
    QVERIFY(restored.renderVoiceAlone(tom, SampleRate, MaxSeconds) != DrumSynth909Device { "909" }.renderVoiceAlone(tom, SampleRate, MaxSeconds));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DrumSynth909Test)
