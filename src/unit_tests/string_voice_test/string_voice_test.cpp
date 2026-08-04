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

#include "string_voice_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/string_voice_device.hpp"
#include "../../domain/dsp/formant_filter_bank.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QBuffer>
#include <QTest>
#include <algorithm>
#include <limits>
#include <numbers>

#include <array>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

AudioContext makeContext(std::vector<double> & buffer, uint32_t frameCount, uint32_t sampleRate = 44100)
{
    return AudioContext { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
}

double peakLevel(const std::vector<double> & buffer)
{
    double peak { 0.0 };
    for (const double s : buffer) {
        peak = std::max(peak, std::abs(s));
    }
    return peak;
}

double rmsLevel(const std::vector<double> & buffer)
{
    double sum { 0.0 };
    for (const double s : buffer) {
        sum += s * s;
    }
    return std::sqrt(sum / static_cast<double>(buffer.size()));
}

void renderFrames(StringVoiceDevice & dev, uint32_t frames, uint32_t sampleRate = 44100)
{
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    auto ctx = makeContext(buffer, frames, sampleRate);
    dev.processAudio(ctx);
}

// Average number of samples between consecutive rising zero-crossings of the
// left channel within [startFrame, endFrame), used to detect pitch modulation
// (vibrato) between two portions of the same render.
double averageZeroCrossingPeriod(const std::vector<double> & buffer, uint32_t startFrame, uint32_t endFrame)
{
    std::vector<uint32_t> crossings;
    for (uint32_t i = startFrame + 1; i < endFrame; ++i) {
        const double prev = buffer[(i - 1) * 2];
        const double curr = buffer[i * 2];
        if (prev <= 0.0 && curr > 0.0) {
            crossings.push_back(i);
        }
    }
    if (crossings.size() < 2) {
        return 0.0;
    }
    return static_cast<double>(crossings.back() - crossings.front()) / static_cast<double>(crossings.size() - 1);
}

//! Energy between two frequencies, from log-spaced DFT probes. The voices drift a few cents by
//! design, so anything that measures at exact harmonic multiples would read low.
double bandEnergy(const std::vector<double> & buffer, uint32_t startFrame, uint32_t frames, double lowHz, double highHz, uint32_t sampleRate = 44100)
{
    constexpr int probes { 9 };
    double energy { 0.0 };
    for (int probe = 0; probe < probes; probe++) {
        const double frequency { lowHz * std::pow(highHz / lowHz, static_cast<double>(probe) / (probes - 1)) };
        const double omega { 2.0 * std::numbers::pi * frequency / sampleRate };
        double re { 0.0 };
        double im { 0.0 };
        for (uint32_t i = 0; i < frames; i++) {
            const double sample { buffer[(startFrame + i) * 2] };
            re += sample * std::cos(omega * i);
            im -= sample * std::sin(omega * i);
        }
        energy += re * re + im * im;
    }
    return energy;
}

//! One register on its own, held long enough for the envelope to settle.
std::vector<double> renderRegister(bool male, uint32_t frames)
{
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setSampleRate(44100);
    dev.setStringsLevel8(0.0f);
    dev.setStringsLevel4(0.0f);
    dev.setVoiceMale8(male ? 1.0f : 0.0f);
    dev.setVoiceUpperMale8(0.0f);
    dev.setVoiceFemale4(male ? 0.0f : 1.0f);
    dev.setVoiceAttack(0.0f);
    dev.setEnsembleEnabled(false);
    dev.setVibratoDepth(0.0f);
    // The male register is asked for a note below the keyboard split and the female one above it,
    // which is the only place either sounds.
    dev.processMidiNoteOn(male ? 48 : 60, 100);

    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    auto ctx = makeContext(buffer, frames);
    dev.processAudio(ctx);
    return buffer;
}

} // namespace

void StringVoiceTest::test_midiNoteOn_shouldActivateAudio()
{
    StringVoiceDevice dev { "Test StringVoice" };
    dev.processMidiNoteOn(60, 100);
    QVERIFY(dev.hasActiveAudio());
}

void StringVoiceTest::test_midiNoteOff_shouldDecayToSilence()
{
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setStringsRelease(0.0f); // very short
    dev.setVoiceRelease(0.0f);
    dev.processMidiNoteOn(60, 100);

    renderFrames(dev, 256);
    QVERIFY(dev.hasActiveAudio());

    dev.processMidiNoteOff(60);

    // Render enough to release
    renderFrames(dev, 44100);
    QVERIFY(!dev.hasActiveAudio());
}

