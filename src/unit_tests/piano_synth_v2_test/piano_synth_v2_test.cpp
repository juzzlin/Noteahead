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

#include "piano_synth_v2_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/piano_synth_v2_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t DefaultSampleRate = 48000;

AudioContext makeContext(std::vector<double> & buffer, uint32_t frameCount, uint32_t sampleRate)
{
    return AudioContext { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
}

double peakLevel(const std::vector<double> & buffer)
{
    double peak = 0.0;
    for (const double s : buffer) {
        peak = std::max(peak, std::abs(s));
    }
    return peak;
}

double rmsLevel(const std::vector<double> & buffer)
{
    double sum = 0.0;
    for (const double s : buffer) {
        sum += s * s;
    }
    return std::sqrt(sum / static_cast<double>(buffer.size()));
}

double toDb(double linear)
{
    return 20.0 * std::log10(std::max(linear, 1e-12));
}

double noteFrequency(int note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

double centsBetween(double measured, double reference)
{
    return 1200.0 * std::log2(measured / reference);
}

void renderFrames(PianoSynthV2Device & piano, uint32_t frames, uint32_t sampleRate = DefaultSampleRate)
{
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    auto ctx = makeContext(buffer, frames, sampleRate);
    piano.processAudio(ctx);
}

// Renders the device down to mono, optionally discarding the strike so that only the
// part of the tone being measured is left.
std::vector<double> renderTone(PianoSynthV2Device & piano, uint32_t sampleRate, uint32_t skip, uint32_t length)
{
    const uint32_t frames = skip + length;
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    auto ctx = makeContext(buffer, frames, sampleRate);
    piano.processAudio(ctx);

    std::vector<double> mono(length);
    for (uint32_t i = 0; i < length; i++) {
        const size_t frame = static_cast<size_t>(skip + i) * 2;
        mono[i] = buffer[frame] + buffer[frame + 1];
    }
    return mono;
}

// Magnitude of the Hann-windowed DFT at an arbitrary frequency, evaluated by rotating a
// phasor so that the frequency grid can be made as fine as needed.
double dftMagnitude(std::span<const double> samples, double sampleRate, double frequency)
{
    const double w = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double cosW = std::cos(w);
    const double sinW = std::sin(w);
    const double last = static_cast<double>(samples.size() - 1);
    double phasorRe = 1.0;
    double phasorIm = 0.0;
    double re = 0.0;
    double im = 0.0;
    for (size_t i = 0; i < samples.size(); i++) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i) / last);
        const double value = samples[i] * window;
        re += value * phasorRe;
        im -= value * phasorIm;
        const double nextRe = phasorRe * cosW - phasorIm * sinW;
        phasorIm = phasorRe * sinW + phasorIm * cosW;
        phasorRe = nextRe;
    }
    return std::hypot(re, im);
}

// Locates a partial on a fine grid spanning a band around where it is expected and
// refines the winning bin parabolically. Nothing is returned if the peak sits at the
// edge of the span, since the real partial is then somewhere further out.
std::optional<double> measurePeakFrequency(std::span<const double> samples, double sampleRate, double expected, double span)
{
    constexpr int steps = 60;
    const double step = expected * span / steps;

    std::vector<double> magnitudes(2 * steps + 1);
    int best = 0;
    for (int i = 0; i <= 2 * steps; i++) {
        magnitudes[i] = dftMagnitude(samples, sampleRate, expected + (i - steps) * step);
        if (magnitudes[i] > magnitudes[best]) {
            best = i;
        }
    }
    if (best == 0 || best == 2 * steps) {
        return std::nullopt;
    }

    const double denominator = magnitudes[best - 1] - 2.0 * magnitudes[best] + magnitudes[best + 1];
    const double offset = denominator == 0.0 ? 0.0 : 0.5 * (magnitudes[best - 1] - magnitudes[best + 1]) / denominator;
    return expected + (best - steps + offset) * step;
}

