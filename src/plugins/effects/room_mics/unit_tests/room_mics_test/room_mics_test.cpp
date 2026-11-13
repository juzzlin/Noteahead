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

#include "room_mics_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../common/parameter_mapper.hpp"
#include "../../../../../domain/dsp/audio_context.hpp"
#include "../../room_acoustics.hpp"
#include "../../room_convolver.hpp"
#include "../../room_mics.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <span>
#include <vector>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;
using namespace RoomAcoustics;

constexpr double rate { 48000.0 };

void setParameter(RoomMics & effect, const QString & key, float value)
{
    if (const auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

//! Only the pair @p group heard, at unity.
void soloGroup(RoomMics & effect, MicGroup group)
{
    for (size_t g = 0; g < MicGroupCount; g++) {
        setParameter(effect, C::xmlKeyMicLevel(g), g == static_cast<size_t>(group) ? static_cast<float>(ParameterMapper::unmapFader(1.0)) : 0.0f);
    }
}

std::unique_ptr<RoomMics> makeRoomMics()
{
    auto effect = std::make_unique<RoomMics>();
    effect->setSampleRate(rate);
    return effect;
}

//! The effect's own output for @p stereo, rendered offline in blocks as the engine would.
std::vector<double> render(RoomMics & effect, std::vector<double> stereo, bool offline = true)
{
    if (offline) {
        effect.finishDesign();
    }
    const size_t frames = stereo.size() / 2;
    constexpr size_t block = 256;
    for (size_t start = 0; start < frames; start += block) {
        const size_t count = std::min(block, frames - start);
        AudioContext context;
        context.buffer = std::span(stereo.data() + start * 2, count * 2);
        context.frameCount = static_cast<uint32_t>(count);
        context.sampleRate = static_cast<uint32_t>(rate);
        context.offline = offline;
        effect.processBlock(context);
    }
    return stereo;
}

std::vector<double> impulse(size_t frames, bool left, bool right)
{
    std::vector<double> stereo(frames * 2, 0.0);
    stereo[0] = left ? 1.0 : 0.0;
    stereo[1] = right ? 1.0 : 0.0;
    return stereo;
}

double energy(const std::vector<double> & stereo, size_t channel, size_t from, size_t to)
{
    double sum = 0.0;
    for (size_t n = from; n < std::min(to, stereo.size() / 2); n++) {
        sum += stereo[n * 2 + channel] * stereo[n * 2 + channel];
    }
    return sum;
}

size_t firstArrival(const std::vector<double> & stereo, size_t channel, double threshold)
{
    for (size_t n = 0; n < stereo.size() / 2; n++) {
        if (std::abs(stereo[n * 2 + channel]) > threshold) {
            return n;
        }
    }
    return stereo.size();
}

double distance(const Vec3 & a, const Vec3 & b)
{
    return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z);
}

} // namespace

void RoomMicsTest::test_convolver_shouldMatchDirectConvolution()
{
    std::mt19937 random { 7 };
    std::uniform_real_distribution<double> uniform { -1.0, 1.0 };
    constexpr size_t length = 1000;
    std::array<std::vector<double>, RoomConvolver::KernelCount> kernels;
    std::array<const double *, RoomConvolver::KernelCount> pointers {};
    for (size_t k = 0; k < kernels.size(); k++) {
        kernels[k].resize(length);
        std::ranges::generate(kernels[k], [&] { return uniform(random); });
        pointers[k] = kernels[k].data();
    }
    RoomConvolver convolver;
    convolver.configure(length);
    convolver.startLoad(pointers, length);
    convolver.finishLoad();
    // Live from the next block
    for (size_t n = 0; n < RoomConvolver::BlockSize; n++) {
        double left = 0.0;
        double right = 0.0;
        convolver.process(left, right);
    }
    QVERIFY(convolver.hasKernels());
    convolver.reset();

    constexpr size_t frames = 3000;
    std::vector<std::array<double, 2>> input(frames);
    for (auto && frame : input) {
        frame = { uniform(random), uniform(random) };
    }
    double worst = 0.0;
    for (size_t n = 0; n < frames; n++) {
        double left = input[n][0];
        double right = input[n][1];
        convolver.process(left, right);
        const std::array<double, 2> output { left, right };
        for (size_t o = 0; o < 2; o++) {
            double expected = 0.0;
            for (size_t i = 0; i < 2; i++) {
                const auto & kernel = kernels[RoomConvolver::kernelIndex(i, o)];
                for (size_t k = 0; k < length && k <= n; k++) {
                    expected += kernel[k] * input[n - k][i];
                }
            }
            worst = std::max(worst, std::abs(output[o] - expected));
        }
    }
    QVERIFY2(worst < 1.0e-9, qPrintable(QString::number(worst)));
}