void StringVoiceTest::test_polyphony_shouldSupportMultipleSimultaneousNotes()
{
    // With equal-power headroom compensation (added to prevent chords from
    // clipping, see test_polyphony_shouldNotClipWithManyNotes below), the
    // single-sample peak of a 2-note chord is no longer guaranteed to exceed
    // that of a single note played at full, uncompensated gain -- their
    // relative phase at any given sample is essentially random. Total energy
    // (RMS) across the buffer is the correct, interference-robust way to
    // verify that a second voice is genuinely contributing audio.
    // The window has to be long enough to resolve what the second voice adds. The mix is equal
    // power, so two notes carry only some 6 - 13 % more than one, and 256 frames is under two
    // cycles of the lower note: too short to tell that apart from where the two happen to interfere.
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setStringsAttack(0.0f);
    dev.setVoiceAttack(0.0f);
    dev.setEnsembleEnabled(false);
    dev.processMidiNoteOn(60, 100);
    dev.processMidiNoteOn(64, 100);

    const uint32_t frameCount { 8192 };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    auto ctx = makeContext(buffer, frameCount);
    dev.processAudio(ctx);

    const double rmsTwo { rmsLevel(buffer) };

    StringVoiceDevice devOne { "Test StringVoice One" };
    devOne.setStringsAttack(0.0f);
    devOne.setVoiceAttack(0.0f);
    devOne.setEnsembleEnabled(false);
    devOne.processMidiNoteOn(60, 100);
    std::vector<double> bufferOne(static_cast<size_t>(frameCount) * 2, 0.0);
    auto ctxOne = makeContext(bufferOne, frameCount);
    devOne.processAudio(ctxOne);
    const double rmsOne { rmsLevel(bufferOne) };

    QVERIFY(rmsTwo > rmsOne);
}

void StringVoiceTest::test_polyphony_shouldNotClipWithManyNotes()
{
    // Regression test: stacking a full chord used to sum each voice at a
    // fixed level regardless of how many others were sounding, clipping
    // (and distorting) well before the voice pool was exhausted.
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setStringsAttack(0.0f);
    dev.setVoiceAttack(0.0f);

    const std::array<uint8_t, 8> notes { 48, 52, 55, 60, 64, 67, 71, 72 };
    for (const auto note : notes) {
        dev.processMidiNoteOn(note, 127);
    }

    const uint32_t frameCount { 4096 };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    auto ctx = makeContext(buffer, frameCount);
    dev.processAudio(ctx);

    QVERIFY(peakLevel(buffer) < 1.0);
}

void StringVoiceTest::test_allNotesOff_shouldSilenceAllVoices()
{
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setStringsRelease(0.0f);
    dev.setVoiceRelease(0.0f);

    dev.processMidiNoteOn(60, 100);
    dev.processMidiNoteOn(64, 100);
    QVERIFY(dev.hasActiveAudio());

    dev.processMidiAllNotesOff();
    renderFrames(dev, 44100);
    QVERIFY(!dev.hasActiveAudio());
}

void StringVoiceTest::test_serialization_shouldRestoreParameters()
{
    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);

    {
        NahdXmlWriter writer { buffer };
        StringVoiceDevice dev { "Test StringVoice" };
        dev.setStringsBalance(0.35f);
        dev.setVoiceBalance(0.65f);
        dev.setStringsLevel8(0.45f);
        dev.setStringsLevel4(0.6f);
        dev.setVoiceMale4(0.3f);
        dev.setVoiceUpperMale8(0.7f);
        dev.setEnsembleEnabled(false);
        dev.setEnsembleMode(1);
        dev.setVocoderEnabled(true);
        dev.setVocoderSidechain(3);
        dev.setLpfCutoff(0.65f);
        dev.setHpfCutoff(0.15f);
        dev.setPanSpread(0.85f);
        dev.serializeToXml(writer);
    }

    buffer.close();
    buffer.open(QIODevice::ReadOnly);

    {
        NahdXmlReader reader { buffer };
        StringVoiceDevice dev { "Restored StringVoice" };

        QVERIFY(reader.readNextStartElement());
        QCOMPARE(reader.name(), Constants::NahdXml::xmlKeyDevice());

        dev.deserializeFromXml(reader);

        QCOMPARE(dev.stringsBalance(), 0.35f);
        QCOMPARE(dev.voiceBalance(), 0.65f);
        QCOMPARE(dev.stringsLevel8(), 0.45f);
        QCOMPARE(dev.stringsLevel4(), 0.6f);
        QCOMPARE(dev.voiceMale4(), 0.3f);
        QCOMPARE(dev.voiceUpperMale8(), 0.7f);
        QCOMPARE(dev.ensembleEnabled(), false);
        QCOMPARE(dev.ensembleMode(), 1);
        QCOMPARE(dev.vocoderEnabled(), true);
        QCOMPARE(dev.vocoderSidechain(), 3);
        QCOMPARE(dev.lpfCutoff(), 0.65f);
        QCOMPARE(dev.hpfCutoff(), 0.15f);
        QCOMPARE(dev.panSpread(), 0.85f);
    }
}

