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

#include "fm_synth_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/devices/fm_synth_device.hpp"
#include "../../domain/dsp/lfo.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QRegularExpression>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <ranges>
#include <set>
#include <span>
#include <utility>
#include <vector>

namespace noteahead {

namespace {

//! Chosen with the note below so that one cycle is a whole number of samples: A4 at this rate is
//! exactly 100 samples long. Every test that asks whether the pitch moved needs that.
constexpr uint32_t SampleRate { 44000 };
constexpr uint8_t NoteA4 { 69 };
constexpr size_t PeriodA4 { 100 };
constexpr uint32_t FrameCount { 1024 };

std::vector<double> renderMono(FmSynthDevice & synth, size_t blocks)
{
    std::vector<double> mono;
    mono.reserve(blocks * FrameCount);
    for (size_t block = 0; block < blocks; block++) {
        std::vector<double> buffer(FrameCount * 2, 0.0);
        AudioContext context { std::span<double> { buffer.data(), buffer.size() }, FrameCount, SampleRate };
        synth.processAudio(context);
        for (uint32_t i = 0; i < FrameCount; i++) {
            mono.push_back((buffer[i * 2] + buffer[i * 2 + 1]) * 0.5);
        }
    }
    return mono;
}

//! Left and right RMS of one short Poly note, rendered after the previous one has died away.
std::pair<double, double> polyNoteSides(FmSynthDevice & synth, uint8_t note)
{
    synth.processMidiNoteOn(note, 127);
    std::vector<double> buffer(FrameCount * 2, 0.0);
    AudioContext context { std::span<double> { buffer.data(), buffer.size() }, FrameCount, SampleRate };
    synth.processAudio(context);
    synth.processMidiNoteOff(note);
    renderMono(synth, 64);

    double left = 0.0;
    double right = 0.0;
    for (uint32_t i = 0; i < FrameCount; i++) {
        left += buffer[i * 2] * buffer[i * 2];
        right += buffer[i * 2 + 1] * buffer[i * 2 + 1];
    }
    return { std::sqrt(left), std::sqrt(right) };
}

std::unique_ptr<FmSynthDevice> polySpreadSynth(float spread)
{
    auto synth = std::make_unique<FmSynthDevice>("Test FM");
    synth->setAmpRelease(0.0f);
    synth->setVoiceMode(FmSynthDevice::VoiceMode::Poly);
    synth->setPan(0.5f);
    synth->setPanSpread(spread);
    return synth;
}

double rootMeanSquare(const std::vector<double> & samples)
{
    if (samples.empty()) {
        return 0.0;
    }
    const double total = std::accumulate(samples.begin(), samples.end(), 0.0, [](double sum, double sample) { return sum + sample * sample; });
    return std::sqrt(total / static_cast<double>(samples.size()));
}

//! How fast the waveform moves relative to how big it is. A pure sine scores lowest, and every
//! sideband an operator adds sits above the fundamental and pushes it up, so this stands in for
//! brightness without needing a window aligned to a bin.
double brightness(const std::vector<double> & samples)
{
    if (samples.size() < 2) {
        return 0.0;
    }
    double travel = 0.0;
    for (size_t i = 1; i < samples.size(); i++) {
        travel += std::abs(samples[i] - samples[i - 1]);
    }
    const double rms = rootMeanSquare(samples);
    return rms > 0.0 ? travel / static_cast<double>(samples.size() - 1) / rms : 0.0;
}

//! Worst mismatch between the tail and the same tail one cycle of A4 earlier, relative to the peak.
//! Zero means the signal repeats at the note's own period, whatever else it is doing.
double periodError(const std::vector<double> & samples)
{
    const size_t start = samples.size() / 2;
    double worst = 0.0;
    double peak = 0.0;
    for (size_t i = start; i + PeriodA4 < samples.size(); i++) {
        worst = std::max(worst, std::abs(samples[i] - samples[i + PeriodA4]));
        peak = std::max(peak, std::abs(samples[i]));
    }
    return peak > 0.0 ? worst / peak : 0.0;
}

double meanOffset(const std::vector<double> & samples)
{
    if (samples.empty()) {
        return 0.0;
    }
    return std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
}

//! A synth playing one held A4 through the serial algorithm, with operator 2 as the modulator at
//! the given level. Level zero leaves operator 1 sounding on its own.
std::unique_ptr<FmSynthDevice> makeModulatedSynth(float modulatorLevel, uint8_t velocity = 127)
{
    auto synth = std::make_unique<FmSynthDevice>("Test FM");
    synth->setAlgorithm(0);
    synth->setOperatorLevel(1, modulatorLevel);
    synth->processMidiNoteOn(NoteA4, velocity);
    return synth;
}

} // namespace

void FmSynthTest::test_lpfSlope_shouldDefaultToTheSlopeItAlwaysHad()
{
    // 24 dB/oct is what these voices have always been, so that is where a device starts and where
    // every project that never touches the setting stays.
    const FmSynthDevice device { "FM Synth" };
    QCOMPARE(device.lpfSlope(), 1);
}

void FmSynthTest::test_lpfSlope_shallow_shouldKeepMoreOfTheTop()
{
    // Two poles instead of four. Worth asserting per device rather than once: the setting was added
    // to three synths at a stroke and reached the audio path of only one of them, which a test on
    // that one could not notice.
    //
    // Measured at the note's own fundamental with the corner set below it, rather than somewhere up
    // in the stop band: what a patch has up there depends on the patch, and a default one that is
    // close to a sine leaves nothing there to measure but the noise floor.
    const auto magnitudeAboveCutoff = [](int slope) {
        FmSynthDevice device { "FM Synth" };
        device.setLpfSlope(slope);
        device.setLpfCutoff(0.25f);
        device.setLpfResonance(0.0f);
        device.processMidiNoteOn(60, 127);

        const auto rate = static_cast<uint32_t>(Constants::defaultSampleRate());
        // Past the attack, so the measurement is of the filter rather than of the envelope.
        std::vector<double> warmUp(2048 * 2, 0.0);
        AudioContext warmUpContext { std::span(warmUp.data(), warmUp.size()), 2048, rate };
        device.processAudio(warmUpContext);

        std::vector<double> buffer(4096 * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), 4096, rate };
        device.processAudio(context);

        double re = 0.0, im = 0.0;
        for (size_t i = 0; i < 4096; i++) {
            const double phase = 2.0 * M_PI * 261.63 * static_cast<double>(i) / Constants::defaultSampleRate();
            re += buffer[i * 2] * std::cos(phase);
            im += buffer[i * 2] * std::sin(phase);
        }
        return std::hypot(re, im) / 4096.0;
    };