// One note struck on a clean voice, rendered down to mono. Everything the keyboard
// survey measures is derived from this.
std::vector<double> renderNote(int note, uint8_t velocity, double seconds, uint32_t sampleRate = DefaultSampleRate,
                               float detune = 0.0f, float stretch = 0.0f)
{
    PianoSynthV2Device piano { "Test Piano" };
    piano.setVolume(1.0f);
    piano.setGain(0.5f);
    piano.setStringDetune(detune); // Beating between the pair would wander the envelope
    piano.setStretch(stretch);
    piano.processMidiNoteOn(static_cast<uint8_t>(note), velocity);

    const auto frames = static_cast<uint32_t>(seconds * sampleRate);
    return renderTone(piano, sampleRate, 0, frames);
}

std::vector<double> renderNoteWithBrightness(int note, uint8_t velocity, double seconds, float brightness)
{
    PianoSynthV2Device piano { "Test Piano" };
    piano.setVolume(1.0f);
    piano.setGain(0.5f);
    piano.setStringDetune(0.0f);
    piano.setBrightness(brightness);
    piano.processMidiNoteOn(static_cast<uint8_t>(note), velocity);

    const auto frames = static_cast<uint32_t>(seconds * DefaultSampleRate);
    return renderTone(piano, DefaultSampleRate, 0, frames);
}

// Amplitude of one partial over time, taken from successive half-overlapping windows.
std::vector<double> partialEnvelope(const std::vector<double> & mono, double sampleRate, double frequency, double windowSeconds)
{
    const auto window = static_cast<size_t>(sampleRate * windowSeconds);
    const size_t hop = window / 2;
    std::vector<double> envelope;
    for (size_t start = 0; start + window <= mono.size(); start += hop) {
        envelope.push_back(dftMagnitude(std::span { mono }.subspan(start, window), sampleRate, frequency));
    }
    return envelope;
}

// Decay of one partial in dB per second, fitted over a stretch of the note starting a
// given time after the strike. Which stretch is measured matters here: a piano note
// decays in two stages, and the fast one sits on top of the slow one at the start.
std::optional<double> measureSlope(const std::vector<double> & mono, double sampleRate, double frequency,
                                   double fromSeconds, double toSeconds)
{
    constexpr double windowSeconds = 0.05;
    const double hopSeconds = windowSeconds * 0.5;

    const auto envelope = partialEnvelope(mono, sampleRate, frequency, windowSeconds);
    const auto first = static_cast<size_t>(fromSeconds / hopSeconds);
    const auto last = std::min(envelope.size(), static_cast<size_t>(toSeconds / hopSeconds));
    if (first + 4 > last) {
        return std::nullopt;
    }

    double sumT = 0.0, sumY = 0.0, sumTT = 0.0, sumTY = 0.0;
    size_t count = 0;
    for (size_t i = first; i < last; i++) {
        if (envelope[i] <= 0.0) {
            continue;
        }
        const double t = static_cast<double>(i) * hopSeconds;
        const double y = toDb(envelope[i]);
        sumT += t;
        sumY += y;
        sumTT += t * t;
        sumTY += t * y;
        count++;
    }
    if (count < 4) {
        return std::nullopt;
    }

    const double n = static_cast<double>(count);
    const double denominator = n * sumTT - sumT * sumT;
    if (denominator == 0.0) {
        return std::nullopt;
    }
    return (n * sumTY - sumT * sumY) / denominator;
}

// T60 read off the slow stage of the decay, which is what a piano's sustain is heard as
// and what the reference recording was measured for.
std::optional<double> measureT60(const std::vector<double> & mono, double sampleRate, double frequency,
                                 double fromSeconds, double toSeconds)
{
    const auto slope = measureSlope(mono, sampleRate, frequency, fromSeconds, toSeconds);
    if (!slope || *slope >= -1e-6) {
        return std::nullopt;
    }
    return -60.0 / *slope;
}