void StringVoiceTest::test_audio_output_not_zero()
{
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setSampleRate(44100);
    dev.processMidiNoteOn(60, 100);

    const uint32_t frameCount { 2048 };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    auto ctx = makeContext(buffer, frameCount);
    dev.processAudio(ctx);

    const double peak { peakLevel(buffer) };
    QVERIFY(peak > 0.001);
}

void StringVoiceTest::test_realtimeCallbacks_shouldNotBeSilencedByEnsemble()
{
    // Real-time playback delivers many small callback buffers in a row (e.g. 256
    // frames), unlike a single big offline render call. The ensemble chorus is
    // enabled by default, so this must still produce sound.
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setSampleRate(44100);
    dev.processMidiNoteOn(60, 100);

    double maxAbs { 0.0 };
    for (int callback = 0; callback < 200; ++callback) {
        const uint32_t frameCount { 256 };
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        auto ctx = makeContext(buffer, frameCount);
        dev.processAudio(ctx);
        maxAbs = std::max(maxAbs, peakLevel(buffer));
    }

    QVERIFY(maxAbs > 0.001);
}

void StringVoiceTest::test_vibrato_shouldModulatePitchWithinASingleBuffer()
{
    // The vibrato LFO must advance once per audio sample, not once per
    // processAudio() call. Render one long buffer and compare the average
    // zero-crossing period (a proxy for instantaneous pitch) between two
    // widely-separated windows: with deep, fast vibrato they must differ
    // noticeably. If the LFO were only stepped once per call, the pitch shift
    // would be frozen for the entire buffer and the two windows would match.
    // Isolate the (unfiltered) Strings section so this test tracks pure pitch
    // and isn't coupled to the choir's formant filtering, which shapes its
    // harmonic content (and therefore this zero-crossing heuristic's
    // sensitivity) independently of vibrato correctness.
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setSampleRate(44100);
    dev.setStringsAttack(0.0f);
    dev.setVoiceAttack(0.0f);
    dev.setVoiceMale8(0.0f);
    dev.setEnsembleEnabled(false);
    dev.setVibratoRate(1.0f); // fastest (~10 Hz)
    dev.setVibratoDepth(1.0f); // deepest
    dev.setVibratoDelay(0.0f);
    dev.processMidiNoteOn(69, 100); // A4, 440 Hz, short period for fine-grained zero-crossing measurement

    const uint32_t frameCount { 22050 }; // 0.5s: several vibrato cycles at ~10 Hz
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    auto ctx = makeContext(buffer, frameCount);
    dev.processAudio(ctx);

    // Measure over windows well short of one vibrato cycle and take the spread across the whole
    // render. Comparing two quarter-buffer windows instead would average out most of a ~10 Hz
    // modulation and leave what is left dependent on where those two windows happen to fall in its
    // phase, which is not what this test is about.
    constexpr int windows { 20 };
    double shortest { std::numeric_limits<double>::max() };
    double longest { 0.0 };
    for (int window = 0; window < windows; window++) {
        const double period { averageZeroCrossingPeriod(buffer, frameCount * window / windows, frameCount * (window + 1) / windows) };
        QVERIFY(period > 0.0);
        shortest = std::min(shortest, period);
        longest = std::max(longest, period);
    }

    // A semitone of vibrato is about 6 % peak to peak on the period.
    QVERIFY2(longest - shortest > shortest * 0.03,
             QString("Pitch only moved from %1 to %2 samples per cycle").arg(shortest).arg(longest).toUtf8().constData());
}