    const double steep = magnitudeAboveCutoff(1);
    const double shallow = magnitudeAboveCutoff(0);
    QVERIFY2(shallow > steep * 2.0, qPrintable(QString("steep %1, shallow %2").arg(steep).arg(shallow)));
}

void FmSynthTest::test_name_shouldReturnCorrectName()
{
    const std::string name = "Test FM";
    const FmSynthDevice synth { name };
    QCOMPARE(synth.name(), name);
}

void FmSynthTest::test_defaultValues_shouldBeCorrect()
{
    const FmSynthDevice synth { "Test FM" };

    QCOMPARE(synth.algorithm(), 0);
    QCOMPARE(synth.feedback(), 0.0f);
    QCOMPARE(synth.operatorLevel(0), 1.0f);
    for (size_t i = 1; i < FmSynthDevice::OperatorCount; i++) {
        QCOMPARE(synth.operatorLevel(i), 0.0f);
    }
    for (size_t i = 0; i < FmSynthDevice::OperatorCount; i++) {
        QCOMPARE(synth.operatorWaveform(i), FmOperator::Waveform::Sine);
        QCOMPARE(synth.operatorRatio(i), 1);
        QCOMPARE(synth.operatorDetune(i), 0.5f);
        QCOMPARE(synth.operatorVelocitySensitivity(i), 0.0f);
        QCOMPARE(synth.operatorKeyScale(i), 0.0f);
        QCOMPARE(synth.operatorSustain(i), 1.0f);
    }
    QCOMPARE(synth.lpfCutoff(), 1.0f);
    QCOMPARE(synth.hpfCutoff(), 0.0f);
    QCOMPARE(synth.ampSustain(), 1.0f);
    QCOMPARE(synth.voiceMode(), FmSynthDevice::VoiceMode::Poly);
    QCOMPARE(synth.pitchBendRange(), 2);
}

void FmSynthTest::test_parameterSetting_shouldUpdateValues()
{
    FmSynthDevice synth { "Test FM" };

    synth.setAlgorithm(5);
    QCOMPARE(synth.algorithm(), 5);
    synth.setFeedback(0.7f);
    QCOMPARE(synth.feedback(), 0.7f);
    synth.setLpfCutoff(0.25f);
    QCOMPARE(synth.lpfCutoff(), 0.25f);
    synth.setVoiceMode(FmSynthDevice::VoiceMode::Unison);
    QCOMPARE(synth.voiceMode(), FmSynthDevice::VoiceMode::Unison);
    synth.setModTarget(FmSynthDevice::ModTarget::Feedback);
    QCOMPARE(synth.modTarget(), FmSynthDevice::ModTarget::Feedback);
    synth.setLfoTarget(FmSynthDevice::LfoTarget::Pan);
    QCOMPARE(synth.lfoTarget(), FmSynthDevice::LfoTarget::Pan);
    synth.setPitchBendRange(12);
    QCOMPARE(synth.pitchBendRange(), 12);
}

void FmSynthTest::test_lfoEngagement_shouldDefaultToImmediate()
{
    // Zero for both is what makes a project saved before these existed sound exactly as it did.
    const FmSynthDevice synth { "Test FM" };
    QCOMPARE(synth.lfoDelay(), 0.0f);
    QCOMPARE(synth.lfoFade(), 0.0f);
    QCOMPARE(synth.lfo2Delay(), 0.0f);
    QCOMPARE(synth.lfo2Fade(), 0.0f);
}

void FmSynthTest::test_lfoEngagement_parameterSetting_shouldUpdateValues()
{
    FmSynthDevice synth { "Test FM" };

    synth.setLfoDelay(0.4f);
    QCOMPARE(synth.lfoDelay(), 0.4f);
    synth.setLfoFade(0.6f);
    QCOMPARE(synth.lfoFade(), 0.6f);
    synth.setLfo2Delay(0.3f);
    QCOMPARE(synth.lfo2Delay(), 0.3f);
    synth.setLfo2Fade(0.7f);
    QCOMPARE(synth.lfo2Fade(), 0.7f);
}

void FmSynthTest::test_operatorParameters_shouldBeIndependentPerOperator()
{
    FmSynthDevice synth { "Test FM" };

    for (size_t i = 0; i < FmSynthDevice::OperatorCount; i++) {
        synth.setOperatorRatio(i, static_cast<int>(i) + 2);
        synth.setOperatorLevel(i, 0.1f * static_cast<float>(i + 1));
        synth.setOperatorWaveform(i, static_cast<FmOperator::Waveform>(i));
    }

    for (size_t i = 0; i < FmSynthDevice::OperatorCount; i++) {
        QCOMPARE(synth.operatorRatio(i), static_cast<int>(i) + 2);
        QCOMPARE(synth.operatorLevel(i), 0.1f * static_cast<float>(i + 1));
        QCOMPARE(synth.operatorWaveform(i), static_cast<FmOperator::Waveform>(i));
    }
}

void FmSynthTest::test_algorithms_everyOne_shouldOnlyModulateHigherOperators()
{
    // The render loop walks the operators once, from the last to the first, and reads a modulator's
    // output straight out of this sample. That is only correct while every modulation path runs
    // downwards. A table entry pointing the other way would silently be reading last sample's
    // value, so the invariant is asserted rather than trusted.
    for (const auto & algorithm : FmSynthDevice::algorithms()) {
        for (size_t op = 0; op < FmSynthDevice::OperatorCount; op++) {
            for (size_t source = 0; source <= op; source++) {
                QVERIFY((algorithm.modulators.at(op) & (1u << source)) == 0);
            }
        }
    }
}

void FmSynthTest::test_algorithms_everyOne_shouldHaveACarrier()
{
    for (const auto & algorithm : FmSynthDevice::algorithms()) {
        QVERIFY(algorithm.carriers != 0);
    }
}

void FmSynthTest::test_algorithms_everyOne_shouldMakeOperatorOneACarrier()
{
    // What lets the init patch be a plain sine on operator 1 whatever algorithm is selected, and
    // what stops any algorithm from being silent when only operator 1 is turned up.
    for (const auto & algorithm : FmSynthDevice::algorithms()) {
        QVERIFY(algorithm.carriers & 1u);
    }
}

