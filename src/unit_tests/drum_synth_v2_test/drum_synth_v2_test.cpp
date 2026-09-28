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
#include "../../application/service/drum_voice_preview.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"
#include "../../domain/dsp/fft.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>

#include <algorithm>
#include <span>

#include <cmath>

namespace noteahead {

namespace {

constexpr uint32_t frameCount = 4096;
constexpr uint32_t sampleRate = 44100;

//! Triggers one note on a freshly built device and hands back the interleaved output. Templated over
//! the device so that V1 and V2 are driven through exactly the same steps, which is the whole point
//! of the comparison below.
//! Welch-averaged power spectrum of the left channel over the first @p seconds.
//!
//! The averaging is not an optimisation. A single frame's flatness collapses for noise -- its bin
//! magnitudes are Rayleigh distributed, so the geometric mean falls far under the arithmetic one
//! and white noise scores about 0.6 rather than 1.0. Measured that way these cymbals looked like
//! they were matching the hardware while they were nothing like it.
std::vector<double> spectrum(const std::vector<double> & frames, double seconds)
{
    constexpr int size = 2048;
    std::vector<double> acc(size / 2, 0.0);
    const auto limit = std::min(frames.size() / 2, static_cast<size_t>(seconds * sampleRate));
    int windows = 0;
    for (size_t start = 0; start + size <= limit; start += size / 2) {
        std::vector<double> re(size), im(size, 0.0);
        for (int i = 0; i < size; i++) {
            re[static_cast<size_t>(i)] = frames.at((start + static_cast<size_t>(i)) * 2) * (0.5 - 0.5 * std::cos(2.0 * M_PI * i / (size - 1)));
        }
        Fft::forward(re.data(), im.data(), size);
        for (size_t k = 0; k < acc.size(); k++) {
            acc[k] += re[k] * re[k] + im[k] * im[k];
        }
        windows++;
    }
    for (auto && value : acc) {
        value /= std::max(1, windows);
    }
    return acc;
}

size_t binOf(double hz)
{
    return static_cast<size_t>(hz * 2048 / sampleRate);
}

double bandPower(const std::vector<double> & ps, double lowHz, double highHz)
{
    double sum = 0.0;
    for (auto k = binOf(lowHz); k < std::min(ps.size(), binOf(highHz)); k++) {
        sum += ps.at(k);
    }
    return sum;
}

//! How noisy a band is: 0 for a pure tone, and for noise as much as the band's own tilt allows.
//!
//! Only ever asked about one octave at a time. Over a wider band this mostly reports the spectral
//! slope rather than the noisiness, because a sound that falls steeply across the band is far from
//! flat however noisy it is.
double bandFlatness(const std::vector<double> & ps, double lowHz, double highHz)
{
    double logSum = 0.0;
    double sum = 0.0;
    size_t count = 0;
    for (auto k = binOf(lowHz); k < std::min(ps.size(), binOf(highHz)); k++) {
        const double power = std::max(ps.at(k), 1.0e-20);
        logSum += std::log(power);
        sum += power;
        count++;
    }
    return count ? std::exp(logSum / count) / (sum / count) : 0.0;
}

//! Whether a voice is one V2 plays with an engine of its own.
//!
//! The cymbals are fitted to a recording of the hardware: V1's crash carried four tenths of steady
//! noise against four of metal, which measures as a wash rather than as struck metal, and neither
//! cymbal had any body below 600 Hz. V1 keeps what it always played, so these three are the voices
//! the two devices no longer share.
bool isFittedCymbal(int voice)
{
    using enum DrumSynthV2::VoiceIndex;
    const auto index = static_cast<DrumSynthV2::VoiceIndex>(voice);
    return index == Crash || index == Ride || index == ReverseCrash;
}

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

void DrumSynthV2Test::test_voiceSend_shouldReachTheBusOnItsOwn()
{
    // A snare in the plate while the rest of the kit stays dry: the case a per-device send cannot
    // express, and the reason for all of this.
    DrumSynthV2Device device { "Drum Synth V2" };
    device.setSendSourceLevel(0, 0, 1.0f);
    device.processMidiNoteOn(36, 100);

    std::vector<std::vector<double>> buses(2, std::vector<double>(4096 * 2, 0.0));
    std::vector<std::span<double>> busSpans;
    for (auto & bus : buses) {
        busSpans.emplace_back(bus.data(), bus.size());
    }
    std::vector<double> buffer(4096 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 4096, 44100 };
    context.sendBuses = std::span<const std::span<double>>(busSpans);
    device.processAudio(context);

    const auto peak = [](const std::vector<double> & samples) {
        double result = 0.0;
        for (const double sample : samples) {
            result = std::max(result, std::abs(sample));
        }
        return result;
    };

    QCOMPARE(device.reverbSend(0), 0.0f);
    QVERIFY2(peak(buses.at(0)) > 0.01, qPrintable(QString { "the bus got %1" }.arg(peak(buses.at(0)))));
    QVERIFY2(peak(buses.at(1)) < 1.0e-9, qPrintable(QString { "the unused bus got %1" }.arg(peak(buses.at(1)))));
}

void DrumSynthV2Test::test_voiceSend_unrouted_shouldSendNothing()
{
    // The compatibility case: every kit in every project saved until now.
    DrumSynthV2Device device { "Drum Synth V2" };
    device.processMidiNoteOn(36, 100);

    std::vector<double> bus(4096 * 2, 0.0);
    std::vector<std::span<double>> busSpans { std::span<double>(bus.data(), bus.size()) };
    std::vector<double> buffer(4096 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 4096, 44100 };
    context.sendBuses = std::span<const std::span<double>>(busSpans);
    device.processAudio(context);

    double busPeak = 0.0;
    double outputPeak = 0.0;
    for (size_t i = 0; i < bus.size(); i++) {
        busPeak = std::max(busPeak, std::abs(bus[i]));
        outputPeak = std::max(outputPeak, std::abs(buffer[i]));
    }
    QVERIFY2(outputPeak > 0.01, "the kick was not heard at all");
    QVERIFY2(busPeak < 1.0e-12, qPrintable(QString { "the bus got %1" }.arg(busPeak)));
}

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
        if (isFittedCymbal(voice)) {
            continue;
        }
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
    // The envelope ships transparent -- sustain at full -- so shaping a voice means pulling the
    // sustain down first. Without that the stage below has nothing to shape: the level never leaves
    // the top.
    device.updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 0.0f);
    const std::string holdKey { Constants::NahdXml::xmlKeyAmpHold().toStdString() };
    device.updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), holdKey, 0.2f);
    const std::string decayKey { Constants::NahdXml::xmlKeyAmpDecay().toStdString() };
    device.updateVoiceParameter(static_cast<int>(DrumSynthV2::VoiceIndex::Kick), decayKey, 0.2f);

    const auto tightened = tailLength(asRendered(fullTailBlocks), sampleRate);
    QVERIFY2(tightened < 0.2, qPrintable(QString { "Hold did not tighten the kick: %1 s, was %2 s" }.arg(tightened).arg(asShipped)));
}