// Yamaha CP80. Levels and stiffness come from a chromatic sweep of the whole keyboard at
// velocity 64 and then 100, taken a semitone at a time; the decay times come from an
// earlier sweep of the octaves, whose notes are held long enough for the slow stage to be
// fitted at all. Levels are relative to the note at MIDI 60 rather than absolute, since
// only the shape across the keyboard is being matched, and each is the median of the note
// and its neighbours: the instrument is sampled in groups of three or four semitones, and
// within a group the level and the stiffness both step.
struct ReferenceNote
{
    int note;
    double relativeLevelDb;
    double t60;
    double inharmonicity;
};

constexpr std::array<ReferenceNote, 8> reference { {
  { 24, 0.3, 30.0, 4.25e-3 },
  { 36, 0.8, 21.9, 3.78e-4 },
  { 48, 2.2, 25.0, 2.41e-4 },
  { 60, 0.1, 13.6, 3.57e-4 },
  { 72, -2.1, 8.6, 7.90e-4 },
  { 84, -0.5, 3.6, 0.0 },
  { 96, -0.4, 2.3, 0.0 },
  { 108, -3.2, 1.15, 0.0 },
} };

// The window the slow stage of the decay is fitted over. High notes are gone before the
// window the bass is measured in has opened.
std::pair<double, double> decayWindow(int note)
{
    if (note >= 96) {
        return { 0.25, 0.9 };
    }
    if (note >= 84) {
        return { 0.4, 1.4 };
    }
    return { 1.0, 2.4 };
}

struct NoteMeasurement
{
    double peakDb {};
    std::optional<double> t60;
    std::optional<double> fittedB;
};

// Stiffness coefficient B fitted to the note's own partials, the same way the reference
// recording was measured: each partial is located on a fine grid and the fit is a least
// squares one on (f_k / k)^2 against k^2.
std::optional<double> fitInharmonicity(const std::vector<double> & mono, double sampleRate, double f0, int partials)
{
    double sumX = 0.0, sumY = 0.0, sumXX = 0.0, sumXY = 0.0;
    int count = 0;
    // Refined as the partials come in. By the twelfth partial stiffness has moved the
    // partial further than the gap to its neighbour, so searching where a harmonic series
    // would put it finds the wrong peak — or the skirt of the right one, which fits a
    // negative coefficient. Each partial is looked for where the fit so far says it is.
    double estimate = 0.0;
    for (int k = 1; k <= partials; k++) {
        const double kk = static_cast<double>(k);
        const double predicted = f0 * kk * std::sqrt(1.0 + estimate * kk * kk);
        if (predicted > sampleRate * 0.4) {
            break;
        }
        const auto measured = measurePeakFrequency(mono, sampleRate, predicted, 0.008 + 0.002 * kk);
        if (!measured) {
            break;
        }

        const double x = kk * kk;
        const double y = (*measured / kk) * (*measured / kk);
        sumX += x;
        sumY += y;
        sumXX += x * x;
        sumXY += x * y;
        count++;

        if (count >= 3) {
            const double n = static_cast<double>(count);
            const double denominator = n * sumXX - sumX * sumX;
            if (denominator != 0.0) {
                const double slope = (n * sumXY - sumX * sumY) / denominator;
                const double intercept = (sumY - slope * sumX) / n;
                if (intercept > 0.0) {
                    estimate = std::max(slope / intercept, 0.0);
                }
            }
        }
    }

    return count >= 5 ? std::optional { estimate } : std::nullopt;
}

NoteMeasurement measureNote(int note, uint8_t velocity = 100, uint32_t sampleRate = DefaultSampleRate)
{
    const auto [from, to] = decayWindow(note);
    const auto mono = renderNote(note, velocity, to + 0.2, sampleRate);

    NoteMeasurement measurement;
    measurement.peakDb = toDb(peakLevel(mono));
    measurement.t60 = measureT60(mono, sampleRate, noteFrequency(note), from, to);

    const auto window = std::min<size_t>(mono.size(), static_cast<size_t>(sampleRate * 0.4));
    const std::vector<double> head { mono.begin(), mono.begin() + static_cast<long>(window) };
    measurement.fittedB = fitInharmonicity(head, sampleRate, noteFrequency(note), 12);

    return measurement;
}