void FmSynthTest::test_algorithms_shouldAllBeDistinct()
{
    std::set<std::vector<int>> seen;
    for (const auto & algorithm : FmSynthDevice::algorithms()) {
        std::vector<int> shape { algorithm.carriers };
        for (const auto modulators : algorithm.modulators) {
            shape.push_back(modulators);
        }
        QVERIFY(seen.insert(shape).second);
    }
}

void FmSynthTest::test_algorithmNames_shouldNameEveryAlgorithm()
{
    QCOMPARE(FmSynthDevice::algorithmNames().size(), FmSynthDevice::AlgorithmCount);
    QCOMPARE(FmSynthDevice::algorithms().size(), FmSynthDevice::AlgorithmCount);
}

void FmSynthTest::test_ratioForSetting_shouldFollowTheDxGrid()
{
    QCOMPARE(FmSynthDevice::ratioForSetting(0), 0.5);
    QCOMPARE(FmSynthDevice::ratioForSetting(1), 1.0);
    QCOMPARE(FmSynthDevice::ratioForSetting(2), 2.0);
    QCOMPARE(FmSynthDevice::ratioForSetting(FmSynthDevice::MaxRatioSetting), 31.0);
}

void FmSynthTest::test_defaultPatch_shouldSoundAPlainSine()
{
    const auto synth = makeModulatedSynth(0.0f);
    const auto samples = renderMono(*synth, 4);

    QVERIFY(rootMeanSquare(samples) > 0.0);
    // A sine's travel per sample over its RMS at 100 samples to the cycle. Anything with sidebands
    // on it scores well above this.
    QVERIFY(brightness(samples) < 0.12);
    QVERIFY(periodError(samples) < 0.01);
}

void FmSynthTest::test_modulatorLevel_shouldBrightenTheOutput()
{
    auto quiet = makeModulatedSynth(0.0f);
    auto loud = makeModulatedSynth(1.0f);

    QVERIFY(brightness(renderMono(*loud, 4)) > brightness(renderMono(*quiet, 4)) * 3.0);
}

void FmSynthTest::test_modulatorLevel_shouldNotShiftThePitch()
{
    // The reason the engine adds the modulator to the carrier's phase rather than to its frequency.
    // At a whole-number ratio the sidebands land on harmonics of the note, so the output still
    // repeats at the note's own period no matter how far the index is opened up.
    for (const float level : { 0.0f, 0.5f, 1.0f }) {
        const auto synth = makeModulatedSynth(level);
        QVERIFY(periodError(renderMono(*synth, 4)) < 0.01);
    }
}

void FmSynthTest::test_modIndexModulation_atRest_shouldLeaveTheTimbreAlone()
{
    // The index modulation is a multiplier, so a modulation source sitting at rest has to fold back
    // to exactly the patch's own index rather than to some scaled version of it.
    auto plain = makeModulatedSynth(0.7f);

    FmSynthDevice modulated { "Test FM" };
    modulated.setAlgorithm(0);
    modulated.setOperatorLevel(1, 0.7f);
    modulated.setModTarget(FmSynthDevice::ModTarget::ModIndex);
    modulated.setModInt(0.5f); // Centre of the bipolar knob: no modulation
    modulated.processMidiNoteOn(NoteA4, 127);

    QCOMPARE(renderMono(modulated, 2), renderMono(*plain, 2));
}

void FmSynthTest::test_modEg_shouldReachEveryTarget()
{
    // The counterpart to the test above: at rest the Mod EG must change nothing, and off rest it
    // must change something -- on every target it offers, not just the default one.
    const auto render = [](FmSynthDevice::ModTarget target, float intensity) {
        FmSynthDevice synth { "Test FM" };
        synth.setAlgorithm(0);
        // The whole serial chain is live, so that the feedback target has a path to the output:
        // feedback sits on operator 4, and an operator at zero level feeds nothing to anything.
        synth.setOperatorLevel(1, 0.6f);
        synth.setOperatorLevel(2, 0.5f);
        synth.setOperatorLevel(3, 0.4f);
        synth.setFeedback(0.3f);
        synth.setLpfCutoff(0.25f);
        synth.setModTarget(target);
        synth.setModInt(intensity);
        synth.setModDecay(0.6f);
        synth.processMidiNoteOn(NoteA4, 127);
        return renderMono(synth, 3);
    };

    for (const auto target : { FmSynthDevice::ModTarget::Cutoff, FmSynthDevice::ModTarget::Pitch,
                               FmSynthDevice::ModTarget::ModIndex, FmSynthDevice::ModTarget::Feedback }) {
        // 0.5 is the centre of the bipolar intensity knob, which is where it does nothing.
        QVERIFY(render(target, 1.0f) != render(target, 0.5f));
    }
}

void FmSynthTest::test_feedback_shouldBrightenTheOutput()
{
    auto plain = makeModulatedSynth(0.0f);

    FmSynthDevice fed { "Test FM" };
    // Operator 4 is the feedback operator, and Additive is the one algorithm where it is heard
    // directly rather than through a chain.
    fed.setAlgorithm(7);
    fed.setOperatorLevel(0, 0.0f);
    fed.setOperatorLevel(3, 1.0f);
    fed.setFeedback(0.5f);
    fed.processMidiNoteOn(NoteA4, 127);

    QVERIFY(brightness(renderMono(fed, 4)) > brightness(renderMono(*plain, 4)) * 2.0);
}

void FmSynthTest::test_absSineCarrier_shouldNotOffsetTheOutput()
{
    // Abs Sine spends its whole cycle on one side of zero, so as a carrier it would push a constant
    // offset through the amp envelope and be heard as a thump at every note on. The voice's DC
    // blocker is what stops that.
    FmSynthDevice synth { "Test FM" };
    synth.setOperatorWaveform(0, FmOperator::Waveform::AbsSine);
    synth.processMidiNoteOn(NoteA4, 127);

    const auto samples = renderMono(synth, 4);
    const std::vector<double> tail { samples.begin() + static_cast<long>(samples.size() / 2), samples.end() };

    QVERIFY(rootMeanSquare(tail) > 0.0);
    QVERIFY(std::abs(meanOffset(tail)) < rootMeanSquare(tail) * 0.05);
}