void DrumSynthV2Test::test_ampEnvelope_fullSustain_shouldMatchV1Exactly()
{
    // What the sustain stage is for. Without one the envelope always decays to silence, so the
    // closest V2 could come to V1 was its defaults parked just past each voice's own tail -- which
    // measures thirty-odd decibels down rather than nothing at all. With the sustain at full and
    // the attack, hold and release at zero the envelope is a constant one, and the voice is V1's.
    const DrumSynthV2Device notes { "Notes" };
    for (int voice = 0; voice < DrumSynthV2::NumVoices; voice++) {
        if (isFittedCymbal(voice)) {
            continue;
        }
        const auto note = notes.voiceNote(voice);

        DrumSynthV2Device v2 { "V2" };
        v2.updateVoiceParameter(voice, Constants::NahdXml::xmlKeyAmpAttack().toStdString(), 0.0f);
        v2.updateVoiceParameter(voice, Constants::NahdXml::xmlKeyAmpHold().toStdString(), 0.0f);
        v2.updateVoiceParameter(voice, Constants::NahdXml::xmlKeyAmpRelease().toStdString(), 0.0f);
        v2.updateVoiceParameter(voice, Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 1.0f);

        v2.processMidiNoteOn(note, 100);
        std::vector<double> rendered;
        std::vector<double> buffer(frameCount * 2, 0.0);
        for (uint32_t block = 0; block < fullTailBlocks; block++) {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
            v2.processAudio(context);
            rendered.insert(rendered.end(), buffer.begin(), buffer.end());
        }

        const auto v1 = renderNote<DrumSynthDevice>(note, fullTailBlocks);
        QCOMPARE(rendered.size(), v1.size());

        double largest = 0.0;
        for (size_t i = 0; i < v1.size(); i++) {
            largest = std::max(largest, std::abs(rendered.at(i) - v1.at(i)));
        }
        QVERIFY2(largest < 1.0e-12,
                 qPrintable(QString { "%1 differs from V1 by %2" }.arg(DrumSynthV2::voiceName(voice)).arg(largest)));
    }
}

