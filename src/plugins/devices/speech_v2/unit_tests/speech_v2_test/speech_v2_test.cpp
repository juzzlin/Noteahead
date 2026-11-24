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

#include "speech_v2_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/fft.hpp"
#include "../../../../../domain/dsp/speech/speech_sequencer.hpp"
#include "../../../../../infra/midi/midi_cc_mapping.hpp"
#include "../../../speech/speech_device.hpp"
#include "../../speech_v2_device.hpp"
#include "../../speech_v2_dialog_model.hpp"

#include <QSignalSpy>
#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };

std::vector<double> renderDevice(Device & device, uint32_t frames, double bpm = 120.0)
{
    std::vector<double> buffer(frames * 2, 0.0);
    AudioContext context { std::span<double>(buffer.data(), buffer.size()), frames, static_cast<uint32_t>(SampleRate), bpm, {}, 1, false };
    device.processAudio(context);
    return buffer;
}

double rmsMono(const std::vector<double> & buffer)
{
    if (buffer.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    size_t count = 0;
    for (size_t i = 0; i < buffer.size(); i += 2) {
        sum += buffer[i] * buffer[i];
        count++;
    }
    return count > 0 ? std::sqrt(sum / static_cast<double>(count)) : 0.0;
}

double peakAmplitude(const std::vector<double> & buffer)
{
    double peak = 0.0;
    for (const double s : buffer) {
        peak = std::max(peak, std::abs(s));
    }
    return peak;
}

double preciseFundamental(const std::vector<double> & buffer)
{
    std::vector<double> mono;
    for (size_t i = 0; i < buffer.size(); i += 2) {
        mono.push_back(buffer[i]);
    }
    const auto minLag = static_cast<size_t>(SampleRate / 600.0);
    const auto maxLag = static_cast<size_t>(SampleRate / 50.0);
    if (mono.size() < maxLag * 2) {
        return 0.0;
    }
    std::vector<double> corr(maxLag + 2, 0.0);
    for (size_t lag = minLag; lag <= maxLag + 1; lag++) {
        double sum = 0.0;
        for (size_t i = 0; i + lag < mono.size(); i++) {
            sum += mono[i] * mono[i + lag];
        }
        corr[lag] = sum;
    }
    size_t best = minLag;
    for (size_t lag = minLag + 1; lag <= maxLag; lag++) {
        if (corr[lag] > corr[best]) {
            best = lag;
        }
    }
    const double a = corr[best - 1];
    const double b = corr[best];
    const double c = corr[best + 1];
    const double denom = a - 2.0 * b + c;
    const double offset = denom != 0.0 ? 0.5 * (a - c) / denom : 0.0;
    return SampleRate / (static_cast<double>(best) + offset);
}

double spectralCentroid(const std::vector<double> & buffer)
{
    constexpr int size = 16384;
    std::vector<double> re(size, 0.0);
    std::vector<double> im(size, 0.0);
    for (int i = 0; i < size && static_cast<size_t>(i) * 2 < buffer.size(); i++) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (size - 1));
        re[static_cast<size_t>(i)] = buffer[static_cast<size_t>(i) * 2] * window;
    }
    Fft::forward(re.data(), im.data(), size);

    const auto low = static_cast<size_t>(std::lround(100.0 * size / SampleRate));
    const auto high = static_cast<size_t>(std::lround(8000.0 * size / SampleRate));
    double weighted = 0.0;
    double total = 0.0;
    for (size_t i = low; i <= high; i++) {
        const double mag = std::hypot(re[i], im[i]);
        weighted += mag * static_cast<double>(i) * SampleRate / size;
        total += mag;
    }
    return total > 0.0 ? weighted / total : 0.0;
}

double magnitudeNear(const std::vector<double> & buffer, double hz)
{
    constexpr int size = 32768;
    std::vector<double> re(size, 0.0);
    std::vector<double> im(size, 0.0);
    for (int i = 0; i < size && static_cast<size_t>(i) * 2 < buffer.size(); i++) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (size - 1));
        re[static_cast<size_t>(i)] = buffer[static_cast<size_t>(i) * 2] * window;
    }
    Fft::forward(re.data(), im.data(), size);

    const auto centre = static_cast<size_t>(std::lround(hz * size / SampleRate));
    double best = 0.0;
    for (size_t b = centre - 2; b <= centre + 2; b++) {
        best = std::max(best, std::hypot(re[b], im[b]));
    }
    return best;
}