void FmSynthTest::test_algorithm_additive_shouldNotBeLouderThanSerial()
{
    // Four carriers at whole-number ratios of one note are correlated, not independent, so their
    // amplitudes add rather than their powers. Without the carrier-count division, switching to
    // Additive would be a level jump rather than a change of timbre.
    FmSynthDevice serial { "Test FM" };
    serial.setAlgorithm(0);
    serial.processMidiNoteOn(NoteA4, 127);

    FmSynthDevice additive { "Test FM" };
    additive.setAlgorithm(7);
    for (size_t i = 0; i < FmSynthDevice::OperatorCount; i++) {
        additive.setOperatorLevel(i, 1.0f);
    }
    additive.processMidiNoteOn(NoteA4, 127);

    QVERIFY(rootMeanSquare(renderMono(additive, 4)) < rootMeanSquare(renderMono(serial, 4)) * 1.5);
}

void FmSynthTest::test_keyScale_shouldQuietenTheModulatorOnHighNotes()
{
    const auto brightnessAt = [](uint8_t note, float keyScale) {
        FmSynthDevice synth { "Test FM" };
        synth.setAlgorithm(0);
        synth.setOperatorLevel(1, 1.0f);
        synth.setOperatorKeyScale(1, keyScale);
        synth.processMidiNoteOn(note, 127);
        return brightness(renderMono(synth, 4));
    };

    // Two octaves above the break point, full key scaling takes the modulator well down. Below the
    // break point it must change nothing at all.
    QVERIFY(brightnessAt(84, 1.0f) < brightnessAt(84, 0.0f) * 0.75);
    QCOMPARE(brightnessAt(48, 1.0f), brightnessAt(48, 0.0f));
}

void FmSynthTest::test_velocitySensitivity_shouldQuietenTheModulatorOnSoftNotes()
{
    const auto brightnessAt = [](uint8_t velocity) {
        FmSynthDevice synth { "Test FM" };
        synth.setAlgorithm(0);
        synth.setOperatorLevel(1, 1.0f);
        synth.setOperatorVelocitySensitivity(1, 1.0f);
        synth.processMidiNoteOn(NoteA4, velocity);
        return brightness(renderMono(synth, 4));
    };

    // Velocity into a modulator's level is what makes an FM patch brighten as it is played harder.
    QVERIFY(brightnessAt(30) < brightnessAt(127) * 0.75);
}

void FmSynthTest::test_velocitySensitivity_zero_shouldLeaveTheModulatorAlone()
{
    auto soft = makeModulatedSynth(1.0f, 30);
    auto hard = makeModulatedSynth(1.0f, 127);

    // The default is zero, so velocity reaches the amp envelope but not the timbre: the two differ
    // in level and not in brightness.
    const double softBrightness = brightness(renderMono(*soft, 4));
    const double hardBrightness = brightness(renderMono(*hard, 4));
    QVERIFY(std::abs(softBrightness - hardBrightness) < hardBrightness * 0.01);
}

void FmSynthTest::test_noteOn_shouldProduceAudio()
{
    FmSynthDevice synth { "Test FM" };
    QVERIFY(!synth.hasActiveAudio());

    synth.processMidiNoteOn(NoteA4, 127);
    QVERIFY(synth.hasActiveAudio());
    QVERIFY(rootMeanSquare(renderMono(synth, 1)) > 0.0001);
}

void FmSynthTest::test_noteOff_shouldEventuallySilenceTheVoice()
{
    FmSynthDevice synth { "Test FM" };
    synth.setAmpRelease(0.0f);
    synth.processMidiNoteOn(NoteA4, 127);
    renderMono(synth, 1);

    synth.processMidiNoteOff(NoteA4);
    renderMono(synth, 64);

    QVERIFY(!synth.hasActiveAudio());
}

void FmSynthTest::test_ampRelease_shouldGovernTheTail_whateverTheOperatorRelease()
{
    // The Amp EG owns the note's tail. It used to be in series with each carrier's own release, and
    // the shorter of the two won: with the operators left at their defaults, winding the Amp EG's
    // release to the top changed nothing audible, because the carrier had already closed.
    const auto blocksUntilSilent = [](float ampRelease) {
        FmSynthDevice synth { "Test FM" };
        synth.setAmpRelease(ampRelease);
        synth.processMidiNoteOn(NoteA4, 127);
        renderMono(synth, 1);
        synth.processMidiNoteOff(NoteA4);

        int blocks = 0;
        for (; blocks < 100; blocks++) {
            if (rootMeanSquare(renderMono(synth, 1)) < 1.0e-5) {
                break;
            }
        }
        return blocks;
    };

    // Operator releases are left at their defaults throughout: that is the case that was broken.
    QVERIFY(blocksUntilSilent(0.8f) > blocksUntilSilent(0.1f) * 4);
}

void FmSynthTest::test_ampRelease_short_shouldCutALongOperatorRelease()
{
    // And the other direction: the Amp EG is the master, so a short release ends the note however
    // the operators are set.
    FmSynthDevice synth { "Test FM" };
    synth.setAmpRelease(0.1f);
    synth.setAlgorithm(0);
    synth.setOperatorLevel(1, 1.0f);
    synth.setOperatorSustain(1, 1.0f);
    synth.processMidiNoteOn(NoteA4, 127);
    renderMono(synth, 1);
    synth.processMidiNoteOff(NoteA4);
    renderMono(synth, 32);

    QVERIFY(!synth.hasActiveAudio());
}

void FmSynthTest::test_noteOff_shouldNotChangeTheTimbre()
{
    // Releasing a note fades the sound it currently is; it does not turn it into a different one.
    // The operators hold where they are and only the amp envelope moves, so the tail keeps the
    // sidebands the note was sounding with. Released along with the voice, the modulators collapse
    // on their own far shorter schedule and a rich patch drops to a bare sine at note off.
    FmSynthDevice synth { "Test FM" };
    synth.setAlgorithm(0);
    synth.setOperatorLevel(1, 1.0f);
    synth.setOperatorSustain(1, 1.0f);
    synth.setAmpRelease(0.8f);
    synth.processMidiNoteOn(NoteA4, 127);

    const double held = brightness(renderMono(synth, 2));
    synth.processMidiNoteOff(NoteA4);
    const double released = brightness(renderMono(synth, 2));

    QVERIFY(std::abs(released - held) < held * 0.1);
}

