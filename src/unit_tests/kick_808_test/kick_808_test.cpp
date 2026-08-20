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

#include "kick_808_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/kick_808_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate = 48000;

//! Renders the left channel into a flat vector, so the tests can look at the waveform itself.
std::vector<double> renderMono(Kick808Device & kick, uint32_t frames)
{
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), frames, SampleRate };
    kick.processAudio(context);

    std::vector<double> mono(frames, 0.0);
    for (uint32_t i = 0; i < frames; i++) {
        mono[i] = buffer[i * 2];
    }
    return mono;
}

double peakLevel(const std::vector<double> & samples)
{
    double peak = 0.0;
    for (const double s : samples) {
        peak = std::max(peak, std::abs(s));
    }
    return peak;
}

//! Estimates the fundamental by counting upward zero crossings. The resonator tail is very nearly a
//! pure sine once the click and the pitch envelope are out of the way, so this is accurate enough
//! to tell semitones apart.
double estimateFrequency(const std::vector<double> & samples, uint32_t skipFrames)
{
    int crossings = 0;
    for (size_t i = skipFrames + 1; i < samples.size(); i++) {
        if (samples[i - 1] <= 0.0 && samples[i] > 0.0) {
            crossings++;
        }
    }
    const double seconds = static_cast<double>(samples.size() - skipFrames) / SampleRate;
    return crossings / seconds;
}

//! Energy between two frequencies, summed over log-spaced DFT probes. Enough to compare how bright
//! two hits are without pulling in an FFT.
double bandEnergy(const std::vector<double> & samples, double lowHz, double highHz)
{
    constexpr int probes = 12;
    double energy = 0.0;
    for (int probe = 0; probe < probes; probe++) {
        const double frequency = lowHz * std::pow(highHz / lowHz, static_cast<double>(probe) / (probes - 1));
        const double omega = 2.0 * std::numbers::pi * frequency / SampleRate;
        double re = 0.0;
        double im = 0.0;
        for (size_t i = 0; i < samples.size(); i++) {
            re += samples[i] * std::cos(omega * static_cast<double>(i));
            im -= samples[i] * std::sin(omega * static_cast<double>(i));
        }
        energy += re * re + im * im;
    }
    return energy;
}

//! Tapers the second half of a render to zero with a raised cosine.
//!
//! bandEnergy probes the buffer exactly as it stands, and a hit cut off mid-ring is a step at the
//! end of the buffer whose leakage spreads the fundamental over the whole spectrum - far above what
//! is left of the click two octaves up, so without this the top octaves measure the truncation
//! rather than the drum. The onset is deliberately left alone: the click's own step is the subject.
std::vector<double> fadedOut(const std::vector<double> & samples)
{
    std::vector<double> faded = samples;
    const size_t fade = faded.size() / 2;
    for (size_t i = 0; i < fade; i++) {
        faded[faded.size() - fade + i] *= 0.5 * (1.0 + std::cos(std::numbers::pi * static_cast<double>(i) / static_cast<double>(fade)));
    }
    return faded;
}

//! Frequency of the first half cycle of a hit, which is where the pitch envelope is at its
//! deepest. Half a period is all there is to measure at that point, so it is measured directly
//! rather than by counting crossings over a window the sweep would move under.
double firstHalfCycleFrequency(const std::vector<double> & samples)
{
    std::vector<double> crossings;
    for (size_t i = 1; i < samples.size() && crossings.size() < 3; i++) {
        if ((samples[i - 1] <= 0.0 && samples[i] > 0.0) || (samples[i - 1] >= 0.0 && samples[i] < 0.0)) {
            const double fraction = std::abs(samples[i - 1]) / (std::abs(samples[i - 1]) + std::abs(samples[i]));
            crossings.push_back((static_cast<double>(i - 1) + fraction) / SampleRate);
        }
    }
    return crossings.size() >= 2 ? 0.5 / (crossings.at(1) - crossings.at(0)) : 0.0;
}