// Level of one partial just after the strike.
double partialLevelDb(const std::vector<double> & mono, double sampleRate, double frequency)
{
    const auto skip = static_cast<size_t>(sampleRate * 0.03);
    const auto window = static_cast<size_t>(sampleRate * 0.3);
    if (skip + window > mono.size()) {
        return toDb(0.0);
    }
    return toDb(dftMagnitude(std::span { mono }.subspan(skip, window), sampleRate, frequency));
}

// Where the energy of the strike sits, which is what velocity acts on.
double spectralCentroid(const std::vector<double> & mono, double sampleRate, double f0, int partials)
{
    double weighted = 0.0;
    double total = 0.0;
    for (int k = 1; k <= partials; k++) {
        const double frequency = f0 * k;
        if (frequency > sampleRate * 0.4) {
            break;
        }
        const double magnitude = std::pow(10.0, partialLevelDb(mono, sampleRate, frequency) / 20.0);
        weighted += frequency * magnitude * magnitude;
        total += magnitude * magnitude;
    }
    return total > 0.0 ? weighted / total : 0.0;
}

std::optional<double> measurePitch(int note, uint8_t velocity = 100, float stretch = 0.0f)
{
    const double expected = noteFrequency(note);
    const auto mono = renderNote(note, velocity, 0.7, DefaultSampleRate, 0.0f, stretch);
    const auto skip = static_cast<size_t>(DefaultSampleRate * 0.05);
    const auto length = std::min(mono.size() - skip, static_cast<size_t>(DefaultSampleRate * 0.5));
    return measurePeakFrequency(std::span { mono }.subspan(skip, length), DefaultSampleRate, expected, 0.03);
}

} // namespace

void PianoSynthV2Test::test_midiNoteOn_shouldActivateAudio()
{
    PianoSynthV2Device piano { "Test Piano" };
    piano.processMidiNoteOn(60, 100);
    QVERIFY(piano.hasActiveAudio());
}

void PianoSynthV2Test::test_midiNoteOff_shouldDecayToSilence()
{
    PianoSynthV2Device piano { "Test Piano" };
    piano.setReleaseTime(0.0f); // Shortest possible damper
    piano.processMidiNoteOn(60, 100);

    renderFrames(piano, 512);
    QVERIFY(piano.hasActiveAudio());

    piano.processMidiNoteOff(60);

    renderFrames(piano, DefaultSampleRate);
    QVERIFY(!piano.hasActiveAudio());
}

void PianoSynthV2Test::test_polyphony_shouldSupportMultipleSimultaneousNotes()
{
    PianoSynthV2Device piano { "Test Piano" };
    piano.processMidiNoteOn(60, 100);
    piano.processMidiNoteOn(64, 100);
    piano.processMidiNoteOn(67, 100);

    const uint32_t frameCount = 8192;
    const double rmsThree = rmsLevel(renderTone(piano, DefaultSampleRate, 0, frameCount));

    PianoSynthV2Device pianoOne { "Test Piano One" };
    pianoOne.processMidiNoteOn(60, 100);
    const double rmsOne = rmsLevel(renderTone(pianoOne, DefaultSampleRate, 0, frameCount));

    QVERIFY2(rmsThree > rmsOne,
             QString { "Three-note level (%1) not greater than one-note level (%2)" }.arg(rmsThree).arg(rmsOne).toUtf8().constData());
}

void PianoSynthV2Test::test_sustainPedal_shouldKeepNoteActiveAfterNoteOff()
{
    PianoSynthV2Device piano { "Test Piano" };

    piano.processMidiCc(64, 127, 0);
    piano.processMidiNoteOn(60, 100);
    renderFrames(piano, 256);

    piano.processMidiNoteOff(60);

    QVERIFY(piano.hasActiveAudio());
}

