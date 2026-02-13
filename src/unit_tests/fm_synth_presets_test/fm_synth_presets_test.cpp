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

#include "fm_synth_presets_test.hpp"

#include "../../domain/devices/fm_synth_device.hpp"
#include "../../domain/devices/fm_synth_presets.hpp"

#include <QTest>

#include <functional>
#include <map>
#include <set>
#include <string>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 44100 };
constexpr uint32_t FrameCount { 1024 };

//! Seeds to sweep for the checks that only read the patch, which are cheap.
constexpr uint32_t SeedCount { 200 };

//! Seeds to sweep for the checks that render, which are not.
constexpr uint32_t AuditionSeedCount { 60 };

//! Long enough that a slow attack has arrived before the level is judged: a pad is quiet for its
//! first fraction of a second by design, and measuring only that would call it broken.
constexpr size_t AuditionBlocks { 24 };

//! Level a patch has to reach on a four-note chord to count as audible. Well above silence: a patch
//! twenty-odd dB under the rest of the list is one nobody would pick twice, so this is set to catch
//! that rather than only to catch true silence.
constexpr double AudibleThreshold { 0.02 };

std::vector<double> renderMono(FmSynthDevice & synth, size_t blocks)
{
    std::vector<double> mono;
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

double peak(const std::vector<double> & samples)
{
    double result = 0.0;
    for (const double sample : samples) {
        result = std::max(result, std::abs(sample));
    }
    return result;
}

//! Renders a chord on the given patch, the way a preset would first be met.
std::vector<double> audition(const std::function<void(FmSynthDevice &)> & load)
{
    FmSynthDevice synth { "Test FM" };
    load(synth);
    for (const uint8_t note : { 48, 60, 64, 67 }) {
        synth.processMidiNoteOn(note, 110);
    }
    return renderMono(synth, AuditionBlocks);
}

//! Every parameter name the device actually has.
std::set<std::string> deviceParameterNames()
{
    const FmSynthDevice synth { "Test FM" };
    std::set<std::string> names;
    for (auto && [name, parameter] : synth.parameters()) {
        names.insert(name);
    }
    return names;
}

} // namespace

void FmSynthPresetsTest::test_presets_shouldAllBeNamed()
{
    const auto & presets = FmSynthPresets::presets();
    QVERIFY(!presets.empty());
    QCOMPARE(presets.front().name, std::string { "Init" });
    QVERIFY(presets.front().parameters.empty());

    std::set<std::string> names;
    for (auto && preset : presets) {
        QVERIFY(!preset.name.empty());
        QVERIFY(names.insert(preset.name).second);
    }
}

void FmSynthPresetsTest::test_presets_everyOne_shouldNameOnlyRealParameters()
{
    // A preset writes by name, and a name the device does not have is dropped in silence. That is
    // the one mistake in a table like this that nothing else would ever surface: the patch simply
    // loads without the parameter, sounding almost but not quite right.
    const auto known = deviceParameterNames();
    for (auto && preset : FmSynthPresets::presets()) {
        for (auto && [name, value] : preset.parameters) {
            QVERIFY2(known.count(name), (preset.name + ": " + name).c_str());
        }
    }
}

void FmSynthPresetsTest::test_presets_everyOne_shouldSound()
{
    for (int i = 0; i < static_cast<int>(FmSynthPresets::presets().size()); i++) {
        const auto samples = audition([i](FmSynthDevice & synth) { synth.loadPreset(i); });
        QVERIFY2(peak(samples) > AudibleThreshold, FmSynthPresets::presets().at(static_cast<size_t>(i)).name.c_str());
    }
}

void FmSynthPresetsTest::test_presets_everyOne_shouldStayWithinFullScale()
{
    // A four-note chord is what a preset meets first. A patch that clips there is not usable, and
    // the headroom is the device's to keep whatever the operator levels are set to.
    for (int i = 0; i < static_cast<int>(FmSynthPresets::presets().size()); i++) {
        const auto samples = audition([i](FmSynthDevice & synth) { synth.loadPreset(i); });
        QVERIFY2(peak(samples) <= 1.0, FmSynthPresets::presets().at(static_cast<size_t>(i)).name.c_str());
    }
}