void FmSynthTest::test_allNotesOff_shouldReleaseEveryVoice()
{
    FmSynthDevice synth { "Test FM" };
    synth.setAmpRelease(0.0f);
    for (uint8_t note = 60; note < 68; note++) {
        synth.processMidiNoteOn(note, 127);
    }
    renderMono(synth, 1);

    synth.processMidiAllNotesOff();
    renderMono(synth, 64);

    QVERIFY(!synth.hasActiveAudio());
}

void FmSynthTest::test_panSpread_poly_shouldAlternateSides()
{
    // Poly hands notes out round-robin, so two staccato notes land on the first two slots' sides
    auto synth = polySpreadSynth(1.0f);
    const auto [firstLeft, firstRight] = polyNoteSides(*synth, NoteA4);
    QVERIFY(!synth->hasActiveAudio());
    const auto [secondLeft, secondRight] = polyNoteSides(*synth, NoteA4 + 2);
    QVERIFY2(firstLeft > 4.0 * firstRight, qPrintable(QString { "%1 vs %2" }.arg(firstLeft).arg(firstRight)));
    QVERIFY2(secondRight > 4.0 * secondLeft, qPrintable(QString { "%1 vs %2" }.arg(secondRight).arg(secondLeft)));
}

void FmSynthTest::test_panSpread_polyZero_shouldBeMono()
{
    auto synth = polySpreadSynth(0.0f);
    for (auto && note : { NoteA4, static_cast<uint8_t>(NoteA4 + 2) }) {
        const auto [left, right] = polyNoteSides(*synth, note);
        QVERIFY(left > 0.0);
        QVERIFY2(std::abs(left - right) < 1.0e-9 * left, qPrintable(QString { "%1 vs %2" }.arg(left).arg(right)));
    }
}

void FmSynthTest::test_polyphony_shouldPlayEveryVoice()
{
    FmSynthDevice one { "Test FM" };
    one.processMidiNoteOn(60, 127);

    FmSynthDevice many { "Test FM" };
    for (uint8_t note = 60; note < 60 + FmSynthDevice::MaxVoices; note++) {
        many.processMidiNoteOn(note, 127);
    }

    QVERIFY(rootMeanSquare(renderMono(many, 2)) > rootMeanSquare(renderMono(one, 2)) * 1.5);
}

void FmSynthTest::test_pitchBend_shouldChangeTheOutput()
{
    auto plain = makeModulatedSynth(0.0f);

    FmSynthDevice bent { "Test FM" };
    bent.processMidiNoteOn(NoteA4, 127);
    bent.processMidiPitchBend(16383, 0);

    QVERIFY(renderMono(bent, 2) != renderMono(*plain, 2));
}

void FmSynthTest::test_midiCcModWheel_atRest_shouldLeaveThePatchAlone()
{
    // The wheel is neutral at zero, like the Synth's: resting it must sound exactly like a patch
    // that never saw the wheel, and only pushing it up may add modulation. The intensity knob is
    // the bipolar one, and the wheel never touches it.
    const auto renderWithWheel = [](int wheel) {
        FmSynthDevice synth { "Test FM" };
        synth.setLfoRate(0.8f);
        synth.setLfoTarget(FmSynthDevice::LfoTarget::Pitch);
        if (wheel >= 0) {
            synth.processMidiCc(1, static_cast<uint8_t>(wheel), 0);
        }
        synth.processMidiNoteOn(NoteA4, 127);
        return renderMono(synth, 4);
    };

    const auto untouched = renderWithWheel(-1);
    const auto atRest = renderWithWheel(0);
    const auto pushedUp = renderWithWheel(127);

    QCOMPARE(atRest, untouched);
    QVERIFY(pushedUp != untouched);
}

void FmSynthTest::test_midiCcModWheel_shouldNotMoveTheIntensityKnob()
{
    FmSynthDevice synth { "Test FM" };
    synth.setLfoInt(0.75f);

    synth.processMidiCc(1, 127, 0);

    QCOMPARE(synth.lfoInt(), 0.75f);
}

void FmSynthTest::test_resetAllControllers_shouldTakeBackTheModWheel()
{
    const auto renderAfter = [](bool wheelThenReset) {
        FmSynthDevice synth { "Test FM" };
        synth.setLfoRate(0.8f);
        synth.setLfoTarget(FmSynthDevice::LfoTarget::Pitch);
        if (wheelThenReset) {
            synth.processMidiCc(1, 127, 0);
            synth.processMidiCc(121, 0, 0);
        }
        synth.processMidiNoteOn(NoteA4, 127);
        return renderMono(synth, 4);
    };

    QCOMPARE(renderAfter(true), renderAfter(false));
}

void FmSynthTest::test_delay_atZeroMix_shouldLeaveNothingBehind()
{
    // The mix defaults to zero, so a patch that says nothing about the delay stays dry -- every
    // preset so far was voiced before the delay existed and must sound as it did.
    FmSynthDevice synth { "Test FM" };
    synth.setAmpRelease(0.0f);
    synth.processMidiNoteOn(NoteA4, 127);
    renderMono(synth, 2);
    synth.processMidiNoteOff(NoteA4);
    renderMono(synth, 8);

    QVERIFY(!synth.hasActiveAudio());
    QCOMPARE(rootMeanSquare(renderMono(synth, 4)), 0.0);
}

void FmSynthTest::test_delay_shouldRepeatAfterTheNoteStops()
{
    // What a delay is for: sound after the voice has gone. The note is released and left to fall
    // silent, so anything heard past that point can only be the delay.
    FmSynthDevice synth { "Test FM" };
    synth.setAmpRelease(0.0f);
    // The time is in seconds, so this is 80 ms -- short enough to have come back round inside the
    // window below, where the half-second default would not have.
    synth.setDelayTime(0.08f);
    synth.setDelayMix(0.8f);
    synth.setDelayFeedback(0.6f);
    synth.processMidiNoteOn(NoteA4, 127);
    renderMono(synth, 2);
    synth.processMidiNoteOff(NoteA4);
    renderMono(synth, 8);

    QVERIFY(!synth.hasActiveAudio());
    QVERIFY(rootMeanSquare(renderMono(synth, 4)) > 0.0001);
}