//! Settles a voice for pitch measurements: no pitch envelope to bend the tail and a long decay so
//! there is plenty of waveform to count.
void makeSteady(Kick808Device & kick)
{
    kick.setPitchDepth(0.0f);
    kick.setDecay(1.0f);
    kick.setTone(0.0f);
}

} // namespace

void Kick808Test::test_midiNoteOn_shouldActivateAudio()
{
    Kick808Device kick { "Test Kick" };
    kick.processMidiNoteOn(36, 100);
    QVERIFY(kick.hasActiveAudio());
}

void Kick808Test::test_midiNoteOff_shouldNotStopRinging()
{
    Kick808Device kick { "Test Kick" };
    kick.setDecay(1.0f);
    kick.processMidiNoteOn(36, 100);
    renderMono(kick, 256);

    kick.processMidiNoteOff(36);
    renderMono(kick, 256);

    // The hit is one-shot: only Decay ends it, never the note length.
    QVERIFY(kick.hasActiveAudio());
}

void Kick808Test::test_allNotesOff_shouldChokeVoice()
{
    Kick808Device kick { "Test Kick" };
    kick.setDecay(1.0f);
    kick.processMidiNoteOn(36, 100);
    renderMono(kick, 256);
    QVERIFY(kick.hasActiveAudio());

    kick.processMidiAllNotesOff();
    renderMono(kick, SampleRate / 2);

    QVERIFY(!kick.hasActiveAudio());
}

void Kick808Test::test_decay_short_shouldProduceShorterTailThanLong()
{
    Kick808Device shortKick { "Short Kick" };
    shortKick.setDecay(0.0f);
    shortKick.processMidiNoteOn(36, 127);
    const auto shortTail = renderMono(shortKick, SampleRate / 2);

    Kick808Device longKick { "Long Kick" };
    longKick.setDecay(1.0f);
    longKick.processMidiNoteOn(36, 127);
    const auto longTail = renderMono(longKick, SampleRate / 2);

    // Measure what is left after 250 ms: the click setting should be long gone, the boom still there.
    const std::vector<double> shortLate { shortTail.begin() + SampleRate / 4, shortTail.end() };
    const std::vector<double> longLate { longTail.begin() + SampleRate / 4, longTail.end() };

    QVERIFY2(peakLevel(shortLate) < peakLevel(longLate) * 0.1,
             QString("Short tail (%1) is not clearly shorter than long tail (%2)").arg(peakLevel(shortLate)).arg(peakLevel(longLate)).toUtf8().constData());
}

void Kick808Test::test_decay_full_shouldRingForSeconds()
{
    Kick808Device kick { "Test Kick" };
    kick.setDecay(1.0f);
    kick.processMidiNoteOn(36, 127);

    // The full BOOM end of the range has to survive a full second.
    renderMono(kick, SampleRate);
    QVERIFY(kick.hasActiveAudio());
}

void Kick808Test::test_decay_half_shouldStillRingAfterAQuarterSecond()
{
    Kick808Device kick { "Test Kick" };
    kick.setDecay(0.5f);
    kick.processMidiNoteOn(36, 127);
    const auto rendered = renderMono(kick, SampleRate);

    const std::vector<double> early { rendered.begin() + SampleRate * 40 / 1000, rendered.begin() + SampleRate * 60 / 1000 };
    const std::vector<double> late { rendered.begin() + SampleRate / 4, rendered.begin() + SampleRate * 270 / 1000 };
    const double decayed = 20.0 * std::log10(peakLevel(late) / peakLevel(early));

    // Mid knob is the hardware's own mid knob: an RD-8 with Decay at half is some 15 dB down a
    // quarter of a second in, which is a -60 dB time near a second rather than a third of one.
    QVERIFY2(decayed > -22.0 && decayed < -9.0,
             QString("Level at 250 ms was %1 dB below 40 ms, expected around -15 dB").arg(decayed).toUtf8().constData());
}