void DrumSynthV2Test::test_ampEnvelope_closed_shouldStopTheVoiceRendering()
{
    // A closed envelope has to take the voice out of hasActiveAudio(), or the device keeps being
    // rendered for the whole of an engine tail nobody can hear any more.
    const DrumSynthV2Device notes { "Notes" };
    DrumSynthV2Device device { "Test" };
    const auto kick = static_cast<int>(DrumSynthV2::VoiceIndex::Kick);
    // The envelope ships transparent -- sustain at full -- so shaping a voice means pulling the
    // sustain down first. Without that the stage below has nothing to shape: the level never leaves
    // the top.
    device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 0.0f);
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
        device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 0.0f);
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

void DrumSynthV2Test::test_renderVoiceAlone_shouldStopWhenTheVoiceDoes()
{
    DrumSynthV2Device device { "Preview" };
    const auto kick = static_cast<int>(DrumSynthV2::VoiceIndex::Kick);

    const auto rendered = device.renderVoiceAlone(kick, sampleRate, 8.0);
    QVERIFY2(!rendered.empty(), "nothing was rendered");

    const auto seconds = static_cast<double>(rendered.size() / 2) / sampleRate;
    // The kick rings for well over a second and nothing like the eight it was allowed, so the stop
    // condition is doing the work rather than the ceiling.
    QVERIFY2(seconds > 0.2 && seconds < 7.0, qPrintable(QString::number(seconds)));

    double peak = 0.0;
    for (auto && sample : rendered) {
        peak = std::max(peak, std::abs(sample));
    }
    QVERIFY2(peak > 0.01, qPrintable(QString::number(peak)));
}

void DrumSynthV2Test::test_renderVoiceAlone_shouldBeDeterministic()
{
    // The picture must not flicker between repaints, and a drum has no pitch to follow: the same
    // voice has to render the same frames every time, noise-based voices included.
    DrumSynthV2Device device { "Preview" };
    const auto hat = static_cast<int>(DrumSynthV2::VoiceIndex::ClosedHiHat);

    const auto first = device.renderVoiceAlone(hat, sampleRate, 4.0);
    const auto second = device.renderVoiceAlone(hat, sampleRate, 4.0);
    QCOMPARE(first.size(), second.size());
    for (size_t i = 0; i < first.size(); i++) {
        QCOMPARE(first.at(i), second.at(i));
    }
}

void DrumSynthV2Test::test_renderVoiceAlone_shouldRenderOnlyThatVoice()
{
    // One voice alone, so the picture is of the drum that is selected and not of whatever else the
    // kit happens to have sounding.
    DrumSynthV2Device device { "Preview" };
    const auto crash = static_cast<int>(DrumSynthV2::VoiceIndex::Crash);
    device.processMidiNoteOn(device.voiceNote(static_cast<int>(DrumSynthV2::VoiceIndex::Kick)), 127);

    const auto rendered = device.renderVoiceAlone(crash, sampleRate, 6.0);
    const auto onlyCrash = DrumSynthV2Device { "Clean" }.renderVoiceAlone(crash, sampleRate, 6.0);
    QCOMPARE(rendered.size(), onlyCrash.size());
    for (size_t i = 0; i < rendered.size(); i++) {
        QCOMPARE(rendered.at(i), onlyCrash.at(i));
    }
}

void DrumSynthV2Test::test_voicePreview_shouldPictureTheVoiceAndMeasureWhatIsHeard()
{
    DrumSynthV2Device device { "Preview" };
    const auto kick = static_cast<int>(DrumSynthV2::VoiceIndex::Kick);

    const auto preview = DrumVoicePreview::render(device, kick, 128, sampleRate);
    QCOMPARE(preview.peaks.size(), 128);
    QVERIFY2(preview.durationSeconds > 0.2, qPrintable(QString::number(preview.durationSeconds)));
    QVERIFY2(preview.audibleSeconds > 0.2, qPrintable(QString::number(preview.audibleSeconds)));

    // Something was drawn rather than a row of zeroes.
    double tallest = 0.0;
    for (auto && point : preview.peaks) {
        tallest = std::max(tallest, point.toDouble());
    }
    QVERIFY2(tallest > 0.01, qPrintable(QString::number(tallest)));
}