void PianoSynthV2Test::test_sustainPedal_shouldReleaseNoteWhenPedalLifted()
{
    PianoSynthV2Device piano { "Test Piano" };
    piano.setReleaseTime(0.0f);

    piano.processMidiCc(64, 127, 0);
    piano.processMidiNoteOn(60, 100);
    renderFrames(piano, 256);
    piano.processMidiNoteOff(60);

    piano.processMidiCc(64, 0, 0);

    renderFrames(piano, DefaultSampleRate);
    QVERIFY(!piano.hasActiveAudio());
}

void PianoSynthV2Test::test_allNotesOff_shouldSilenceAllVoices()
{
    PianoSynthV2Device piano { "Test Piano" };
    piano.setReleaseTime(0.0f);

    piano.processMidiNoteOn(48, 100);
    piano.processMidiNoteOn(60, 100);
    piano.processMidiNoteOn(72, 100);
    renderFrames(piano, 256);

    piano.processMidiAllNotesOff();
    renderFrames(piano, DefaultSampleRate);

    QVERIFY(!piano.hasActiveAudio());
}

void PianoSynthV2Test::test_serialization_shouldRestoreParameters()
{
    PianoSynthV2Device piano { "Test Piano" };
    piano.setBrightness(0.8f);
    piano.setDecay(0.3f);
    piano.setInharmonicity(0.1f);
    piano.setStretch(0.7f);
    piano.setRichness(0.9f);
    piano.setDoubleDecay(0.25f);

    QString xml;
    NahdXmlWriter writer { xml };
    piano.serializeToXml(writer);

    PianoSynthV2Device piano2 { "Restored Piano" };
    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        piano2.deserializeFromXml(reader);
    }

    QCOMPARE(piano2.brightness(), piano.brightness());
    QCOMPARE(piano2.decay(), piano.decay());
    QCOMPARE(piano2.inharmonicity(), piano.inharmonicity());
    QCOMPARE(piano2.stretch(), piano.stretch());
    QCOMPARE(piano2.richness(), piano.richness());
    QCOMPARE(piano2.doubleDecay(), piano.doubleDecay());
}

void PianoSynthV2Test::test_reset_shouldRestoreFactoryDefaults()
{
    // What the Reset button in the dialog is wired to. Every control has to come back,
    // including the ones on the Device base class.
    PianoSynthV2Device piano { "Test Piano" };

    const float brightness = piano.brightness();
    const float decay = piano.decay();
    const float richness = piano.richness();
    const float stretch = piano.stretch();
    const float stereoWidth = piano.stereoWidth();

    piano.setBrightness(0.93f);
    piano.setDecay(0.07f);
    piano.setRichness(0.11f);
    piano.setStretch(0.88f);
    piano.setStereoWidth(0.95f);
    piano.setGain(0.31f);
    QVERIFY(std::abs(piano.brightness() - brightness) > 0.1f);

    piano.reset();

    QVERIFY(std::abs(piano.brightness() - brightness) < 0.001f);
    QVERIFY(std::abs(piano.decay() - decay) < 0.001f);
    QVERIFY(std::abs(piano.richness() - richness) < 0.001f);
    QVERIFY(std::abs(piano.stretch() - stretch) < 0.001f);
    QVERIFY(std::abs(piano.stereoWidth() - stereoWidth) < 0.001f);
}

void PianoSynthV2Test::test_velocity_shouldFollowSquareLaw()
{
    // The reference gains seven to eight decibels between velocity 64 and 100, which is
    // amplitude going as the square of velocity rather than in step with it.
    const double soft = toDb(peakLevel(renderNote(60, 64, 0.3)));
    const double hard = toDb(peakLevel(renderNote(60, 100, 0.3)));
    const double gained = hard - soft;

    QVERIFY2(gained > 5.5 && gained < 10.0,
             QString { "Velocity 64 to 100 gained %1 dB, expected seven to eight" }.arg(gained).toUtf8().constData());
}