void StringVoiceTest::test_ensembleMode_shouldSupportChorusIPlusII()
{
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setEnsembleMode(2); // Chorus I + II
    QCOMPARE(dev.ensembleMode(), 2);
}

void StringVoiceTest::test_voiceRegisters_shouldFollowTheKeyboardSplit()
{
    // The keyboard splits at C4 the way the hardware's does: below it a note sounds Male 8' and 4',
    // at or above it Male 8' and Female 4'. Neither pair may sound on the other side of the split.
    auto renderPeak = [](StringVoiceDevice & dev) {
        std::vector<double> buffer(512 * 2, 0.0);
        auto ctx = makeContext(buffer, 512);
        dev.processAudio(ctx);
        return peakLevel(buffer);
    };

    auto onlyRegister = [](StringVoiceDevice & dev, float male8, float male4, float upperMale8, float female4) {
        dev.setStringsLevel8(0.0f);
        dev.setStringsLevel4(0.0f);
        dev.setVoiceMale8(male8);
        dev.setVoiceMale4(male4);
        dev.setVoiceUpperMale8(upperMale8);
        dev.setVoiceFemale4(female4);
        dev.setVoiceAttack(0.0f);
        dev.setEnsembleEnabled(false);
    };

    StringVoiceDevice male4Lower { "Male4 lower" };
    onlyRegister(male4Lower, 0.0f, 1.0f, 0.0f, 0.0f);
    male4Lower.processMidiNoteOn(48, 100);
    QVERIFY2(renderPeak(male4Lower) > 0.001, "Male 4' was silent below the split, where it belongs");

    StringVoiceDevice male4Upper { "Male4 upper" };
    onlyRegister(male4Upper, 0.0f, 1.0f, 0.0f, 0.0f);
    male4Upper.processMidiNoteOn(72, 100);
    QVERIFY2(renderPeak(male4Upper) < 0.001, "Male 4' sounded above the split, where the hardware has none");

    StringVoiceDevice femaleUpper { "Female4 upper" };
    onlyRegister(femaleUpper, 0.0f, 0.0f, 0.0f, 1.0f);
    femaleUpper.processMidiNoteOn(72, 100);
    QVERIFY2(renderPeak(femaleUpper) > 0.001, "Female 4' was silent above the split, where it belongs");

    StringVoiceDevice femaleLower { "Female4 lower" };
    onlyRegister(femaleLower, 0.0f, 0.0f, 0.0f, 1.0f);
    femaleLower.processMidiNoteOn(48, 100);
    QVERIFY2(renderPeak(femaleLower) < 0.001, "The female voice sounded below the split, which the hardware never does");

    StringVoiceDevice upperMale { "UpperMale8" };
    onlyRegister(upperMale, 0.0f, 0.0f, 1.0f, 0.0f);
    upperMale.processMidiNoteOn(72, 100);
    QVERIFY2(renderPeak(upperMale) > 0.001, "Upper Male 8' was silent above the split");

    StringVoiceDevice silentDev { "Silent" };
    silentDev.setStringsLevel8(0.0f);
    silentDev.setStringsLevel4(0.0f);
    silentDev.setVoiceMale8(0.0f);
    silentDev.setVoiceMale4(0.0f);
    silentDev.setVoiceUpperMale8(0.0f);
    silentDev.setVoiceFemale4(0.0f);
    silentDev.setVoiceAttack(0.0f);
    silentDev.setEnsembleEnabled(false);
    silentDev.processMidiNoteOn(60, 100);
    QCOMPARE(renderPeak(silentDev), 0.0);
}