void FmSynthTest::test_voiceMode_unison_shouldDetuneTheStack()
{
    FmSynthDevice synth { "Test FM" };
    synth.setVoiceMode(FmSynthDevice::VoiceMode::Unison);
    synth.setVoiceDepth(0.8f);
    synth.processMidiNoteOn(NoteA4, 127);
    renderMono(synth, 1);

    std::set<long long> frequencies;
    for (size_t i = 0; i < FmSynthDevice::MaxVoices; i++) {
        frequencies.insert(std::llround(synth.voiceGlideFrequency(i) * 1000.0));
    }

    QCOMPARE(frequencies.size(), static_cast<size_t>(FmSynthDevice::MaxVoices));
}

namespace {

//! A patch of @p mode that lets every voice fall silent between notes, so each note starts its
//! voices from nothing: the case Phase Randomness is about.
std::unique_ptr<FmSynthDevice> phaseTestSynth(FmSynthDevice::VoiceMode mode, float phaseRandomness)
{
    auto synth = std::make_unique<FmSynthDevice>("Phase");
    synth->setVoiceMode(mode);
    synth->setVoiceDepth(0.5f);
    synth->setAmpSustain(1.0f);
    synth->setAmpRelease(0.0f);
    synth->setPhaseRandomness(phaseRandomness);
    return synth;
}

//! The first 50 ms of a note, left channel, after which the note is released and left to go silent.
std::vector<double> attackOf(FmSynthDevice & synth, uint8_t note)
{
    const auto rate = static_cast<uint32_t>(Constants::defaultSampleRate());
    const auto frames = static_cast<uint32_t>(rate / 20);
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), frames, rate };
    synth.processMidiNoteOn(note, 100);
    synth.processAudio(context);
    synth.processMidiNoteOff(note);
    std::vector<double> silence(2048 * 2, 0.0);
    AudioContext silenceContext { std::span(silence.data(), silence.size()), 2048, rate };
    for (int block = 0; block < 20; block++) {
        std::fill(silence.begin(), silence.end(), 0.0);
        synth.processAudio(silenceContext);
    }
    std::vector<double> left;
    for (size_t frame = 0; frame < buffer.size(); frame += 2) {
        left.push_back(buffer[frame]);
    }
    return left;
}

double attackDb(FmSynthDevice & synth, uint8_t note)
{
    const auto attack = attackOf(synth, note);
    const double power = std::accumulate(attack.begin(), attack.end(), 0.0, [](double sum, double sample) { return sum + sample * sample; }) / static_cast<double>(attack.size());
    return 10.0 * std::log10(power);
}

double largestDifference(const std::vector<double> & a, const std::vector<double> & b)
{
    double largest = 0.0;
    for (size_t i = 0; i < std::min(a.size(), b.size()); i++) {
        largest = std::max(largest, std::abs(a.at(i) - b.at(i)));
    }
    return largest;
}

} // namespace

void FmSynthTest::test_phaseRandomness_default_shouldBeFullyRandom()
{
    // Fully random, which is how every voice always started
    QCOMPARE(FmSynthDevice { "Phase" }.phaseRandomness(), 1.0f);
}

void FmSynthTest::test_phaseRandomness_shouldRoundTrip()
{
    FmSynthDevice out { "Phase" };
    out.setPhaseRandomness(0.3f);
    QString xml;
    {
        NahdXmlWriter writer { xml };
        out.serializeToXml(writer);
    }
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    FmSynthDevice in { "Phase" };
    in.deserializeFromXml(reader);

    QVERIFY(std::abs(in.phaseRandomness() - 0.3f) < 0.001f);
}

void FmSynthTest::test_phaseRandomness_projectWithoutIt_shouldLoadFullyRandom()
{
    FmSynthDevice out { "Phase" };
    out.setPhaseRandomness(0.0f);
    QString xml;
    {
        NahdXmlWriter writer { xml };
        out.serializeToXml(writer);
    }
    // As a project saved before the control existed has it
    xml.remove(QRegularExpression { R"(<Parameter name="phaseRandomness"[^>]*/>)" });
    QVERIFY(!xml.contains("phaseRandomness"));
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    FmSynthDevice in { "Phase" };
    in.deserializeFromXml(reader);

    QCOMPARE(in.phaseRandomness(), 1.0f);
}

void FmSynthTest::test_phaseRandomness_zero_shouldStartEveryNoteTheSame()
{
    for (const auto mode : { FmSynthDevice::VoiceMode::Unison, FmSynthDevice::VoiceMode::Supersaw }) {
        auto synth = phaseTestSynth(mode, 0.0f);
        // A supersaw voice's damping filter keeps the last of the note before, some 90 dB down, so
        // a fresh device's first note is not quite the one after it. The phases are what is tested.
        attackOf(*synth, 64);
        const auto first = attackOf(*synth, 64);
        const auto second = attackOf(*synth, 64);
        const auto third = attackOf(*synth, 64);

        // Every voice from the same phase, so the same note is the same note, sample for sample
        QCOMPARE(largestDifference(first, second), 0.0);
        QCOMPARE(largestDifference(first, third), 0.0);
    }
}

void FmSynthTest::test_phaseRandomness_zeroDual_shouldStartEveryPairTheSame()
{
    // Successive notes go round the pairs, and each pair has to start from the same two phases
    auto synth = phaseTestSynth(FmSynthDevice::VoiceMode::Dual, 0.0f);
    const auto first = attackOf(*synth, 64);
    for (int i = 0; i < 4; i++) {
        QCOMPARE(largestDifference(first, attackOf(*synth, 64)), 0.0);
    }
}

void FmSynthTest::test_phaseRandomness_full_shouldVaryFromNoteToNote()
{
    // What the control exists to take away: a fresh random stack per note, and so a different level
    auto synth = phaseTestSynth(FmSynthDevice::VoiceMode::Unison, 1.0f);
    std::vector<double> levels;
    for (int i = 0; i < 12; i++) {
        levels.push_back(attackDb(*synth, 64));
    }
    const auto [low, high] = std::ranges::minmax(levels);
    QVERIFY2(high - low > 2.0, qPrintable(QString::number(high - low)));
}