void PianoSynthV2Test::test_velocity_shouldBrightenTheStrike()
{
    // A harder strike shortens the hammer's contact and pushes its spectrum up. The
    // reference's centroid climbs by about half again from velocity 64 to 100.
    const double f0 = noteFrequency(48);
    const double soft = spectralCentroid(renderNote(48, 64, 0.4), DefaultSampleRate, f0, 24);
    const double hard = spectralCentroid(renderNote(48, 100, 0.4), DefaultSampleRate, f0, 24);

    QVERIFY2(hard > soft * 1.15,
             QString { "Centroid moved from %1 Hz to %2 Hz, expected a clear rise" }.arg(soft).arg(hard).toUtf8().constData());
}

void PianoSynthV2Test::test_brightness_shouldTiltTheStrike()
{
    // Brightness has to be audible on the note as it is struck, not only on what is left
    // ringing afterwards. It used to reach the decay times alone, which left the strike
    // spectrum identical from one end of the control to the other.
    const double f0 = noteFrequency(48);
    const double dark = spectralCentroid(renderNoteWithBrightness(48, 100, 0.4, 0.0f), DefaultSampleRate, f0, 24);
    const double bright = spectralCentroid(renderNoteWithBrightness(48, 100, 0.4, 1.0f), DefaultSampleRate, f0, 24);

    QVERIFY2(bright > dark * 1.25,
             QString { "Centroid moved only from %1 Hz to %2 Hz across the whole control" }.arg(dark).arg(bright).toUtf8().constData());
}

void PianoSynthV2Test::test_brightness_shouldSetHowLongPartialsHold()
{
    // And it still decides how long the upper partials hold on, which is the other half of
    // what makes a piano sound bright or dull.
    const double f0 = noteFrequency(60);
    const auto dark = measureSlope(renderNoteWithBrightness(60, 100, 2.5, 0.0f), DefaultSampleRate, f0 * 8.0, 0.3, 2.2);
    const auto bright = measureSlope(renderNoteWithBrightness(60, 100, 2.5, 1.0f), DefaultSampleRate, f0 * 8.0, 0.3, 2.2);
    QVERIFY(dark.has_value() && bright.has_value());

    QVERIFY2(*dark < *bright * 2.0,
             QString { "Eighth partial fell at %1 dB/s dark against %2 dB/s bright" }.arg(*dark).arg(*bright).toUtf8().constData());
}

void PianoSynthV2Test::test_tuning_shouldMatchNoteFrequency_acrossKeyboard()
{
    for (const int note : { 24, 36, 48, 60, 72, 84, 96, 108 }) {
        const auto measured = measurePitch(note);
        QVERIFY2(measured.has_value(), QString { "No pitch found for note %1" }.arg(note).toUtf8().constData());
        const double cents = centsBetween(*measured, noteFrequency(note));
        QVERIFY2(std::abs(cents) < 5.0,
                 QString { "Note %1 sounded %2 cents off" }.arg(note).arg(cents).toUtf8().constData());
    }
}

void PianoSynthV2Test::test_tuning_shouldNotDependOnVelocity()
{
    for (const int note : { 36, 60, 84 }) {
        const auto soft = measurePitch(note, 40);
        const auto hard = measurePitch(note, 120);
        QVERIFY(soft.has_value() && hard.has_value());
        const double cents = centsBetween(*hard, *soft);
        QVERIFY2(std::abs(cents) < 2.0,
                 QString { "Note %1 moved %2 cents between soft and hard" }.arg(note).arg(cents).toUtf8().constData());
    }
}