void StringVoiceTest::test_balance_shouldScaleEachSectionIndependently()
{
    // One level per section, the hardware's Balance sliders. Turning one down must not touch the
    // other, which is the whole point of not having to reach for each register's footage.
    const auto render = [](float stringsBalance, float voiceBalance) {
        StringVoiceDevice dev { "Test StringVoice" };
        dev.setSampleRate(44100);
        dev.setStringsLevel8(1.0f);
        dev.setVoiceMale8(1.0f);
        dev.setStringsAttack(0.0f);
        dev.setVoiceAttack(0.0f);
        dev.setEnsembleEnabled(false);
        dev.setStringsBalance(stringsBalance);
        dev.setVoiceBalance(voiceBalance);
        dev.processMidiNoteOn(48, 100);

        constexpr uint32_t frameCount { 8192 };
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        auto ctx = makeContext(buffer, frameCount);
        dev.processAudio(ctx);
        return buffer;
    };

    const double both { rmsLevel(render(1.0f, 1.0f)) };
    const double stringsOnly { rmsLevel(render(1.0f, 0.0f)) };
    const double voiceOnly { rmsLevel(render(0.0f, 1.0f)) };
    const double neither { rmsLevel(render(0.0f, 0.0f)) };

    QVERIFY(both > 0.0);
    QVERIFY2(stringsOnly > 0.0 && voiceOnly > 0.0,
             QString("A section fell silent with its own balance up: strings %1, voice %2").arg(stringsOnly).arg(voiceOnly).toUtf8().constData());
    QVERIFY2(neither < both * 0.001,
             QString("Both balances down still gave %1 against %2").arg(neither).arg(both).toUtf8().constData());
    QVERIFY(stringsOnly < both);
    QVERIFY(voiceOnly < both);

    // Half of the strings section must land halfway in level, not somewhere the voice section moved.
    const double halfStrings { rmsLevel(render(0.5f, 0.0f)) };
    QVERIFY2(std::abs(halfStrings - stringsOnly * 0.5) < stringsOnly * 0.05,
             QString("Half balance gave %1, expected about %2").arg(halfStrings).arg(stringsOnly * 0.5).toUtf8().constData());
}

void StringVoiceTest::test_formants_male_shouldPeakAtOhFrequencies()
{
    // The male register sings an /o/, F1 700 and F2 900 Hz, which is where a VC340 recording of
    // the same registers puts that machine's formant region. It sang a textbook /u/ an octave below
    // until that measurement, which left it 20 - 32 dB short of the hardware right where the
    // hardware sings, and reading as a filtered string rather than a voice.
    constexpr uint32_t frames { 32768 };
    const auto buffer { renderRegister(true, frames) };
    const uint32_t start { frames / 4 };
    const uint32_t window { frames / 2 };

    // Bands wide enough to hold partials of the note being played: C3's land 131 Hz apart, so a
    // band narrower than that can fall between two of them and measure only the skirts.
    const double belowFormants { bandEnergy(buffer, start, window, 200.0, 450.0) };
    const double formants { bandEnergy(buffer, start, window, 600.0, 1050.0) };
    const double top { bandEnergy(buffer, start, window, 4000.0, 6000.0) };

    QVERIFY2(formants > belowFormants * 10.0,
             QString("The formant region (%1) does not dominate what lies below it (%2)").arg(formants).arg(belowFormants).toUtf8().constData());
    QVERIFY2(top < formants * 1.0e-3,
             QString("Too much above the formants: %1 at 4 - 6 kHz against %2 across them").arg(top).arg(formants).toUtf8().constData());
}

void StringVoiceTest::test_formants_female_shouldNotNotchBetweenPeaks()
{
    // Summing the resonances in phase cancelled between the peaks: the female register measured a
    // 28 dB hole between F1 and F2 where a real vowel has a valley of a few decibels.
    //
    // Measured on the bank itself rather than on a rendered note. The female voice only sounds an
    // octave above the keyboard split now, so its partials are too far apart to land either side of
    // a valley this narrow.
    constexpr int responseLength { 8192 };
    constexpr double sampleRate { 44100.0 };
    FormantFilterBank bank;
    bank.setSampleRate(sampleRate);

    std::vector<double> response(responseLength, 0.0);
    for (int i = 0; i < responseLength; i++) {
        double male { 0.0 };
        double female { 0.0 };
        bank.process(0.0, i == 0 ? 1.0 : 0.0, male, female);
        response[i] = female;
    }

    const auto magnitudeAt = [&response](double frequency) {
        double re { 0.0 };
        double im { 0.0 };
        for (size_t i = 0; i < response.size(); i++) {
            const double omega { 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / sampleRate };
            re += response[i] * std::cos(omega);
            im -= response[i] * std::sin(omega);
        }
        return std::hypot(re, im);
    };

    const double f1 { magnitudeAt(850.0) };
    const double valley { magnitudeAt(1030.0) };

    QVERIFY2(valley > f1 * 0.1,
             QString("Notch between F1 (%1) and F2: %2 in between").arg(f1).arg(valley).toUtf8().constData());
}