void Kick808Test::test_pitchEnvelope_default_shouldStartWellAboveTheNote()
{
    Kick808Device kick { "Test Kick" };
    kick.processMidiNoteOn(36, 127);
    const auto rendered = renderMono(kick, SampleRate / 2);

    // C2 is 65.41 Hz, and the sweep has to carry the first swings up into the couple of hundred
    // hertz where the body of an 808 hit sits. Without that the drum is nothing but its fundamental.
    const double first = firstHalfCycleFrequency(rendered);
    QVERIFY2(first > 2.0 * 65.41,
             QString("First half cycle was %1 Hz, expected well over an octave above the note").arg(first).toUtf8().constData());
}

void Kick808Test::test_pitchEnvelope_default_shouldSettleWithinTwentyFiveMilliseconds()
{
    Kick808Device kick { "Test Kick" };
    kick.processMidiNoteOn(36, 127);
    const auto rendered = renderMono(kick, SampleRate / 10);

    // The sweep is a strike, not a glide: by 25 ms it has to be over and the drum sitting on its
    // note, or the hit reads as a falling tone rather than as a drum.
    const double settled = estimateFrequency(rendered, SampleRate * 25 / 1000);
    QVERIFY2(std::abs(settled / 65.41 - 1.0) < 0.05,
             QString("Pitch was still %1 Hz after 25 ms, expected C2 at 65.41 Hz").arg(settled).toUtf8().constData());
}

void Kick808Test::test_peakLevel_anySetting_shouldLeaveHeadroom()
{
    // The excitation is compensated at the frequency the sweep starts on, so a deeper sweep is also
    // a hotter hit. No corner of the Tone and Decay ranges may run a full-velocity hit out of room
    // for the fader, the pan or the drive stage.
    for (int toneStep = 0; toneStep <= 4; toneStep++) {
        for (int decayStep = 0; decayStep <= 4; decayStep++) {
            Kick808Device kick { "Test Kick" };
            kick.setTone(static_cast<float>(toneStep) / 4.0f);
            kick.setDecay(static_cast<float>(decayStep) / 4.0f);
            kick.processMidiNoteOn(36, 127);
            const double peak = peakLevel(renderMono(kick, SampleRate / 2));
            QVERIFY2(peak < 0.8,
                     QString("Peak was %1 at tone %2, decay %3").arg(peak).arg(toneStep / 4.0).arg(decayStep / 4.0).toUtf8().constData());
        }
    }
}

void Kick808Test::test_click_brightest_shouldKeepFallingTowardsNyquist()
{
    Kick808Device kick { "Test Kick" };
    kick.setTone(1.0f);
    kick.processMidiNoteOn(36, 127);
    const auto rendered = fadedOut(renderMono(kick, SampleRate / 4));

    // The click is a step, and a step left unfiltered carries as much level at 20 kHz as at 10 kHz.
    // A single pole does not fix that: its response flattens out towards Nyquist rather than
    // continuing to fall, which on a constant percentage bandwidth analyser reads as the top of the
    // spectrum ramping back up. The top octave has to stay well under the one below it even at the
    // Tone setting that opens the click furthest.
    const double upper = bandEnergy(rendered, 10000.0, 20000.0);
    const double lower = bandEnergy(rendered, 5000.0, 10000.0);
    const double fall = 10.0 * std::log10(upper / lower);
    QVERIFY2(fall < -12.0,
             QString("Top octave was only %1 dB under the one below it").arg(fall).toUtf8().constData());
}