void PianoSynthV2Test::test_stretch_shouldSharpenTheTreble_whenEnabled()
{
    // Off by default, so that the piano stays in tune with the rest of Noteahead. Turned
    // up it follows the reference's Railsback curve, which runs the top key sharp by a
    // third of a semitone and the bass flat.
    const auto trebleFlat = measurePitch(108, 100, 0.0f);
    const auto trebleStretched = measurePitch(108, 100, 1.0f);
    QVERIFY(trebleFlat.has_value() && trebleStretched.has_value());
    const double treble = centsBetween(*trebleStretched, *trebleFlat);
    QVERIFY2(treble > 25.0,
             QString { "Top key stretched by only %1 cents" }.arg(treble).toUtf8().constData());

    const auto bassFlat = measurePitch(36, 100, 0.0f);
    const auto bassStretched = measurePitch(36, 100, 1.0f);
    QVERIFY(bassFlat.has_value() && bassStretched.has_value());
    const double bass = centsBetween(*bassStretched, *bassFlat);
    QVERIFY2(bass < -3.0,
             QString { "Bass stretched by %1 cents, expected it to go flat" }.arg(bass).toUtf8().constData());
}

void PianoSynthV2Test::test_partials_shouldRunSharp_acrossKeyboard()
{
    // Stiffness is what makes a piano a piano: every partial sits sharp of where a
    // harmonic series would put it, and progressively so.
    for (const auto & entry : reference) {
        if (entry.inharmonicity <= 0.0) {
            continue; // Too few partials below Nyquist up there to fit anything to
        }
        const auto measurement = measureNote(entry.note);
        QVERIFY2(measurement.fittedB.has_value(),
                 QString { "No stiffness fit for note %1" }.arg(entry.note).toUtf8().constData());
        const double ratio = *measurement.fittedB / entry.inharmonicity;
        QVERIFY2(ratio > 0.4 && ratio < 2.5,
                 QString { "Note %1 fitted B %2, reference %3" }
                   .arg(entry.note)
                   .arg(*measurement.fittedB)
                   .arg(entry.inharmonicity)
                   .toUtf8()
                   .constData());
    }
}

void PianoSynthV2Test::test_partials_shouldDecayFasterThanTheFundamental()
{
    // On the reference the eighth partial of a bass note is gone in well under half the
    // time its fundamental takes. One loop filter cannot do that; a decay time per mode can.
    const double f0 = noteFrequency(36);
    const auto mono = renderNote(36, 100, 2.6);

    const auto fundamental = measureSlope(mono, DefaultSampleRate, f0, 1.0, 2.4);
    const auto eighth = measureSlope(mono, DefaultSampleRate, f0 * 8.0 * std::sqrt(1.0 + 3.78e-4 * 64.0), 1.0, 2.4);
    QVERIFY(fundamental.has_value() && eighth.has_value());

    QVERIFY2(*eighth < *fundamental * 1.8,
             QString { "Eighth partial fell at %1 dB/s against the fundamental's %2" }.arg(*eighth).arg(*fundamental).toUtf8().constData());
}

void PianoSynthV2Test::test_radiation_shouldLeaveBassFundamentalBelowSecondPartial()
{
    // At the bottom of the keyboard the reference radiates the second and third partials
    // some nine and twelve decibels louder than the fundamental, because a soundboard is
    // a poor radiator at thirty hertz. Without this the bass sounds synthetic.
    const double f0 = noteFrequency(24);
    const auto mono = renderNote(24, 100, 0.6);

    const double first = partialLevelDb(mono, DefaultSampleRate, f0);
    const double second = partialLevelDb(mono, DefaultSampleRate, f0 * 2.0);
    const double third = partialLevelDb(mono, DefaultSampleRate, f0 * 3.0);

    QVERIFY2(second > first + 3.0,
             QString { "Second partial is %1 dB over the fundamental, expected several" }.arg(second - first).toUtf8().constData());
    QVERIFY2(third > first + 3.0,
             QString { "Third partial is %1 dB over the fundamental, expected several" }.arg(third - first).toUtf8().constData());
}