void FmSynthTest::test_phaseRandomness_zero_shouldStartAsLoudAsTheRandomAverage()
{
    // The fixed spread is meant to start where a randomly started note does on average, so turning
    // the control down evens the notes out without changing the mix. Seven phases cannot be spread
    // as evenly as eight, so the supersaw gets more room than the unison.
    for (const auto mode : { FmSynthDevice::VoiceMode::Unison, FmSynthDevice::VoiceMode::Supersaw }) {
        const double tolerance = mode == FmSynthDevice::VoiceMode::Unison ? 1.0 : 2.5;
        for (const uint8_t note : { 36, 52, 64, 76, 84 }) {
            auto random = phaseTestSynth(mode, 1.0f);
            double randomPower = 0.0;
            constexpr int trials = 40;
            for (int i = 0; i < trials; i++) {
                randomPower += std::pow(10.0, attackDb(*random, note) / 10.0);
            }
            const double randomDb = 10.0 * std::log10(randomPower / trials);

            auto spread = phaseTestSynth(mode, 0.0f);
            const double spreadDb = attackDb(*spread, note);

            QVERIFY2(std::abs(spreadDb - randomDb) < tolerance, qPrintable(QString { "mode %1, note %2: spread %3 dB, random %4 dB" }.arg(static_cast<int>(mode)).arg(note).arg(spreadDb).arg(randomDb)));
        }
    }
}

void FmSynthTest::test_phaseRandomness_partial_shouldRenderTheSameEveryTime()
{
    // Random only by the device's own seeded generator, so a render is reproducible at any setting
    auto a = phaseTestSynth(FmSynthDevice::VoiceMode::Unison, 0.4f);
    auto b = phaseTestSynth(FmSynthDevice::VoiceMode::Unison, 0.4f);
    for (int i = 0; i < 4; i++) {
        QCOMPARE(largestDifference(attackOf(*a, 64), attackOf(*b, 64)), 0.0);
    }
}

namespace {

//! Clear direction changes of the level, in 20 ms windows over 0.3 s of a held note with LFO 1 at
//! 8 Hz on Volume: several at full speed, and next to none once the divider slows it to a crawl.
//! Windows that long average the waveform out, and the threshold ignores what beating is left.
int tremoloTurns(FmSynthDevice & synth, int divider)
{
    synth.setAmpAttack(0.0f);
    synth.setAmpSustain(1.0f);
    synth.setLfoTarget(FmSynthDevice::LfoTarget::Volume);
    synth.setLfoMode(Lfo::Mode::Normal);
    synth.setLfoRate(static_cast<float>(ParameterMapper::unmapLfoFrequency(8.0, 0.05, 20.0)));
    synth.setLfoInt(1.0f);
    synth.setLfoRateDivider(divider);
    const auto rate = static_cast<uint32_t>(Constants::defaultSampleRate());
    const auto frames = static_cast<uint32_t>(rate * 4 / 10);
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), frames, rate };
    synth.processMidiNoteOn(64, 100);
    synth.processAudio(context);

    const size_t window = rate / 50;
    std::vector<double> levels;
    for (size_t start = rate / 10; start + window <= frames; start += window) {
        double power = 0.0;
        for (size_t i = start; i < start + window; i++) {
            power += buffer[i * 2] * buffer[i * 2];
        }
        levels.push_back(power / static_cast<double>(window));
    }
    int turns = 0;
    int direction = 0;
    for (size_t i = 1; i < levels.size(); i++) {
        const double change = levels[i] - levels[i - 1];
        if (std::abs(change) < 0.2 * std::max(levels[i], levels[i - 1])) {
            continue;
        }
        const int next = change > 0 ? 1 : -1;
        if (direction != 0 && next != direction) {
            turns++;
        }
        direction = next;
    }
    return turns;
}

} // namespace

void FmSynthTest::test_lfoRateDivider_default_shouldBeOne()
{
    FmSynthDevice synth { "Divider" };
    QCOMPARE(synth.lfoRateDivider(), 1);
    QCOMPARE(synth.lfo2RateDivider(), 1);
}

void FmSynthTest::test_lfoRateDivider_shouldRoundTrip()
{
    FmSynthDevice out { "Divider" };
    out.setLfoRateDivider(8);
    out.setLfo2RateDivider(64);
    QString xml;
    {
        NahdXmlWriter writer { xml };
        out.serializeToXml(writer);
    }
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    FmSynthDevice in { "Divider" };
    in.deserializeFromXml(reader);
    QCOMPARE(in.lfoRateDivider(), 8);
    QCOMPARE(in.lfo2RateDivider(), 64);
}

void FmSynthTest::test_lfoRateDivider_projectWithoutIt_shouldLoadOne()
{
    FmSynthDevice out { "Divider" };
    out.setLfoRateDivider(8);
    out.setLfo2RateDivider(8);
    QString xml;
    {
        NahdXmlWriter writer { xml };
        out.serializeToXml(writer);
    }
    // As a project saved before the control existed has it
    xml.remove(QRegularExpression { R"(<Parameter name="lfo2?RateDivider"[^>]*/>)" });
    QVERIFY(!xml.contains("RateDivider"));
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    FmSynthDevice in { "Divider" };
    in.deserializeFromXml(reader);
    QCOMPARE(in.lfoRateDivider(), 1);
    QCOMPARE(in.lfo2RateDivider(), 1);
}

void FmSynthTest::test_lfoRateDivider_shouldSlowTheVoicesLfo()
{
    // Reaches the LFO of every voice, not just the parameter
    FmSynthDevice fast { "Fast" };
    FmSynthDevice slow { "Slow" };
    const int fastTurns = tremoloTurns(fast, 1);
    const int slowTurns = tremoloTurns(slow, Lfo::MaxRateDivider);
    QVERIFY2(fastTurns >= 3, qPrintable(QString::number(fastTurns)));
    QVERIFY2(slowTurns <= 1, qPrintable(QString::number(slowTurns)));
}

namespace {

//! Renders a held tremolo note that starts after @p silentFrames of silence, rendered or, with
//! @p idle, skipped the way the engine skips a silent device.
std::vector<double> tremoloNoteAfterSilence(bool freeRun, uint32_t silentFrames, bool idle = false)
{
    FmSynthDevice synth { "Free Run" };
    synth.setAmpAttack(0.0f);
    synth.setAmpSustain(1.0f);
    synth.setLfoTarget(FmSynthDevice::LfoTarget::Volume);
    synth.setLfoRate(static_cast<float>(ParameterMapper::unmapLfoFrequency(4.0, 0.05, 20.0)));
    synth.setLfoInt(1.0f);
    synth.setLfoFreeRun(freeRun);
    const auto rate = static_cast<uint32_t>(Constants::defaultSampleRate());
    if (silentFrames > 0) {
        std::vector<double> silence(static_cast<size_t>(silentFrames) * 2, 0.0);
        AudioContext silentContext { std::span(silence.data(), silence.size()), silentFrames, rate };
        if (idle) {
            synth.advanceIdle(silentContext);
        } else {
            synth.processAudio(silentContext);
        }
    }
    synth.processMidiNoteOn(64, 100);
    const auto frames = rate / 2;
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), frames, rate };
    synth.processAudio(context);
    return buffer;
}