void RoomMicsTest::test_convolver_newResponses_shouldTakeOverWithoutAJump()
{
    // A constant input through a unit kernel, then through half of one: the output steps down
    // over one block rather than at once
    constexpr size_t length = 500;
    std::vector<double> unit(length, 0.0);
    std::vector<double> half(length, 0.0);
    unit[0] = 1.0;
    half[0] = 0.5;
    RoomConvolver convolver;
    convolver.configure(length);
    convolver.startLoad({ unit.data(), unit.data(), unit.data(), unit.data() }, length);
    convolver.finishLoad();
    std::vector<double> outputs;
    for (size_t n = 0; n < RoomConvolver::BlockSize * 4; n++) {
        if (n == RoomConvolver::BlockSize * 2) {
            convolver.startLoad({ half.data(), half.data(), half.data(), half.data() }, length);
            convolver.finishLoad();
        }
        double left = 1.0;
        double right = 1.0;
        convolver.process(left, right);
        outputs.push_back(left);
    }
    // Two inputs into each output: 2.0 before, 1.0 after
    QCOMPARE(outputs[RoomConvolver::BlockSize * 2 - 1], 2.0);
    QCOMPARE(outputs.back(), 1.0);
    double largestStep = 0.0;
    for (size_t n = RoomConvolver::BlockSize + 1; n < outputs.size(); n++) {
        largestStep = std::max(largestStep, std::abs(outputs[n] - outputs[n - 1]));
    }
    QVERIFY2(largestStep < 0.02, qPrintable(QString::number(largestStep)));
}

void RoomMicsTest::test_designer_closeMicOnTheSource_shouldHearItAtUnity()
{
    ImpulseDesigner designer;
    designer.configure(rate);
    RoomSettings settings;
    designer.start(settings);
    designer.finish();
    const auto & response = designer.response(ImpulseDesigner::responseIndex(MicGroup::Close, 0, 0));
    QVERIFY2(std::abs(response[0] - 1.0) < 0.02, qPrintable(QString::number(response[0])));
    // Everything after is the room, well under the source
    double later = 0.0;
    for (size_t n = 1; n < designer.length(); n++) {
        later = std::max(later, std::abs(response[n]));
    }
    QVERIFY2(later < 0.3, qPrintable(QString::number(later)));
}

void RoomMicsTest::test_designer_directSound_shouldArriveAfterTheTimeOfFlight()
{
    ImpulseDesigner designer;
    designer.configure(rate);
    RoomSettings settings;
    settings.groups[static_cast<size_t>(MicGroup::Far)].spacingM = 0.0;
    designer.start(settings);
    designer.finish();
    const auto layout = RoomLayout::from(settings);
    const double expected = distance(layout.sources[0], layout.mics[static_cast<size_t>(MicGroup::Far)][0].position) / SpeedOfSound * rate;
    const auto & response = designer.response(ImpulseDesigner::responseIndex(MicGroup::Far, 0, 0));
    double peak = 0.0;
    for (size_t n = 0; n < designer.length(); n++) {
        peak = std::max(peak, std::abs(response[n]));
    }
    size_t arrival = 0;
    while (std::abs(response[arrival]) < 0.5 * peak) {
        arrival++;
    }
    QVERIFY2(std::abs(static_cast<double>(arrival) - expected) <= 2.0, qPrintable(QString { "%1 vs %2" }.arg(arrival).arg(expected)));
}