void StringVoiceTest::test_voiceStealing_shouldRemainStableWhenOversubscribed()
{
    // Regression test: voice allocation used to steal in a blind round-robin
    // that could cut a note still in full sustain (and did so long before the
    // 32-voice pool was even exhausted, which combined with the missing
    // headroom compensation made the device feel "unplayable" with more than
    // a couple of notes). Playing well beyond the pool size must keep
    // producing sane, bounded, NaN-free audio.
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setStringsAttack(0.0f);
    dev.setVoiceAttack(0.0f);
    dev.setStringsRelease(1.0f);
    dev.setVoiceRelease(1.0f);
    dev.setEnsembleEnabled(false);

    for (uint8_t note = 24; note < 24 + 40; ++note) {
        dev.processMidiNoteOn(note, 100);
    }
    QVERIFY(dev.hasActiveAudio());

    std::vector<double> buffer(512 * 2, 0.0);
    auto ctx = makeContext(buffer, 512);
    dev.processAudio(ctx);

    for (const double s : buffer) {
        QVERIFY(!std::isnan(s));
    }
    QVERIFY(peakLevel(buffer) < 2.0);
    QVERIFY(dev.hasActiveAudio());
}

void StringVoiceTest::test_hpfAndLpf_shouldAttenuateSignal()
{
    auto renderRms = [](StringVoiceDevice & dev) {
        std::vector<double> buffer(2048 * 2, 0.0);
        auto ctx = makeContext(buffer, 2048);
        dev.processAudio(ctx);
        return rmsLevel(buffer);
    };

    StringVoiceDevice openDev { "Open" };
    openDev.setStringsAttack(0.0f);
    openDev.setVoiceAttack(0.0f);
    openDev.setEnsembleEnabled(false);
    openDev.processMidiNoteOn(69, 100);
    const double openRms { renderRms(openDev) };
    QVERIFY(openRms > 0.001);

    StringVoiceDevice closedLpfDev { "ClosedLpf" };
    closedLpfDev.setStringsAttack(0.0f);
    closedLpfDev.setVoiceAttack(0.0f);
    closedLpfDev.setEnsembleEnabled(false);
    closedLpfDev.setLpfCutoff(0.0f);
    closedLpfDev.processMidiNoteOn(69, 100);
    QVERIFY(renderRms(closedLpfDev) < openRms * 0.5);

    StringVoiceDevice openHpfDev { "OpenHpf" };
    openHpfDev.setStringsAttack(0.0f);
    openHpfDev.setVoiceAttack(0.0f);
    openHpfDev.setEnsembleEnabled(false);
    openHpfDev.setHpfCutoff(1.0f);
    openHpfDev.processMidiNoteOn(69, 100);
    QVERIFY(renderRms(openHpfDev) < openRms * 0.5);
}

void StringVoiceTest::test_panSpread_shouldCreateStereoSeparation()
{
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setSampleRate(44100);
    dev.setStringsAttack(0.0f);
    dev.setVoiceAttack(0.0f);
    dev.setEnsembleEnabled(false); // disable chorus to isolate pan spread

    // With panSpread 0, left and right channels must be identical (mono)
    dev.setPanSpread(0.0f);
    dev.processMidiNoteOn(60, 100);
    std::vector<double> bufferMono(512 * 2, 0.0);
    auto ctxMono = makeContext(bufferMono, 512);
    dev.processAudio(ctxMono);

    for (uint32_t i = 0; i < 512; ++i) {
        QCOMPARE(bufferMono[i * 2], bufferMono[i * 2 + 1]);
    }

    dev.processMidiAllNotesOff();
    renderFrames(dev, 44100);

    // With panSpread 1.0, a single voice should be panned entirely to one side
    dev.setPanSpread(1.0f);
    dev.processMidiNoteOn(60, 100); // idx will be 0, side will be -1.0, pan will be 0.0
    std::vector<double> bufferStereo(512 * 2, 0.0);
    auto ctxStereo = makeContext(bufferStereo, 512);
    dev.processAudio(ctxStereo);

    // If pan is 0.0, the signal is entirely on the Left channel (Right should be silent)
    double peakL { 0.0 };
    double peakR { 0.0 };
    for (uint32_t i = 0; i < 512; ++i) {
        peakL = std::max(peakL, std::abs(bufferStereo[i * 2]));
        peakR = std::max(peakR, std::abs(bufferStereo[i * 2 + 1]));
    }
    QVERIFY(peakL > 0.001);
    QCOMPARE(peakR, 0.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::StringVoiceTest)