void DrumSynthV2Test::test_voicePreview_shouldNotDisturbTheDeviceItPictures()
{
    // The device being pictured is the one in the rack, and it may well be playing: rendering has
    // to happen on a copy or the preview would strike a voice the song is in the middle of.
    DrumSynthV2Device device { "Live" };
    const auto kick = static_cast<int>(DrumSynthV2::VoiceIndex::Kick);
    device.processMidiNoteOn(device.voiceNote(kick), 100);

    // Part way into the kick, where a preview landing on the same voice would be obvious.
    std::vector<double> before;
    std::vector<double> buffer(frameCount * 2, 0.0);
    for (uint32_t block = 0; block < 2; block++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
        device.processAudio(context);
    }

    DrumVoicePreview::render(device, kick, 64, sampleRate);

    // What the live device plays next has to be what it would have played anyway.
    DrumSynthV2Device untouched { "Live" };
    untouched.processMidiNoteOn(untouched.voiceNote(kick), 100);
    for (uint32_t block = 0; block < 2; block++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
        untouched.processAudio(context);
    }

    std::vector<double> afterPreview(frameCount * 2, 0.0);
    {
        AudioContext context { std::span(afterPreview.data(), afterPreview.size()), frameCount, sampleRate };
        device.processAudio(context);
    }
    std::vector<double> afterNothing(frameCount * 2, 0.0);
    {
        AudioContext context { std::span(afterNothing.data(), afterNothing.size()), frameCount, sampleRate };
        untouched.processAudio(context);
    }

    for (size_t i = 0; i < afterPreview.size(); i++) {
        QCOMPARE(afterPreview.at(i), afterNothing.at(i));
    }
}

void DrumSynthV2Test::test_voicePreview_shortEnvelope_shouldShortenOnlyWhatIsHeard()
{
    // The two figures part company as soon as the envelope does anything: the picture still shows
    // the whole drum, and the readout says how much of it is left.
    DrumSynthV2Device device { "Preview" };
    const auto kick = static_cast<int>(DrumSynthV2::VoiceIndex::Kick);

    const auto full = DrumVoicePreview::render(device, kick, 64, sampleRate);

    device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 0.0f);
    device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpHold().toStdString(), 0.0f);
    device.updateVoiceParameter(kick, Constants::NahdXml::xmlKeyAmpDecay().toStdString(), 0.05f);
    const auto tightened = DrumVoicePreview::render(device, kick, 64, sampleRate);

    QVERIFY2(tightened.audibleSeconds < full.audibleSeconds * 0.5,
             qPrintable(QString::number(tightened.audibleSeconds) + " vs " + QString::number(full.audibleSeconds)));
    QVERIFY2(std::abs(tightened.durationSeconds - full.durationSeconds) < 1.0e-9,
             qPrintable(QString::number(tightened.durationSeconds) + " vs " + QString::number(full.durationSeconds)));
}

void DrumSynthV2Test::test_voiceElapsedSeconds_shouldFollowTheVoice()
{
    // What the waveform view's playhead runs on: nothing before the strike, climbing with the
    // audio while the voice sounds, and nothing again once it has gone.
    DrumSynthV2Device device { "Playhead" };
    const auto kick = static_cast<int>(DrumSynthV2::VoiceIndex::Kick);
    device.setSampleRate(sampleRate);

    QVERIFY(!device.voiceElapsedSeconds(kick).has_value());

    device.processMidiNoteOn(device.voiceNote(kick), 127);
    const auto render = [&](uint32_t blocks) {
        std::vector<double> buffer(frameCount * 2, 0.0);
        for (uint32_t block = 0; block < blocks; block++) {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
            device.processAudio(context);
        }
    };

    render(2);
    const auto early = device.voiceElapsedSeconds(kick);
    QVERIFY(early.has_value());
    // Two blocks of 4096 at 44.1 kHz is a little under two tenths of a second.
    QVERIFY2(std::abs(*early - 2.0 * frameCount / sampleRate) < 0.01, qPrintable(QString::number(*early)));

    render(2);
    const auto later = device.voiceElapsedSeconds(kick);
    QVERIFY(later.has_value());
    QVERIFY2(*later > *early, "the playhead did not advance");

    // And gone once the voice has run out.
    render(fullTailBlocks);
    QVERIFY(!device.voiceElapsedSeconds(kick).has_value());
}