void Kick808Test::test_keyTrack_enabled_shouldFollowNotePitch()
{
    Kick808Device low { "Low Kick" };
    makeSteady(low);
    low.setKeyTrack(true);
    low.processMidiNoteOn(36, 127);
    const double lowFrequency = estimateFrequency(renderMono(low, SampleRate / 2), SampleRate / 10);

    Kick808Device high { "High Kick" };
    makeSteady(high);
    high.setKeyTrack(true);
    high.processMidiNoteOn(48, 127);
    const double highFrequency = estimateFrequency(renderMono(high, SampleRate / 2), SampleRate / 10);

    // An octave up must double the fundamental.
    QVERIFY2(std::abs(highFrequency / lowFrequency - 2.0) < 0.1,
             QString("Octave ratio was %1 (%2 Hz vs %3 Hz)").arg(highFrequency / lowFrequency).arg(highFrequency).arg(lowFrequency).toUtf8().constData());
}

void Kick808Test::test_keyTrack_disabled_shouldIgnoreNotePitch()
{
    Kick808Device low { "Low Kick" };
    makeSteady(low);
    low.setKeyTrack(false);
    low.processMidiNoteOn(36, 127);
    const double lowFrequency = estimateFrequency(renderMono(low, SampleRate / 2), SampleRate / 10);

    Kick808Device high { "High Kick" };
    makeSteady(high);
    high.setKeyTrack(false);
    high.processMidiNoteOn(60, 127);
    const double highFrequency = estimateFrequency(renderMono(high, SampleRate / 2), SampleRate / 10);

    QVERIFY2(std::abs(highFrequency - lowFrequency) < 1.0,
             QString("Pitch moved with the note despite key track being off (%1 Hz vs %2 Hz)").arg(highFrequency).arg(lowFrequency).toUtf8().constData());

    // With Tune centred the fixed pitch is the reference note, C2.
    QVERIFY2(std::abs(lowFrequency - 65.41) < 1.5,
             QString("Fixed pitch was %1 Hz, expected C2").arg(lowFrequency).toUtf8().constData());
}

void Kick808Test::test_tune_raised_shouldRaisePitch()
{
    Kick808Device centred { "Centred Kick" };
    makeSteady(centred);
    centred.setKeyTrack(false);
    centred.processMidiNoteOn(36, 127);
    const double centredFrequency = estimateFrequency(renderMono(centred, SampleRate / 2), SampleRate / 10);

    // Tune spans +/- 24 semitones around the centre, so 0.75 is an octave up.
    Kick808Device raised { "Raised Kick" };
    makeSteady(raised);
    raised.setKeyTrack(false);
    raised.setTune(0.75f);
    raised.processMidiNoteOn(36, 127);
    const double raisedFrequency = estimateFrequency(renderMono(raised, SampleRate / 2), SampleRate / 10);

    QVERIFY2(std::abs(raisedFrequency / centredFrequency - 2.0) < 0.1,
             QString("Tune ratio was %1 (%2 Hz vs %3 Hz)").arg(raisedFrequency / centredFrequency).arg(raisedFrequency).arg(centredFrequency).toUtf8().constData());
}

void Kick808Test::test_tone_full_shouldRaiseHighFrequencyContent()
{
    // Tone is the click's top-end tilt. It used to shorten the excitation pulse the click was tied
    // to, which made opening the knob radiate *less* high end than closing it did.
    Kick808Device dark { "Dark Kick" };
    dark.setTone(0.0f);
    dark.processMidiNoteOn(36, 127);
    const auto darkHit = renderMono(dark, SampleRate / 4);

    Kick808Device bright { "Bright Kick" };
    bright.setTone(1.0f);
    bright.processMidiNoteOn(36, 127);
    const auto brightHit = renderMono(bright, SampleRate / 4);

    const double darkHigh = bandEnergy(darkHit, 800.0, 4000.0);
    const double brightHigh = bandEnergy(brightHit, 800.0, 4000.0);
    QVERIFY2(brightHigh > darkHigh * 4.0,
             QString("Full tone (%1) did not clearly brighten against zero tone (%2)").arg(brightHigh).arg(darkHigh).toUtf8().constData());

    // Below the click it is a tilt, not a level control: an RD-8 measures within a decibel or two
    // of itself at 100 Hz across the whole range of the knob.
    const double darkLow = bandEnergy(darkHit, 40.0, 100.0);
    const double brightLow = bandEnergy(brightHit, 40.0, 100.0);
    QVERIFY2(brightLow > darkLow * 0.5 && brightLow < darkLow * 2.0,
             QString("Tone moved the low end from %1 to %2").arg(darkLow).arg(brightLow).toUtf8().constData());
}