std::vector<double> heldVowel(SpeechV2Device & device, uint8_t note)
{
    device.setPhrase("/AA/");
    device.setIntonation(0.0f);
    device.setVibratoDepth(0.0f);
    device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Fit));
    device.processDeviceMidiNoteOn(note, 100);
    renderDevice(device, 16384);
    return renderDevice(device, 65536);
}

} // namespace

void SpeechV2Test::test_noteOn_shouldSoundAndGoSilentAfterPhrase()
{
    SpeechV2Device device { "Speech V2" };
    QVERIFY(!device.hasActiveAudio());

    device.processDeviceMidiNoteOn(48, 100);
    QVERIFY(device.hasActiveAudio());
    const auto activeAudio = renderDevice(device, static_cast<uint32_t>(SampleRate * 0.5));
    QVERIFY(rmsMono(activeAudio) > 0.01);

    // Default phrase "hello world" finishes within ~2 seconds; after fade, device goes idle
    renderDevice(device, static_cast<uint32_t>(SampleRate * 2.5));
    QVERIFY(!device.hasActiveAudio());
    const auto tail = renderDevice(device, 2048);
    QCOMPARE(peakAmplitude(tail), 0.0);
}

void SpeechV2Test::test_noteOn_shouldFollowNotePitch_data()
{
    QTest::addColumn<int>("midiNote");
    QTest::addColumn<double>("expectedHz");

    QTest::newRow("C3") << 48 << 130.8128;
    QTest::newRow("A3") << 57 << 220.0;
    QTest::newRow("C4") << 60 << 261.6256;
}

void SpeechV2Test::test_noteOn_shouldFollowNotePitch()
{
    QFETCH(int, midiNote);
    QFETCH(double, expectedHz);

    SpeechV2Device device { "Speech V2" };
    device.setVoicePerturbation(0.0f);
    const auto audio = heldVowel(device, static_cast<uint8_t>(midiNote));
    const double measured = preciseFundamental(audio);
    const double cents = 1200.0 * std::log2(measured / expectedHz);
    QVERIFY2(std::abs(cents) < 15.0, qPrintable(QString { "%1 Hz (%2 c)" }.arg(measured).arg(cents)));
}

void SpeechV2Test::test_velocitySensitivity_shouldScaleSoftNote()
{
    const auto renderAtVel = [](float sens, uint8_t vel) {
        SpeechV2Device device { "Speech V2" };
        device.setVelocitySensitivity(sens);
        device.setPhrase("/AA/");
        device.processDeviceMidiNoteOn(48, vel);
        renderDevice(device, 4096);
        return rmsMono(renderDevice(device, 16384));
    };

    const double insensitive = renderAtVel(0.0f, 32);
    const double sensitiveLoud = renderAtVel(1.0f, 127);
    const double sensitiveSoft = renderAtVel(1.0f, 32);
    QVERIFY(std::abs(insensitive - sensitiveLoud) < 0.05 * sensitiveLoud);
    QVERIFY(sensitiveSoft < sensitiveLoud * 0.4);
}

void SpeechV2Test::test_voiceTypes_allNineShouldBeBoundedAndDistinct()
{
    std::vector<double> centroids;
    for (int type = 0; type <= 8; type++) {
        SpeechV2Device device { "Speech V2" };
        device.setVoiceType(type);
        device.processDeviceMidiNoteOn(48, 100);
        const auto audio = renderDevice(device, static_cast<uint32_t>(SampleRate * 1.0));
        for (const double sample : audio) {
            QVERIFY(std::isfinite(sample));
        }
        const double peak = peakAmplitude(audio);
        QVERIFY2(peak > 0.01 && peak < 1.0, qPrintable(QString { "type %1 peak %2" }.arg(type).arg(peak)));

        SpeechV2Device vowelDevice { "Speech V2" };
        vowelDevice.setVoiceType(type);
        centroids.push_back(spectralCentroid(heldVowel(vowelDevice, 48)));
    }

    // Female (1) brighter than Male (0), Child (2) higher than Female (1), Giant (5) lower than Male (0)
    QVERIFY(centroids[1] > centroids[0]);
    QVERIFY(centroids[2] > centroids[1]);
    QVERIFY(centroids[5] < centroids[0]);

    // Monster (8) produces subharmonic at half fundamental
    SpeechV2Device monster { "Speech V2" };
    monster.setVoiceType(8);
    const auto monsterAudio = heldVowel(monster, 57); // A3 = 220 Hz, sub = 110 Hz
    SpeechV2Device modal { "Speech V2" };
    modal.setVoiceType(0);
    const auto modalAudio = heldVowel(modal, 57);
    const double monsterSub = magnitudeNear(monsterAudio, 110.0);
    const double modalSub = magnitudeNear(modalAudio, 110.0);
    QVERIFY2(monsterSub > modalSub * 5.0, qPrintable(QString { "monster=%1 vs modal=%2" }.arg(monsterSub).arg(modalSub)));
}

