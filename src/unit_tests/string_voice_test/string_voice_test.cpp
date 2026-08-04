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
    dev.setVoiceFemale8(male ? 0.0f : 1.0f);
    dev.setVoiceAttack(0.0f);
    dev.setEnsembleEnabled(false);
    dev.setVibratoDepth(0.0f);
    dev.processMidiNoteOn(48, 100); // C3, well below every formant

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
    StringVoiceDevice dev { "Test StringVoice" };
    dev.setStringsAttack(0.0f);
    dev.setVoiceAttack(0.0f);
    dev.setEnsembleEnabled(false);
    dev.processMidiNoteOn(60, 100);
    dev.processMidiNoteOn(64, 100);

    const uint32_t frameCount { 256 };
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
        dev.setStringsLevel8(0.45f);
        dev.setStringsLevel4(0.6f);
        dev.setVoiceMale4(0.3f);
        dev.setVoiceFemale8(0.7f);
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

        QCOMPARE(dev.stringsLevel8(), 0.45f);
        QCOMPARE(dev.stringsLevel4(), 0.6f);
        QCOMPARE(dev.voiceMale4(), 0.3f);
        QCOMPARE(dev.voiceFemale8(), 0.7f);
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

void StringVoiceTest::test_voiceRegisters_shouldRouteMale4AndFemale8Independently()
{
    // Male 4' and Female 8' used to be unreachable: the choir path only ever
    // wired the 8' oscillator into the Male mix and the 4' oscillator into
    // the Female mix. Verify each new register independently produces audio,
    // and that silence results when nothing at all is routed.
    auto renderPeak = [](StringVoiceDevice & dev) {
        std::vector<double> buffer(512 * 2, 0.0);
        auto ctx = makeContext(buffer, 512);
        dev.processAudio(ctx);
        return peakLevel(buffer);
    };

    StringVoiceDevice male4Dev { "Male4" };
    male4Dev.setStringsLevel8(0.0f);
    male4Dev.setStringsLevel4(0.0f);
    male4Dev.setVoiceMale8(0.0f);
    male4Dev.setVoiceMale4(1.0f);
    male4Dev.setVoiceFemale8(0.0f);
    male4Dev.setVoiceFemale4(0.0f);
    male4Dev.setVoiceAttack(0.0f);
    male4Dev.setEnsembleEnabled(false);
    male4Dev.processMidiNoteOn(60, 100);
    QVERIFY(renderPeak(male4Dev) > 0.001);

    StringVoiceDevice female8Dev { "Female8" };
    female8Dev.setStringsLevel8(0.0f);
    female8Dev.setStringsLevel4(0.0f);
    female8Dev.setVoiceMale8(0.0f);
    female8Dev.setVoiceMale4(0.0f);
    female8Dev.setVoiceFemale8(1.0f);
    female8Dev.setVoiceFemale4(0.0f);
    female8Dev.setVoiceAttack(0.0f);
    female8Dev.setEnsembleEnabled(false);
    female8Dev.processMidiNoteOn(60, 100);
    QVERIFY(renderPeak(female8Dev) > 0.001);

    StringVoiceDevice silentDev { "Silent" };
    silentDev.setStringsLevel8(0.0f);
    silentDev.setStringsLevel4(0.0f);
    silentDev.setVoiceMale8(0.0f);
    silentDev.setVoiceMale4(0.0f);
    silentDev.setVoiceFemale8(0.0f);
    silentDev.setVoiceFemale4(0.0f);
    silentDev.setVoiceAttack(0.0f);
    silentDev.setEnsembleEnabled(false);
    silentDev.processMidiNoteOn(60, 100);
    QCOMPARE(renderPeak(silentDev), 0.0);
}

void StringVoiceTest::test_formants_male_shouldPeakAtOohFrequencies()
{
    // The male register sings an /u/: F1 325, F2 700, F3 2530 Hz. F3 is the one that decides whether
    // the ear hears a voice at all, and it used to sit at 1300 Hz with nothing above 2 kHz.
    constexpr uint32_t frames { 32768 };
    const auto buffer { renderRegister(true, frames) };
    const uint32_t start { frames / 4 };
    const uint32_t window { frames / 2 };

    // Bands wide enough to hold partials of the note being played: C3's land 131 Hz apart, so a
    // band narrower than that can fall between two of them and measure only the skirts.
    const double f1 { bandEnergy(buffer, start, window, 250.0, 450.0) };
    const double f2 { bandEnergy(buffer, start, window, 600.0, 820.0) };
    const double f3 { bandEnergy(buffer, start, window, 2300.0, 2700.0) };
    const double aboveF2 { bandEnergy(buffer, start, window, 1800.0, 2150.0) };

    QVERIFY2(f1 > f2 * 4.0,
             QString("F1 (%1) is not clearly above F2 (%2), which an /u/ needs").arg(f1).arg(f2).toUtf8().constData());
    QVERIFY2(f3 > aboveF2 * 2.0,
             QString("No F3: %1 at 2.4 - 2.65 kHz against %2 just below it").arg(f3).arg(aboveF2).toUtf8().constData());
}

void StringVoiceTest::test_formants_female_shouldNotNotchBetweenPeaks()
{
    // Summing the resonances in phase cancelled between the peaks: the female register measured a
    // 28 dB hole between F1 and F2 where a real vowel has a valley of a few decibels.
    constexpr uint32_t frames { 32768 };
    const auto buffer { renderRegister(false, frames) };
    const uint32_t start { frames / 4 };
    const uint32_t window { frames / 2 };

    const double f1 { bandEnergy(buffer, start, window, 750.0, 950.0) };
    const double valley { bandEnergy(buffer, start, window, 980.0, 1120.0) };

    QVERIFY2(valley > f1 * 0.05,
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