void Kick808Test::test_tone_zero_shouldStillClick()
{
    // The hardware clicks throughout its Tone range, and a kick with no click at all disappears on
    // anything smaller than a full-range speaker.
    Kick808Device kick { "Test Kick" };
    kick.setTone(0.0f);
    kick.processMidiNoteOn(36, 127);
    const auto hit = renderMono(kick, SampleRate / 4);

    // Against the fundamental, which sits an octave and a half below.
    const double fundamental = bandEnergy(hit, 55.0, 75.0);
    const double click = bandEnergy(hit, 300.0, 1000.0);
    QVERIFY2(click > fundamental * 1.0e-5,
             QString("Zero tone left no click: 300 - 1000 Hz at %1 against %2 at the fundamental").arg(click).arg(fundamental).toUtf8().constData());
}

void Kick808Test::test_velocity_shouldAffectOutputLevel()
{
    Kick808Device soft { "Soft Kick" };
    soft.processMidiNoteOn(36, 40);
    const double softPeak = peakLevel(renderMono(soft, SampleRate / 4));

    Kick808Device hard { "Hard Kick" };
    hard.processMidiNoteOn(36, 127);
    const double hardPeak = peakLevel(renderMono(hard, SampleRate / 4));

    QVERIFY2(hardPeak > softPeak * 2.0,
             QString("Hard hit (%1) not clearly louder than soft hit (%2)").arg(hardPeak).arg(softPeak).toUtf8().constData());
}

void Kick808Test::test_retrigger_shouldStayContinuous()
{
    Kick808Device kick { "Test Kick" };
    makeSteady(kick);
    kick.setKeyTrack(false);
    kick.processMidiNoteOn(36, 127);

    const auto tail = renderMono(kick, SampleRate / 5);
    const double lastSample = tail.back();
    const double tailPeak = peakLevel(tail);
    QVERIFY(tailPeak > 0.0);

    // A new hit restarts the resonator but hands the ring that was sounding to the fading tail, so
    // the waveform must not step at the retrigger.
    kick.processMidiNoteOn(36, 1);
    const auto afterRetrigger = renderMono(kick, 1);

    QVERIFY2(std::abs(afterRetrigger.front() - lastSample) < tailPeak * 0.05,
             QString("Retrigger broke waveform continuity: %1 -> %2 (tail peak %3)").arg(lastSample).arg(afterRetrigger.front()).arg(tailPeak).toUtf8().constData());
}

void Kick808Test::test_retrigger_shouldProduceConsistentHits()
{
    // A long decay makes consecutive hits overlap heavily, which is the case where a residual ring
    // left in the resonator would beat against the new pulse and make every hit land differently.
    Kick808Device kick { "Test Kick" };
    kick.setDecay(0.8f);
    kick.setPitchDepth(0.0f);
    kick.setKeyTrack(false);

    constexpr int hitCount = 6;
    constexpr uint32_t settleFrames = SampleRate * 30 / 1000;
    constexpr uint32_t measureFrames = SampleRate * 50 / 1000;
    constexpr uint32_t restFrames = SampleRate * 20 / 1000;

    std::vector<double> peaks;
    for (int hit = 0; hit < hitCount; hit++) {
        kick.processMidiNoteOn(36, 127);
        // Skip past the attack and the fading remains of the previous hit, then weigh the ring.
        renderMono(kick, settleFrames);
        peaks.push_back(peakLevel(renderMono(kick, measureFrames)));
        renderMono(kick, restFrames);
    }

    const auto [minPeak, maxPeak] = std::minmax_element(peaks.begin(), peaks.end());
    QVERIFY(*minPeak > 0.0);
    QVERIFY2(*maxPeak / *minPeak < 1.02,
             QString("Hits were inconsistent: min %1, max %2").arg(*minPeak).arg(*maxPeak).toUtf8().constData());
}