void RoomMicsTest::test_designer_figureEight_shouldNotHearItsSide()
{
    // A pair turned side-on to the source by its splay: an omni hears the direct sound, a
    // figure-eight has its null there
    const auto directPeak = [](Pattern pattern) {
        RoomSettings settings;
        settings.sourceSpreadM = 0.0;
        auto && far = settings.groups[static_cast<size_t>(MicGroup::Far)];
        far.spacingM = 0.0;
        far.angleDeg = 90.0;
        far.pattern = pattern;
        ImpulseDesigner designer;
        designer.configure(rate);
        designer.start(settings);
        designer.finish();
        const auto layout = RoomLayout::from(settings);
        const auto arrival = static_cast<size_t>(distance(layout.sources[0], layout.mics[static_cast<size_t>(MicGroup::Far)][0].position) / SpeedOfSound * rate);
        const auto & response = designer.response(ImpulseDesigner::responseIndex(MicGroup::Far, 0, 0));
        double peak = 0.0;
        for (size_t n = arrival - 2; n <= arrival + 3; n++) {
            peak = std::max(peak, std::abs(response[n]));
        }
        return peak;
    };
    const double omni = directPeak(Pattern::Omni);
    const double figureEight = directPeak(Pattern::FigureEight);
    QVERIFY(omni > 0.01);
    QVERIFY2(figureEight < 0.05 * omni, qPrintable(QString { "%1 vs %2" }.arg(figureEight).arg(omni)));
}

void RoomMicsTest::test_designer_slices_shouldMatchOneGo()
{
    RoomSettings settings;
    ImpulseDesigner whole;
    whole.configure(rate);
    whole.start(settings);
    whole.finish();
    ImpulseDesigner sliced;
    sliced.configure(rate);
    sliced.start(settings);
    int steps = 0;
    while (!sliced.advance(500.0)) {
        steps++;
        QVERIFY(steps < 100000);
    }
    QVERIFY(steps > 10);
    QCOMPARE(sliced.length(), whole.length());
    for (size_t r = 0; r < ImpulseDesigner::ResponseCount; r++) {
        for (size_t n = 0; n < whole.length(); n++) {
            QCOMPARE(sliced.response(r)[n], whole.response(r)[n]);
        }
    }
}

void RoomMicsTest::test_reverbTimes_absorbentRoom_shouldBeShorter()
{
    RoomSettings hard;
    hard.walls = Material::Stone;
    hard.floor = Material::Stone;
    hard.ceiling = Material::Stone;
    RoomSettings soft = hard;
    soft.walls = Material::Curtain;
    soft.floor = Material::Carpet;
    const auto hardTimes = reverbTimes(hard);
    const auto softTimes = reverbTimes(soft);
    for (size_t b = 0; b < BandCount; b++) {
        QVERIFY(softTimes[b] < hardTimes[b]);
    }
    // Sabine's figure for the soft room at 1 kHz, which Eyring sits a little under
    const double absorptionArea = 2.0 * (soft.widthM + soft.depthM) * soft.heightM * absorption(Material::Curtain)[3] + soft.widthM * soft.depthM * (absorption(Material::Carpet)[3] + absorption(Material::Stone)[3]);
    const double sabine = 0.161 * volume(soft) / absorptionArea;
    QVERIFY2(softTimes[3] < sabine && softTimes[3] > 0.7 * sabine, qPrintable(QString { "%1 vs %2" }.arg(softTimes[3]).arg(sabine)));
}

void RoomMicsTest::test_layout_closeMics_shouldFollowTheSource()
{
    RoomSettings settings;
    settings.groups[static_cast<size_t>(MicGroup::Close)].x = 0.5;
    const auto before = RoomLayout::from(settings);
    settings.sourceX += 0.1;
    const auto after = RoomLayout::from(settings);
    const auto & mic = static_cast<size_t>(MicGroup::Close);
    for (size_t m = 0; m < 2; m++) {
        QCOMPARE(after.mics[mic][m].position.x - after.sources[m].x, before.mics[mic][m].position.x - before.sources[m].x);
        QVERIFY(std::abs(after.mics[mic][m].position.x - after.sources[m].x - 0.5) < 1.0e-9);
    }
}

void RoomMicsTest::test_layout_pair_shouldKeepTheRightOnTheRight()
{
    // In front of the source facing it, the right mic is on the side of the right speaker
    RoomSettings settings;
    const auto layout = RoomLayout::from(settings);
    for (size_t g = 0; g < MicGroupCount; g++) {
        QVERIFY(layout.mics[g][1].position.x >= layout.mics[g][0].position.x);
    }
    QVERIFY(layout.sources[1].x > layout.sources[0].x);
}