void SpeechV2Test::test_loudness_helloWorld_shouldMatchSpeechV1()
{
    SpeechDevice v1 { "Speech" };
    v1.processDeviceMidiNoteOn(48, 100);
    const auto audio1 = renderDevice(v1, static_cast<uint32_t>(SampleRate * 1.5));

    SpeechV2Device v2 { "Speech V2" };
    v2.processDeviceMidiNoteOn(48, 100);
    const auto audio2 = renderDevice(v2, static_cast<uint32_t>(SampleRate * 1.5));

    const double rms1 = rmsMono(audio1);
    const double rms2 = rmsMono(audio2);
    const double diffDb = 20.0 * std::log10(std::max(rms2, 1e-12) / std::max(rms1, 1e-12));
    const double peak2 = peakAmplitude(audio2);
    QVERIFY2(std::abs(diffDb) < 3.0, qPrintable(QString { "V2 vs V1: %1 dB (peak2=%2)" }.arg(diffDb).arg(peak2)));
    QVERIFY2(peak2 < 1.0, qPrintable(QString::number(peak2)));
}

void SpeechV2Test::test_midiCc_shouldReachParametersAndResetCleanly()
{
    using namespace MidiCcMapping;
    SpeechV2Device device { "Speech V2" };
    QCOMPARE(device.deviceMidiCcControllers().size(), size_t { 5 });

    device.processDeviceMidiCc(static_cast<uint8_t>(Controller::ChannelVolumeMSB), 64, 0);
    QVERIFY(device.volume() < 0.7f);

    device.processDeviceMidiCc(static_cast<uint8_t>(Controller::PanMSB), 127, 0);
    QVERIFY(device.pan() > 0.9f);

    device.processDeviceMidiCc(static_cast<uint8_t>(Controller::SoundController5), 32, 0);
    QVERIFY(device.lpfCutoff() < 0.4f);

    device.processDeviceMidiCc(static_cast<uint8_t>(Controller::GeneralPurpose6), 64, 0);
    QVERIFY(device.hpfCutoff() > 0.4f);

    device.processDeviceMidiCc(static_cast<uint8_t>(Controller::SoundController2), 127, 0);
    QVERIFY(device.formantShift() > 0.9f);

    device.processDeviceMidiCc(static_cast<uint8_t>(Controller::ResetAllControllers), 0, 0);
    QVERIFY(std::abs(device.formantShift() - 0.5f) < 0.01f);
}

void SpeechV2Test::test_triggerAndSyncModes_shouldBehaveAndSeek()
{
    SpeechV2Device device { "Speech V2" };
    device.setPhrase("Hello world. Second line.");
    QCOMPARE(device.lineCount(), size_t { 2 });
    QCOMPARE(device.syllableCount(), size_t { 8 }); // 6 spoken syllables plus 2 period pauses

    // Phrase mode: does not want note index seek
    QVERIFY(!device.wantsNoteIndexSeek());

    // Step mode: advances cursor per note
    device.setTriggerMode(static_cast<int>(SpeechSequencer::TriggerMode::Step));
    QCOMPARE(device.syllableCursor(), size_t { 0 });
    device.processDeviceMidiNoteOn(48, 100);
    QCOMPARE(device.syllableCursor(), size_t { 1 });
    device.processDeviceMidiNoteOff(48);

    // Line mode: seeks by note index and advances line cursor
    device.setTriggerMode(static_cast<int>(SpeechSequencer::TriggerMode::Line));
    QVERIFY(device.wantsNoteIndexSeek());
    device.seekToNoteIndex(1);
    QCOMPARE(device.lineCursor(), size_t { 1 });
    device.processDeviceMidiNoteOn(48, 100);
    QCOMPARE(device.lineCursor(), size_t { 0 });
    device.processDeviceMidiNoteOff(48);

    // Grid & Fit modes stay active for expected tempo bounds
    device.setTriggerMode(static_cast<int>(SpeechSequencer::TriggerMode::Phrase));
    device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Fit));
    device.setSyncLength(4); // 1 beat = 0.5 s at 120 BPM
    device.processDeviceMidiNoteOn(48, 100);
    renderDevice(device, static_cast<uint32_t>(SampleRate * 0.4));
    QVERIFY(device.hasActiveAudio());
}