void Kick808Test::test_lpfCutoff_closed_shouldAttenuateOutput()
{
    Kick808Device open { "Open Kick" };
    open.processMidiNoteOn(36, 127);
    const double openPeak = peakLevel(renderMono(open, SampleRate / 4));

    Kick808Device closed { "Closed Kick" };
    closed.setLpfCutoff(0.0f);
    closed.processMidiNoteOn(36, 127);
    const double closedPeak = peakLevel(renderMono(closed, SampleRate / 4));

    QVERIFY2(closedPeak < openPeak * 0.5,
             QString("Closed LPF (%1) did not attenuate against open (%2)").arg(closedPeak).arg(openPeak).toUtf8().constData());
}

void Kick808Test::test_hpfCutoff_closed_shouldAttenuateOutput()
{
    Kick808Device open { "Open Kick" };
    open.processMidiNoteOn(36, 127);
    const double openPeak = peakLevel(renderMono(open, SampleRate / 4));

    // Sweeping the high pass all the way up should gut a voice this low.
    Kick808Device closed { "Closed Kick" };
    closed.setHpfCutoff(1.0f);
    closed.processMidiNoteOn(36, 127);
    const double closedPeak = peakLevel(renderMono(closed, SampleRate / 4));

    QVERIFY2(closedPeak < openPeak * 0.5,
             QString("Closed HPF (%1) did not attenuate against open (%2)").arg(closedPeak).arg(openPeak).toUtf8().constData());
}

void Kick808Test::test_midiCc_shouldNotChangeAuthoredValue()
{
    Kick808Device kick { "Test Kick" };
    kick.setDecay(0.25f);

    kick.processMidiCc(72, 127, 0); // Decay

    const auto parameter = kick.parameter(Constants::NahdXml::xmlKeyDecay().toStdString());
    QVERIFY(parameter.has_value());
    QCOMPARE(parameter->get().value(), 1.0f);
    QCOMPARE(parameter->get().authoredValue(), 0.25f);
}

void Kick808Test::test_resetAllControllers_shouldRestoreAuthoredValue()
{
    // The table-driven knobs had no restore path: only fader, pan and gain came back.
    Kick808Device kick { "Test Kick" };
    kick.setDecay(0.25f);
    kick.processMidiCc(72, 127, 0);

    kick.processMidiCc(121, 127, 0); // Reset All Controllers

    QVERIFY(std::abs(kick.decay() - 0.25f) < 0.01f);
}