void RoomMicsTest::test_processBlock_closeMics_shouldHaveNoLatency()
{
    auto effect = makeRoomMics();
    soloGroup(*effect, MicGroup::Close);
    const auto output = render(*effect, impulse(4800, true, true));
    size_t loudest = 0;
    for (size_t n = 0; n < output.size() / 2; n++) {
        if (std::abs(output[n * 2]) > std::abs(output[loudest * 2])) {
            loudest = n;
        }
    }
    QCOMPARE(loudest, size_t { 0 });
    QVERIFY2(output[0] > 0.7 && output[0] < 1.2, qPrintable(QString::number(output[0])));
}

void RoomMicsTest::test_processBlock_roomPair_shouldHearTheLeftSourceFirstOnTheLeft()
{
    auto effect = makeRoomMics();
    soloGroup(*effect, MicGroup::Room);
    setParameter(*effect, C::xmlKeyMicSpacing(static_cast<size_t>(MicGroup::Room)), 0.3f);
    const auto output = render(*effect, impulse(9600, true, false));
    double peak = 0.0;
    for (const double sample : output) {
        peak = std::max(peak, std::abs(sample));
    }
    const auto left = firstArrival(output, 0, 0.3 * peak);
    const auto right = firstArrival(output, 1, 0.3 * peak);
    QVERIFY2(left < right, qPrintable(QString { "%1 vs %2" }.arg(left).arg(right)));
}

void RoomMicsTest::test_processBlock_tail_shouldDecayAtTheRoomsReverbTime()
{
    auto effect = makeRoomMics();
    soloGroup(*effect, MicGroup::Room);
    const double expected = midReverbTime(reverbTimes(effect->settings()));
    const auto frames = static_cast<size_t>(expected * 2.0 * rate);
    auto output = render(*effect, impulse(frames, true, true));
    // In the octave about 1 kHz: the treble dies away sooner, and with most of the bandwidth up there
    // it would set the slope of a broadband measurement
    for (size_t channel = 0; channel < 2; channel++) {
        const double w = 2.0 * std::numbers::pi * 1000.0 / rate;
        const double alpha = std::sin(w) / (2.0 * std::numbers::sqrt2);
        const double a0 = 1.0 + alpha;
        double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
        for (size_t n = 0; n < frames; n++) {
            const double x = output[n * 2 + channel];
            const double y = (alpha * x - alpha * x2 - (-2.0 * std::cos(w)) * y1 - (1.0 - alpha) * y2) / a0;
            x2 = x1;
            x1 = x;
            y2 = y1;
            y1 = y;
            output[n * 2 + channel] = y;
        }
    }

    // Schroeder's backward integral, fitted from -5 to -25 dB
    std::vector<double> remaining(frames + 1, 0.0);
    for (size_t n = frames; n-- > 0;) {
        remaining[n] = remaining[n + 1] + output[n * 2] * output[n * 2] + output[n * 2 + 1] * output[n * 2 + 1];
    }
    const auto timeAt = [&remaining, frames](double db) {
        const double target = remaining[0] * std::pow(10.0, db / 10.0);
        size_t n = 0;
        while (n < frames && remaining[n] > target) {
            n++;
        }
        return static_cast<double>(n) / rate;
    };
    const double measured = 3.0 * (timeAt(-25.0) - timeAt(-5.0));
    QVERIFY2(std::abs(measured / expected - 1.0) < 0.2, qPrintable(QString { "%1 s vs %2 s" }.arg(measured).arg(expected)));
}