void FmSynthPresetsTest::test_preset_shouldReplaceTheWholePanel()
{
    FmSynthDevice synth { "Test FM" };

    // Something the target preset says nothing about, which therefore has to be put back rather
    // than carried over.
    synth.setPanSpread(0.9f);
    synth.setFeedback(0.8f);
    synth.setAlgorithm(6);

    synth.loadPreset(0); // Init

    QCOMPARE(synth.feedback(), 0.0f);
    QCOMPARE(synth.algorithm(), 0);
    QCOMPARE(synth.panSpread(), 0.5f);
}

void FmSynthPresetsTest::test_preset_outOfRange_shouldChangeNothing()
{
    FmSynthDevice synth { "Test FM" };
    synth.setAlgorithm(3);

    synth.loadPreset(-1);
    QCOMPARE(synth.algorithm(), 3);

    synth.loadPreset(static_cast<int>(FmSynthPresets::presets().size()));
    QCOMPARE(synth.algorithm(), 3);
}

void FmSynthPresetsTest::test_randomPatch_shouldNameOnlyRealParameters()
{
    const auto known = deviceParameterNames();
    for (uint32_t seed = 0; seed < SeedCount; seed++) {
        for (auto && [name, value] : FmSynthPresets::randomPatch(seed).parameters) {
            QVERIFY2(known.count(name), name.c_str());
        }
    }
}

void FmSynthPresetsTest::test_randomPatch_sameSeed_shouldGiveTheSamePatch()
{
    // The seed is the patch. It is what lets a test ask for a particular random patch, and what
    // would let one be written down and found again.
    QCOMPARE(FmSynthPresets::randomPatch(1234).parameters, FmSynthPresets::randomPatch(1234).parameters);
}

void FmSynthPresetsTest::test_randomPatch_differentSeeds_shouldGiveDifferentPatches()
{
    std::set<std::map<std::string, float>> seen;
    for (uint32_t seed = 0; seed < SeedCount; seed++) {
        seen.insert(FmSynthPresets::randomPatch(seed).parameters);
    }
    // Pressing the button twice must not hand back the same sound.
    QCOMPARE(seen.size(), static_cast<size_t>(SeedCount));
}

void FmSynthPresetsTest::test_randomPatch_everySeed_shouldSound()
{
    // The one promise the button has to keep. A random patch may be strange, but a silent one is
    // indistinguishable from a broken button, and the user has no way to tell which they got.
    for (uint32_t seed = 0; seed < AuditionSeedCount; seed++) {
        const auto samples = audition([seed](FmSynthDevice & synth) { synth.loadRandomPatch(seed); });
        QVERIFY2(peak(samples) > AudibleThreshold, std::to_string(seed).c_str());
    }
}

void FmSynthPresetsTest::test_randomPatch_everySeed_shouldStayWithinFullScale()
{
    for (uint32_t seed = 0; seed < AuditionSeedCount; seed++) {
        const auto samples = audition([seed](FmSynthDevice & synth) { synth.loadRandomPatch(seed); });
        QVERIFY2(peak(samples) <= 1.0, std::to_string(seed).c_str());
    }
}

void FmSynthPresetsTest::test_randomPatch_shouldAlwaysKeepOperatorOneAudible()
{
    // Operator 1 carries every algorithm, so holding it at full level is what makes "it always
    // sounds" true rather than merely likely.
    for (uint32_t seed = 0; seed < SeedCount; seed++) {
        const auto patch = FmSynthPresets::randomPatch(seed).parameters;
        const auto level = patch.find("op1Level");
        QVERIFY(level != patch.end());
        QCOMPARE(level->second, 1.0f);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FmSynthPresetsTest)