void SpeechV2Test::test_vibrato_shouldRampInSmoothlyOnSustainedVowel()
{
    SpeechV2Device device { "Speech V2" };
    device.setPhrase("/AA/");
    device.setIntonation(0.0f);
    device.setVoicePerturbation(0.0f);
    device.setVibratoRate(0.6f);
    device.setVibratoDepth(1.0f);
    device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Fit));
    device.processDeviceMidiNoteOn(57, 100);

    const auto audio = renderDevice(device, static_cast<uint32_t>(SampleRate * 1.0));
    constexpr size_t windowFrames = 2048;
    constexpr size_t hopFrames = 480;
    const auto centsSwing = [&](size_t fromFrame, size_t toFrame) {
        double minCents = 1e9;
        double maxCents = -1e9;
        for (size_t start = fromFrame; start + windowFrames <= toFrame; start += hopFrames) {
            const std::vector<double> slice(
              audio.begin() + static_cast<long>(start * 2),
              audio.begin() + static_cast<long>((start + windowFrames) * 2));
            const double f0 = preciseFundamental(slice);
            const double cents = 1200.0 * std::log2(std::max(1.0, f0) / 220.0);
            minCents = std::min(minCents, cents);
            maxCents = std::max(maxCents, cents);
        }
        return maxCents - minCents;
    };

    const double onsetSwing = centsSwing(240, 3200); // ~5..67 ms
    const double sustainedSwing = centsSwing(16800, 43200); // ~350..900 ms
    QVERIFY2(sustainedSwing > onsetSwing * 3.0 && sustainedSwing > 35.0,
             qPrintable(QString { "onset=%1 st, sustained=%2 st" }.arg(onsetSwing / 100.0).arg(sustainedSwing / 100.0)));
}

void SpeechV2Test::test_prosody_questionShouldRiseAboveStatement()
{
    const auto finalWordPitch = [](const std::string & phrase) {
        SpeechV2Device device { "Speech V2" };
        device.setPhrase(phrase);
        device.setIntonation(0.8f);
        device.setVoicePerturbation(0.0f);
        device.setVibratoDepth(0.0f);
        device.processDeviceMidiNoteOn(48, 100);
        const auto audio = renderDevice(device, static_cast<uint32_t>(SampleRate * 1.2));

        constexpr size_t win = 2048;
        constexpr size_t hop = 960;
        double lastActiveHz = 0.0;
        for (size_t start = static_cast<size_t>(SampleRate * 0.40); start + win <= audio.size() / 2; start += hop) {
            const std::vector<double> slice(
              audio.begin() + static_cast<long>(start * 2),
              audio.begin() + static_cast<long>((start + win) * 2));
            if (rmsMono(slice) > 0.005) {
                lastActiveHz = preciseFundamental(slice);
            }
        }
        return lastActiveHz;
    };

    const double qHz = finalWordPitch("Hello world?");
    const double sHz = finalWordPitch("Hello world.");
    QVERIFY(qHz > 0.0 && sHz > 0.0);
    const double diffCents = 1200.0 * std::log2(qHz / sHz);
    QVERIFY2(diffCents > 200.0, qPrintable(QString { "q=%1 Hz vs s=%2 Hz (%3 c)" }.arg(qHz).arg(sHz).arg(diffCents)));
}