void PianoSynthV2Test::test_decay_shouldRunInTwoStages()
{
    // Struck together, the strings of a unison lean on the bridge as one and lose their
    // energy quickly; what is left is the out-of-phase motion, and it rings on. The
    // reference shows the knee clearly, and it is most of what a piano's sustain sounds
    // like. The prompt stage is brief — the bridge has drained it inside a fifth of a
    // second at MIDI 84 — so the two windows have to sit either side of that, not
    // straddle it.
    const double f0 = noteFrequency(84);
    const auto mono = renderNote(84, 100, 2.4);

    const auto early = measureSlope(mono, DefaultSampleRate, f0, 0.02, 0.18);
    const auto late = measureSlope(mono, DefaultSampleRate, f0, 1.0, 2.2);
    QVERIFY(early.has_value() && late.has_value());

    QVERIFY2(*early < *late * 1.3,
             QString { "Early decay %1 dB/s is not clearly faster than the late %2 dB/s" }.arg(*early).arg(*late).toUtf8().constData());
}

void PianoSynthV2Test::test_undampedNotes_shouldKeepRinging_afterNoteOff()
{
    // A grand has no dampers over the top octave and a half, so letting the key up there
    // changes nothing.
    PianoSynthV2Device piano { "Test Piano" };
    piano.setReleaseTime(0.0f);
    piano.processMidiNoteOn(100, 100);
    renderFrames(piano, 512);

    piano.processMidiNoteOff(100);
    renderFrames(piano, DefaultSampleRate / 4);

    QVERIFY(piano.hasActiveAudio());
}

void PianoSynthV2Test::test_keyboard_shouldSpeak_atEveryRegister()
{
    for (const int note : { 21, 36, 48, 60, 72, 84, 96, 108 }) {
        const double peak = peakLevel(renderNote(note, 100, 0.25));
        QVERIFY2(peak > 1e-3, QString { "Note %1 barely spoke, peak %2" }.arg(note).arg(peak).toUtf8().constData());
    }
}

void PianoSynthV2Test::test_keyboard_shouldHoldLevel_acrossRegisters()
{
    // The reference stays within a few decibels of itself from the bass to the top
    // octave, only falling away on the very last key. High notes must not go quiet.
    const double anchor = measureNote(60).peakDb;
    for (const auto & entry : reference) {
        const double relative = measureNote(entry.note).peakDb - anchor;
        QVERIFY2(std::abs(relative - entry.relativeLevelDb) < 8.0,
                 QString { "Note %1 came out %2 dB against the reference's %3 dB" }
                   .arg(entry.note)
                   .arg(relative)
                   .arg(entry.relativeLevelDb)
                   .toUtf8()
                   .constData());
    }
}

void PianoSynthV2Test::test_keyboard_shouldFollowReferenceDecay_acrossRegisters()
{
    for (const auto & entry : reference) {
        const auto measurement = measureNote(entry.note);
        QVERIFY2(measurement.t60.has_value(),
                 QString { "No decay measured for note %1" }.arg(entry.note).toUtf8().constData());
        const double ratio = *measurement.t60 / entry.t60;
        QVERIFY2(ratio > 0.5 && ratio < 2.0,
                 QString { "Note %1 decayed over %2 s against the reference's %3 s" }
                   .arg(entry.note)
                   .arg(*measurement.t60)
                   .arg(entry.t60)
                   .toUtf8()
                   .constData());
    }
}

void PianoSynthV2Test::test_keyboard_shouldReportMeasurements_againstReference()
{
    // Reports rather than asserts: the individual properties are held to the reference by
    // the tests above, and having the whole survey in one table makes it possible to see
    // at a glance which register a change has moved.
    qInfo("note | level dB (ref) | T60 s (ref)     | B (ref)");

    const double anchor = measureNote(60).peakDb;
    for (const auto & entry : reference) {
        const auto measurement = measureNote(entry.note);
        const QString t60 = measurement.t60 ? QString::number(measurement.t60.value(), 'f', 2) : QString { "none" };
        const QString fitted = measurement.fittedB ? QString::number(measurement.fittedB.value(), 'e', 2) : QString { "none" };
        qInfo("%4d | %6.1f (%5.1f) | %7s (%5.2f) | %8s (%.2e)",
              entry.note, measurement.peakDb - anchor, entry.relativeLevelDb,
              t60.toUtf8().constData(), entry.t60,
              fitted.toUtf8().constData(), entry.inharmonicity);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PianoSynthV2Test)