void RoomMicsTest::test_processBlock_tail_shouldJoinTheEarlyPartWithoutAStep()
{
    auto effect = makeRoomMics();
    soloGroup(*effect, MicGroup::Room);
    ImpulseDesigner designer;
    designer.configure(rate);
    designer.start(effect->settings());
    designer.finish();
    const double window = designer.windowSeconds();
    const double crossover = designer.crossoverSeconds();
    const double reverbTime = midReverbTime(designer.reverbTimes());
    const auto output = render(*effect, impulse(static_cast<size_t>((window + 0.1) * rate), true, true));

    // The energy just before the crossover, early part only, against just after it, tail only,
    // each against what the room's decay says it should be by then
    const double span = 0.015;
    const auto rateIn = [&output](double from, double to) {
        const auto a = static_cast<size_t>(from * rate);
        const auto b = static_cast<size_t>(to * rate);
        return (energy(output, 0, a, b) + energy(output, 1, a, b)) / (to - from);
    };
    const double before = rateIn(window - crossover - span, window - crossover);
    const double after = rateIn(window + 0.002, window + 0.002 + span);
    const double decayDb = 60.0 * (span + crossover + 0.002) / reverbTime;
    const double stepDb = 10.0 * std::log10(after / before) + decayDb;
    QVERIFY2(std::abs(stepDb) < 4.0, qPrintable(QString { "%1 dB" }.arg(stepDb)));
}

void RoomMicsTest::test_processBlock_levels_shouldMatchBetweenNearAndFarPairs()
{
    // Each pair alone at unity: what the left mic hears of the left source is as loud for each
    std::array<double, MicGroupCount> energies {};
    for (size_t g = 0; g < MicGroupCount; g++) {
        auto effect = makeRoomMics();
        soloGroup(*effect, static_cast<MicGroup>(g));
        const double reverbTime = midReverbTime(reverbTimes(effect->settings()));
        const auto output = render(*effect, impulse(static_cast<size_t>(reverbTime * 1.5 * rate), true, false));
        energies[g] = energy(output, 0, 0, output.size() / 2);
    }
    for (size_t g = 0; g < MicGroupCount; g++) {
        const double db = 10.0 * std::log10(energies[g]);
        QVERIFY2(std::abs(db) < 2.0, qPrintable(QString { "pair %1: %2 dB" }.arg(g).arg(db)));
    }
}

void RoomMicsTest::test_processBlock_live_shouldRedesignWithinAFifthOfASecond()
{
    // Playing: a moved mic is redesigned a slice at a time and loaded while the audio runs
    auto live = makeRoomMics();
    render(*live, impulse(4800, true, true), false);
    setParameter(*live, C::xmlKeyMicY(static_cast<size_t>(MicGroup::Room)), 0.8f);
    render(*live, std::vector<double>(static_cast<size_t>(0.2 * rate) * 2, 0.0), false);
    live->reset();
    const auto heard = render(*live, impulse(9600, true, true), false);

    auto fresh = makeRoomMics();
    setParameter(*fresh, C::xmlKeyMicY(static_cast<size_t>(MicGroup::Room)), 0.8f);
    const auto expected = render(*fresh, impulse(9600, true, true));
    double worst = 0.0;
    for (size_t n = 0; n < expected.size(); n++) {
        worst = std::max(worst, std::abs(heard[n] - expected[n]));
    }
    QVERIFY2(worst < 1.0e-9, qPrintable(QString::number(worst)));
}

void RoomMicsTest::test_isSettled_shouldWaitForTheTail()
{
    auto effect = makeRoomMics();
    QVERIFY(effect->isSettled());
    auto output = render(*effect, impulse(4800, true, true));
    QVERIFY(!effect->isSettled());
    // Silence until the tail is gone
    for (int i = 0; i < 100 && !effect->isSettled(); i++) {
        output = render(*effect, std::vector<double>(9600, 0.0));
    }
    QVERIFY(effect->isSettled());
}

void RoomMicsTest::test_factoryPresets_shouldNameRealParametersAndPlay()
{
    auto effect = makeRoomMics();
    const auto & presets = effect->factoryPresets();
    QVERIFY(presets.size() > 5);
    for (size_t i = 0; i < presets.size(); i++) {
        for (auto && [name, value] : presets[i].parameters) {
            QVERIFY2(effect->parameter(name), qPrintable(QString::fromStdString(presets[i].name + ": " + name)));
        }
        QVERIFY(effect->applyFactoryPreset(i));
        effect->sync();
        const auto output = render(*effect, impulse(4800, true, true));
        double peak = 0.0;
        for (const double sample : output) {
            QVERIFY(std::isfinite(sample));
            peak = std::max(peak, std::abs(sample));
        }
        QVERIFY2(peak > 0.01 && peak < 4.0, qPrintable(QString::fromStdString(presets[i].name) + ": " + QString::number(peak)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RoomMicsTest)