//! The phase LED of a saw LFO 1 just after a note that follows @p silentFrames of silence.
float sawLedAfterNote(bool freeRun, uint32_t silentFrames)
{
    FmSynthDevice synth { "LED" };
    synth.setLfoWaveform(Lfo::Waveform::Saw);
    synth.setLfoRate(static_cast<float>(ParameterMapper::unmapLfoFrequency(4.0, 0.05, 20.0)));
    synth.setLfoFreeRun(freeRun);
    const auto rate = static_cast<uint32_t>(Constants::defaultSampleRate());
    std::vector<double> silence(static_cast<size_t>(silentFrames) * 2, 0.0);
    AudioContext silentContext { std::span(silence.data(), silence.size()), silentFrames, rate };
    synth.processAudio(silentContext);
    synth.processMidiNoteOn(64, 100);
    std::vector<double> buffer(2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 1, rate };
    synth.processAudio(context);
    return synth.lfoLed(0);
}

//! The LED over two blocks of silence after a note has died away, rendered as the engine renders a
//! silent device with events scheduled, or skipped as it skips one without
std::pair<float, float> sawLedsOnceSilent(bool freeRun, bool rendered)
{
    FmSynthDevice synth { "LED" };
    synth.setLfoWaveform(Lfo::Waveform::Saw);
    synth.setLfoRate(static_cast<float>(ParameterMapper::unmapLfoFrequency(4.0, 0.05, 20.0)));
    synth.setLfoFreeRun(freeRun);
    const auto rate = static_cast<uint32_t>(Constants::defaultSampleRate());
    std::vector<double> buffer(3000 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 3000, rate };
    synth.processMidiNoteOn(64, 100);
    synth.processAudio(context);
    synth.processMidiNoteOff(64);
    for (int block = 0; block < 1000 && synth.hasActiveAudio(); block++) {
        synth.processAudio(context);
    }
    const auto silentBlock = [&] {
        if (rendered) {
            synth.processAudio(context);
        } else {
            synth.advanceIdle(context);
        }
        return synth.lfoLed(0);
    };
    const float first = silentBlock();
    return { first, silentBlock() };
}

} // namespace

void FmSynthTest::test_lfoFreeRun_default_shouldBeOff()
{
    const FmSynthDevice synth { "Free Run" };
    QVERIFY(!synth.lfoFreeRun());
    QVERIFY(!synth.lfo2FreeRun());
}

void FmSynthTest::test_lfoFreeRun_shouldRoundTrip()
{
    FmSynthDevice out { "Free Run" };
    out.setLfo2FreeRun(true);
    QString xml;
    {
        NahdXmlWriter writer { xml };
        out.serializeToXml(writer);
    }
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    FmSynthDevice in { "Free Run" };
    in.deserializeFromXml(reader);
    QVERIFY(!in.lfoFreeRun());
    QVERIFY(in.lfo2FreeRun());
}

void FmSynthTest::test_lfoFreeRun_projectWithoutIt_shouldLoadOff()
{
    FmSynthDevice out { "Free Run" };
    out.setLfoFreeRun(true);
    out.setLfo2FreeRun(true);
    QString xml;
    {
        NahdXmlWriter writer { xml };
        out.serializeToXml(writer);
    }
    // As a project saved before the switch existed has it
    xml.remove(QRegularExpression { R"(<Parameter name="lfo2?FreeRun"[^>]*/>)" });
    QVERIFY(!xml.contains("FreeRun"));
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    FmSynthDevice in { "Free Run" };
    in.deserializeFromXml(reader);
    QVERIFY(!in.lfoFreeRun());
    QVERIFY(!in.lfo2FreeRun());
}

void FmSynthTest::test_lfoFreeRun_off_shouldStartEveryNoteFromTheTop()
{
    QCOMPARE(largestDifference(tremoloNoteAfterSilence(false, 0), tremoloNoteAfterSilence(false, 3000)), 0.0);
}

void FmSynthTest::test_lfoFreeRun_on_shouldKeepRunningThroughTheSilence()
{
    // A quarter of a 4 Hz cycle: the note comes in on another part of the tremolo
    const double difference = largestDifference(tremoloNoteAfterSilence(true, 0), tremoloNoteAfterSilence(true, 3000));
    QVERIFY2(difference > 0.01, qPrintable(QString::number(difference)));
}

void FmSynthTest::test_lfoFreeRun_idle_shouldKeepTimeAsRenderingDoes()
{
    const double difference = largestDifference(tremoloNoteAfterSilence(true, 3000, true), tremoloNoteAfterSilence(true, 3000, false));
    QVERIFY2(difference < 1.0e-6, qPrintable(QString::number(difference)));
}

void FmSynthTest::test_lfoLed_noteOn_shouldRestartTheShape()
{
    const float led = sawLedAfterNote(false, 3000);
    QVERIFY2(led < -0.99f, qPrintable(QString::number(led)));
}

void FmSynthTest::test_lfoLed_freeRun_shouldNotRestartOnTheNote()
{
    // A quarter of the way round the saw
    const float led = sawLedAfterNote(true, 3000);
    QVERIFY2(std::abs(led - -0.5f) < 0.01f, qPrintable(QString::number(led)));
}

void FmSynthTest::test_lfoLed_idle_shouldGoDark()
{
    QCOMPARE(sawLedsOnceSilent(false, false), std::make_pair(-1.0f, -1.0f));
}

void FmSynthTest::test_lfoLed_renderedSilence_shouldGoDark()
{
    QCOMPARE(sawLedsOnceSilent(false, true), std::make_pair(-1.0f, -1.0f));
}

void FmSynthTest::test_lfoLed_idleFreeRun_shouldKeepShowingTheLfo()
{
    // A quarter of a cycle apart
    const auto [first, second] = sawLedsOnceSilent(true, false);
    QVERIFY2(std::abs(std::abs(second - first) - 0.5f) < 0.01f || std::abs(std::abs(second - first) - 1.5f) < 0.01f, qPrintable(QString { "%1 -> %2" }.arg(first).arg(second)));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FmSynthTest)
