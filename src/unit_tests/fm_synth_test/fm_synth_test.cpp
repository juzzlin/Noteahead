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

#include "../../domain/devices/fm_synth_device.hpp"

#include <QTest>
#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <set>
#include <span>
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
        synth.setLpfCutoff(0.5f);
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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FmSynthTest)
