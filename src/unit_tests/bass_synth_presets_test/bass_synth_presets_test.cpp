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

#include "bass_synth_presets_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"
#include "../../domain/devices/bass_synth_presets.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <set>
#include <span>
#include <string>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 44100 };
constexpr uint32_t FrameCount { 1024 };

//! Long enough for the filter envelope of even the slowest patch here to have run its course.
constexpr size_t AuditionBlocks { 24 };

//! Level a patch has to reach on one note to count as usable. The instrument is monophonic, so one
//! note is the whole sound rather than a quarter of it.
constexpr double AudibleThreshold { 0.02 };

//! Widest spread the list is allowed to cover, loudest to quietest. A set of patches you can page
//! through without riding the fader.
constexpr double MaxSpreadDecibels { 24.0 };

std::vector<double> render(BassSynthDevice & synth)
{
    std::vector<double> mono;
    for (size_t block = 0; block < AuditionBlocks; block++) {
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

double audition(int index)
{
    BassSynthDevice synth { "Test Bass Synth" };
    synth.setSampleRate(SampleRate);
    synth.loadPreset(index);
    synth.processMidiNoteOn(36, 110);
    return peak(render(synth));
}

std::set<std::string> deviceParameterNames()
{
    const BassSynthDevice synth { "Test Bass Synth" };
    std::set<std::string> names;
    for (auto && [name, parameter] : synth.parameters()) {
        names.insert(name);
    }
    return names;
}

const char * presetName(int index)
{
    return BassSynthPresets::presets().at(static_cast<size_t>(index)).name.c_str();
}

} // namespace

void BassSynthPresetsTest::test_presets_shouldAllBeNamed()
{
    const auto & presets = BassSynthPresets::presets();
    QVERIFY(!presets.empty());

    // Init leads and names nothing, so selecting it is the way back to the default panel.
    QCOMPARE(presets.front().name, std::string { "Init" });
    QVERIFY(presets.front().parameters.empty());

    std::set<std::string> seen;
    for (const auto & preset : presets) {
        QVERIFY(!preset.name.empty());
        QVERIFY2(seen.insert(preset.name).second, preset.name.c_str());
    }
}

void BassSynthPresetsTest::test_presets_afterInit_shouldBeAlphabetical()
{
    const auto & presets = BassSynthPresets::presets();
    for (size_t i = 2; i < presets.size(); i++) {
        QVERIFY2(presets[i - 1].name < presets[i].name, presets[i].name.c_str());
    }
}

void BassSynthPresetsTest::test_presets_everyOne_shouldNameOnlyRealParameters()
{
    // A misspelled key is silent: the patch simply does not set what it meant to.
    const auto known = deviceParameterNames();
    for (const auto & preset : BassSynthPresets::presets()) {
        for (const auto & [name, value] : preset.parameters) {
            QVERIFY2(known.count(name), (preset.name + ": " + name).c_str());
        }
    }
}

void BassSynthPresetsTest::test_presets_everyOne_shouldSound()
{
    for (int index = 0; index < static_cast<int>(BassSynthPresets::presets().size()); index++) {
        QVERIFY2(audition(index) > AudibleThreshold, presetName(index));
    }
}

void BassSynthPresetsTest::test_presets_everyOne_shouldStayWithinFullScale()
{
    // The distortion stage can be driven past full scale, and a patch that clips before the mixer
    // has even seen it leaves nothing to be done about it downstream.
    for (int index = 0; index < static_cast<int>(BassSynthPresets::presets().size()); index++) {
        QVERIFY2(audition(index) <= 1.0, presetName(index));
    }
}

void BassSynthPresetsTest::test_presets_everyOne_shouldStayWithinAUsableLevelRange()
{
    double quietest = 1.0;
    double loudest = 0.0;
    for (int index = 0; index < static_cast<int>(BassSynthPresets::presets().size()); index++) {
        const auto level = audition(index);
        quietest = std::min(quietest, level);
        loudest = std::max(loudest, level);
    }

    QVERIFY(quietest > 0.0);
    const auto spread = 20.0 * std::log10(loudest / quietest);
    QVERIFY2(spread < MaxSpreadDecibels, std::to_string(spread).c_str());
}

void BassSynthPresetsTest::test_loadPreset_shouldReplaceTheWholePanel()
{
    BassSynthDevice synth { "Test Bass Synth" };

    // Something no patch below names, so only the reset a preset does can put it back.
    synth.setSubLevel(0.9f);
    QVERIFY(std::abs(synth.subLevel() - 0.9f) < 1.0e-6f);

    const auto & presets = BassSynthPresets::presets();
    const auto plucked = std::ranges::find_if(presets, [](const auto & preset) { return preset.name == "Plucked"; });
    QVERIFY(plucked != presets.end());
    synth.loadPreset(static_cast<int>(std::distance(presets.begin(), plucked)));

    // Plucked has no sub in it, and a preset is the whole panel rather than the parts it names.
    QVERIFY(synth.subLevel() < 1.0e-6f);
}

void BassSynthPresetsTest::test_loadPreset_outOfRange_shouldChangeNothing()
{
    BassSynthDevice synth { "Test Bass Synth" };
    synth.setSubLevel(0.9f);

    synth.loadPreset(-1);
    QVERIFY(std::abs(synth.subLevel() - 0.9f) < 1.0e-6f);

    synth.loadPreset(static_cast<int>(BassSynthPresets::presets().size()));
    QVERIFY(std::abs(synth.subLevel() - 0.9f) < 1.0e-6f);
}

void BassSynthPresetsTest::test_programChange_shouldNotTouchTheAuthoredPatch()
{
    // A program change in a song moves the panel for as long as it plays, but the patch the user
    // saved has to still be there afterwards -- otherwise playing the song rewrites the project.
    BassSynthDevice synth { "Test Bass Synth" };
    synth.setSubLevel(0.9f);

    synth.processMidiProgramChange(1, 0);
    QVERIFY(synth.subLevel() < 0.9f);

    synth.clearAutomation();
    QVERIFY(std::abs(synth.subLevel() - 0.9f) < 1.0e-6f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BassSynthPresetsTest)