void DrumSynthV2Test::test_cymbals_v1_shouldNotTakeTheFit()
{
    // The fit belongs to V2 alone. The engines are shared, so the constants it is made of sit
    // behind a voicing the original device does not select -- and every kit written against V1 has
    // to keep sounding the way it did. Asserted through what the two disagree about: V1's crash
    // peaks an octave below the splash band, and its ride is hiss where V2's is metal.
    const DrumSynthV2Device notes { "Notes" };
    const auto v1Crash = spectrum(renderNote<DrumSynthDevice>(notes.voiceNote(static_cast<int>(DrumSynthV2::VoiceIndex::Crash)), fullTailBlocks), 0.35);
    QVERIFY(bandPower(v1Crash, 2000.0, 4000.0) > bandPower(v1Crash, 4000.0, 8000.0));

    const auto v1Ride = spectrum(renderNote<DrumSynthDevice>(notes.voiceNote(static_cast<int>(DrumSynthV2::VoiceIndex::Ride)), fullTailBlocks), 0.35);
    QVERIFY(bandFlatness(v1Ride, 5000.0, 16000.0) > 0.6);
}

void DrumSynthV2Test::test_cymbals_ride_shouldBeAsNoisyAsRealMetal()
{
    // A ride's shimmer *is* noise, and this is the regression that matters: fitted once against a
    // flatness measured across a wide band -- which reports tilt rather than noisiness -- the ride
    // came out four times more tonal than the recording and was heard as a dry bell. The recording
    // measures about 0.36 here. V1 sits at roughly 0.78, which is hiss laid over a cymbal, so the
    // cymbal has to be between the two rather than merely under V1.
    DrumSynthV2Device v2 { "V2" };
    const auto ride = static_cast<int>(DrumSynthV2::VoiceIndex::Ride);
    const auto ps = spectrum(v2.renderVoiceAlone(ride, sampleRate, 4.0), 0.35);
    const auto flatness = bandFlatness(ps, 5000.0, 16000.0);
    QVERIFY2(flatness > 0.15, qPrintable(QString { "ride is a bell, not a cymbal: flatness %1" }.arg(flatness)));
    QVERIFY2(flatness < 0.55, qPrintable(QString { "ride is hiss, not a cymbal: flatness %1" }.arg(flatness)));
}

void DrumSynthV2Test::test_cymbals_crash_shouldPeakInTheSplashBand()
{
    // What makes a crash splash is where its weight sits: the recording peaks between four and
    // eight kilohertz and falls away above and below. V1 peaks at two to four and is nine decibels
    // down in the splash band, which is heard as a wash rather than as a cymbal being hit.
    DrumSynthV2Device v2 { "V2" };
    const auto crash = static_cast<int>(DrumSynthV2::VoiceIndex::Crash);
    const auto ps = spectrum(v2.renderVoiceAlone(crash, sampleRate, 4.0), 0.35);
    const auto splash = bandPower(ps, 4000.0, 8000.0);
    for (const auto band : { std::pair { 1000.0, 2000.0 }, std::pair { 2000.0, 4000.0 }, std::pair { 8000.0, 16000.0 } }) {
        const auto other = bandPower(ps, band.first, band.second);
        QVERIFY2(splash > other,
                 qPrintable(QString { "crash does not peak in the splash band: %1-%2 Hz is louder" }.arg(band.first).arg(band.second)));
    }
}

void DrumSynthV2Test::test_cymbals_shouldHaveABody()
{
    // Neither cymbal had anything below 600 Hz worth measuring: V1's ride high-passed it all away,
    // and the crash's body envelope lasted twenty milliseconds. The recordings carry that band for
    // the whole of the sound, within about twelve decibels of their own total.
    DrumSynthV2Device v2 { "V2" };
    for (auto voice : { static_cast<int>(DrumSynthV2::VoiceIndex::Crash), static_cast<int>(DrumSynthV2::VoiceIndex::Ride) }) {
        const auto ps = spectrum(v2.renderVoiceAlone(voice, sampleRate, 4.0), 0.35);
        const auto body = 10.0 * std::log10(std::max(bandPower(ps, 200.0, 600.0), 1.0e-30));
        const auto whole = 10.0 * std::log10(std::max(bandPower(ps, 20.0, 20000.0), 1.0e-30));
        QVERIFY2(body - whole > -30.0,
                 qPrintable(QString { "%1 has no body: %2 dB under the whole" }
                              .arg(DrumSynthV2::voiceName(voice)).arg(body - whole)));
    }
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