void Kick808Test::test_midiCc_shouldReachEveryParameter()
{
    Kick808Device kick { "Test Kick" };

    // Fader and Pan plus every knob on the panel.
    QCOMPARE(kick.availableMidiCcControllers().size(), size_t { 12 });

    kick.processMidiCc(70, 0, 0); // Tuning
    kick.processMidiCc(71, 127, 0); // Tone
    kick.processMidiCc(72, 32, 0); // Decay
    kick.processMidiCc(73, 127, 0); // Pitch Depth
    kick.processMidiCc(75, 0, 0); // Pitch Decay
    kick.processMidiCc(5, 127, 0); // Glide
    kick.processMidiCc(80, 64, 0); // Drive
    kick.processMidiCc(74, 0, 0); // LPF
    kick.processMidiCc(81, 127, 0); // HPF

    QVERIFY(std::abs(kick.tune()) < 0.01f);
    QVERIFY(std::abs(kick.tone() - 1.0f) < 0.01f);
    QVERIFY(std::abs(kick.decay() - 32.0f / 127.0f) < 0.01f);
    QVERIFY(std::abs(kick.pitchDepth() - 1.0f) < 0.01f);
    QVERIFY(std::abs(kick.pitchDecay()) < 0.01f);
    QVERIFY(std::abs(kick.glide() - 1.0f) < 0.01f);
    QVERIFY(std::abs(kick.drive() - 64.0f / 127.0f) < 0.01f);
    QVERIFY(std::abs(kick.lpfCutoff()) < 0.01f);
    QVERIFY(std::abs(kick.hpfCutoff() - 1.0f) < 0.01f);

    // Key Track is a switch, so it follows the usual halfway threshold.
    kick.processMidiCc(76, 0, 0);
    QCOMPARE(kick.keyTrack(), false);
    kick.processMidiCc(76, 127, 0);
    QCOMPARE(kick.keyTrack(), true);
}

void Kick808Test::test_drive_full_shouldLeaveHeadroom()
{
    Kick808Device kick { "Driven Kick" };
    kick.setDrive(1.0f);
    kick.setDecay(1.0f);
    kick.setTone(1.0f);
    // Hard left, so one channel carries the whole signal rather than the pan law taking 3 dB off it.
    kick.setPan(0.0f);
    kick.processMidiNoteOn(36, 127);

    const double drivenPeak = peakLevel(renderMono(kick, SampleRate / 2));

    // The saturator normalises by tanh(gain), so it has to aim below full scale. Reaching 1.0 would
    // latch the device rack's clip indicator on every buffer, which no amount of clearing can undo,
    // and would leave the fader nothing to work with.
    QVERIFY2(drivenPeak > 0.0 && drivenPeak < 0.8,
             QString("Fully driven peak was %1, too close to full scale").arg(drivenPeak).toUtf8().constData());

    // Drive is meant to change the harmonics, not to double as a big makeup gain.
    Kick808Device clean { "Clean Kick" };
    clean.setDecay(1.0f);
    clean.setTone(1.0f);
    clean.setPan(0.0f);
    clean.processMidiNoteOn(36, 127);
    const double cleanPeak = peakLevel(renderMono(clean, SampleRate / 2));

    QVERIFY2(drivenPeak < cleanPeak * 3.0,
             QString("Drive raised the peak from %1 to %2, which is a level control rather than a tone control").arg(cleanPeak).arg(drivenPeak).toUtf8().constData());
}

void Kick808Test::test_serialization_shouldRestoreParameters()
{
    Kick808Device kick { "Test Kick" };
    kick.setTune(0.8f);
    kick.setTone(0.2f);
    kick.setDecay(0.9f);
    kick.setPitchDepth(0.6f);
    kick.setPitchDecay(0.15f);
    kick.setDrive(0.4f);
    kick.setGlide(0.3f);
    kick.setKeyTrack(false);
    kick.setLpfCutoff(0.7f);
    kick.setHpfCutoff(0.25f);

    QString xml;
    NahdXmlWriter writer { xml };
    kick.serializeToXml(writer);

    Kick808Device restored { "Restored Kick" };
    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        restored.deserializeFromXml(reader);
    }

    QCOMPARE(restored.tune(), 0.8f);
    QCOMPARE(restored.tone(), 0.2f);
    QCOMPARE(restored.decay(), 0.9f);
    QCOMPARE(restored.pitchDepth(), 0.6f);
    QCOMPARE(restored.pitchDecay(), 0.15f);
    QCOMPARE(restored.drive(), 0.4f);
    QCOMPARE(restored.glide(), 0.3f);
    QCOMPARE(restored.keyTrack(), false);
    QCOMPARE(restored.lpfCutoff(), 0.7f);
    QCOMPARE(restored.hpfCutoff(), 0.25f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::Kick808Test)