void SpeechV2Test::test_stepModeDiphthong_shouldHoldNucleusUntilNoteOff()
{
    const auto centroidF2 = [](const std::vector<double> & interleaved, size_t offsetFrame, int fftSize) {
        std::vector<double> re(fftSize, 0.0);
        std::vector<double> im(fftSize, 0.0);
        for (int i = 0; i < fftSize && (offsetFrame + static_cast<size_t>(i)) * 2 < interleaved.size(); i++) {
            const double w = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (fftSize - 1));
            re[static_cast<size_t>(i)] = interleaved[(offsetFrame + static_cast<size_t>(i)) * 2] * w;
        }
        Fft::forward(re.data(), im.data(), fftSize);
        const size_t low = static_cast<size_t>(700.0 * fftSize / SampleRate);
        const size_t high = static_cast<size_t>(2800.0 * fftSize / SampleRate);
        double weighted = 0.0;
        double total = 0.0;
        for (size_t b = low; b <= high; b++) {
            const double p = re[b] * re[b] + im[b] * im[b];
            weighted += p * static_cast<double>(b) * SampleRate / fftSize;
            total += p;
        }
        return total > 0.0 ? weighted / total : 0.0;
    };

    SpeechV2Device device { "Speech V2" };
    device.setPhrase("/AY/");
    device.setTriggerMode(static_cast<int>(SpeechSequencer::TriggerMode::Step));
    device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Free));
    device.setIntonation(0.0f);
    device.setVoicePerturbation(0.0f);
    device.processDeviceMidiNoteOn(48, 100);

    const auto held = renderDevice(device, static_cast<uint32_t>(SampleRate * 0.5));
    QVERIFY(device.hasActiveAudio());
    const double heldF2 = centroidF2(held, 24000 - 4096, 4096);
    QVERIFY2(heldF2 < 1500.0, qPrintable(QString { "heldF2=%1 Hz" }.arg(heldF2)));

    device.processDeviceMidiNoteOff(48);
    const auto released = renderDevice(device, static_cast<uint32_t>(SampleRate * 0.22));
    QVERIFY(device.hasActiveAudio());
    const double releaseF2 = centroidF2(released, 7500, 2048);
    QVERIFY2(releaseF2 > heldF2 + 350.0, qPrintable(QString { "heldF2=%1 vs releaseF2=%2" }.arg(heldF2).arg(releaseF2)));

    renderDevice(device, static_cast<uint32_t>(SampleRate * 2.5));
    const auto tail = renderDevice(device, 4096);
    QVERIFY(!device.hasActiveAudio());
    QCOMPARE(peakAmplitude(tail), 0.0);
}

void SpeechV2Test::test_midiPitchBend_shouldShiftPitchByConfiguredRange()
{
    const auto measureBend = [](int range, uint16_t bendValue) {
        SpeechV2Device device { "Speech V2" };
        device.setPitchBendRange(range);
        device.processMidiPitchBend(bendValue, 0);
        return preciseFundamental(heldVowel(device, 57)); // A3 = 220 Hz
    };

    const double centerHz = measureBend(2, 8192);
    const double centerCents = 1200.0 * std::log2(centerHz / 220.0);
    QVERIFY2(std::abs(centerCents) < 15.0, qPrintable(QString::number(centerCents)));

    const double upHz = measureBend(2, 16383);
    const double upCents = 1200.0 * std::log2(upHz / 220.0);
    QVERIFY2(std::abs(upCents - 200.0) < 15.0, qPrintable(QString::number(upCents)));

    const double downHz = measureBend(7, 0);
    const double downCents = 1200.0 * std::log2(downHz / 220.0);
    QVERIFY2(std::abs(downCents + 700.0) < 15.0, qPrintable(QString::number(downCents)));
}

void SpeechV2Test::test_dialogModel_roundTripAndRequestSettings()
{
    const auto device = std::make_shared<SpeechV2Device>("Speech V2");
    SpeechV2DialogModel model { device };

    model.setPhrase("testing speech now");
    QCOMPARE(model.phrase(), QString { "testing speech now" });
    QVERIFY(!model.phrasePhonemes().isEmpty());
    QCOMPARE(model.syllableCount(), 4);

    model.setOpenQuotient(750);
    model.setVoicePerturbation(250);
    model.setFormantShift(600);
    model.setPitchBendRange(7);
    QCOMPARE(model.openQuotient(), 750);
    QCOMPARE(model.voicePerturbation(), 250);
    QCOMPARE(model.formantShift(), 600);
    QCOMPARE(model.pitchBendRange(), 7);

    QSignalSpy openSpy { &model, &SpeechV2DialogModel::openQuotientChanged };
    QSignalSpy jitterSpy { &model, &SpeechV2DialogModel::voicePerturbationChanged };
    QSignalSpy phraseSpy { &model, &SpeechV2DialogModel::phraseChanged };
    QSignalSpy rateSpy { &model, &SpeechV2DialogModel::rateChanged };
    QSignalSpy pbSpy { &model, &SpeechV2DialogModel::pitchBendRangeChanged };
    model.requestSettings();
    QCOMPARE(openSpy.count(), 1);
    QCOMPARE(jitterSpy.count(), 1);
    QCOMPARE(phraseSpy.count(), 1);
    QCOMPARE(rateSpy.count(), 1);
    QCOMPARE(pbSpy.count(), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SpeechV2Test)
