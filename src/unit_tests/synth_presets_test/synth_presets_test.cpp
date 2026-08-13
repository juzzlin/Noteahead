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

#include "synth_presets_test.hpp"

#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/synth_presets.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <span>
#include <string>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 44100 };
constexpr uint32_t FrameCount { 1024 };

//! Long enough that even a slow attack has arrived before the level is judged.
constexpr size_t AuditionBlocks { 24 };

//! Level a preset has to reach on a four-note chord to count as usable. The list this replaced had
//! a quarter of its entries below this, seven of them silent outright, because nothing ever
//! rendered them.
constexpr double AudibleThreshold { 0.02 };

//! The same for a single note, which is quieter than four by nature -- a pad played one note at a
//! time sits some ten dB below a lead, and that is the patch doing its job rather than a fault.
//! Still high enough to have caught all but one of the old list's failures, which ran from -43 dB
//! down to -95 dB on a single note.
constexpr double SingleNoteThreshold { 0.01 };

//! Widest spread the list is allowed to cover, loudest to quietest. A browsable set of presets is
//! one you can page through without riding the fader; the old list spanned 72 dB.
constexpr double MaxSpreadDecibels { 24.0 };

std::vector<double> render(SynthDevice & synth)
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

double auditionChord(int index)
{
    SynthDevice synth { "Test Synth" };
    synth.loadPreset(index);
    for (const uint8_t note : { 48, 60, 64, 67 }) {
        synth.processMidiNoteOn(note, 110);
    }
    return peak(render(synth));
}

double auditionSingleNote(int index)
{
    SynthDevice synth { "Test Synth" };
    synth.loadPreset(index);
    synth.processMidiNoteOn(60, 110);
    return peak(render(synth));
}

std::set<std::string> deviceParameterNames()
{
    const SynthDevice synth { "Test Synth" };
    std::set<std::string> names;
    for (auto && [name, parameter] : synth.parameters()) {
        names.insert(name);
    }
    return names;
}

const char * presetName(int index)
{
    return SynthPresets::presets().at(static_cast<size_t>(index)).name.c_str();
}

} // namespace

void SynthPresetsTest::test_presets_shouldAllBeNamed()
{
    const auto & presets = SynthPresets::presets();
    QVERIFY(!presets.empty());
    QCOMPARE(presets.front().name, std::string { "Init" });
    QVERIFY(presets.front().parameters.empty());

    std::set<std::string> names;
    for (auto && preset : presets) {
        QVERIFY(!preset.name.empty());
        QVERIFY2(names.insert(preset.name).second, preset.name.c_str());
    }
}

void SynthPresetsTest::test_presets_everyOne_shouldNameOnlyRealParameters()
{
    const auto known = deviceParameterNames();
    for (auto && preset : SynthPresets::presets()) {
        for (auto && [name, value] : preset.parameters) {
            QVERIFY2(known.count(name), (preset.name + ": " + name).c_str());
        }
    }
}

void SynthPresetsTest::test_presets_everyOne_shouldSound()
{
    for (int i = 0; i < static_cast<int>(SynthPresets::presets().size()); i++) {
        QVERIFY2(auditionChord(i) > AudibleThreshold, presetName(i));
    }
}

void SynthPresetsTest::test_presets_everyOne_shouldSoundOnASingleNoteToo()
{
    // A chord can mask a patch that is only audible because four notes are stacked on it. The old
    // list's failures were quieter on one note than on four, so both are checked.
    for (int i = 0; i < static_cast<int>(SynthPresets::presets().size()); i++) {
        QVERIFY2(auditionSingleNote(i) > SingleNoteThreshold, presetName(i));
    }
}

void SynthPresetsTest::test_presets_everyOne_shouldStayWithinFullScale()
{
    for (int i = 0; i < static_cast<int>(SynthPresets::presets().size()); i++) {
        QVERIFY2(auditionChord(i) <= 1.0, presetName(i));
    }
}

void SynthPresetsTest::test_presets_everyOne_shouldStayWithinAUsableLevelRange()
{
    double quietest = 1.0;
    double loudest = 0.0;
    for (int i = 0; i < static_cast<int>(SynthPresets::presets().size()); i++) {
        const double level = auditionChord(i);
        quietest = std::min(quietest, level);
        loudest = std::max(loudest, level);
    }

    const double spread = 20.0 * std::log10(loudest / quietest);
    QVERIFY2(spread < MaxSpreadDecibels, std::to_string(spread).c_str());
}

void SynthPresetsTest::test_presets_namingAFilterSweep_shouldAimItAtTheFilter()
{
    // Every modulation destination is an ordinal, and an ordinal written by guesswork lands
    // somewhere plausible-looking and silently wrong. Eight presets in the old list were named for
    // a filter movement while aiming at an oscillator's pitch or at pulse width -- in one case at
    // an oscillator that the same preset left turned down, so it did nothing at all.
    constexpr float modCutoff = 3.0f;
    constexpr float lfoCutoff = 2.0f;

    for (auto && preset : SynthPresets::presets()) {
        const bool sweepsWithEnvelope = preset.parameters.count("modIntensity") && preset.parameters.count("modTarget")
          && preset.parameters.at("modTarget") == modCutoff;
        const bool sweepsWithLfo = preset.parameters.count("lfoIntensity") && preset.parameters.count("lfoTarget")
          && preset.parameters.at("lfoTarget") == lfoCutoff;

        for (const auto * named : { "Acid", "Sweep", "Wobble", "Pluck" }) {
            if (preset.name.find(named) != std::string::npos) {
                QVERIFY2(sweepsWithEnvelope || sweepsWithLfo, preset.name.c_str());
            }
        }
    }
}

void SynthPresetsTest::test_preset_shouldReplaceTheWholePanel()
{
    SynthDevice synth { "Test Synth" };

    synth.setPanSpread(0.9f);
    synth.setLpfResonance(0.8f);
    synth.setPortamento(0.7f);

    synth.loadPreset(0); // Init

    QCOMPARE(synth.lpfResonance(), 0.0f);
    QCOMPARE(synth.portamento(), 0.0f);
    QCOMPARE(synth.panSpread(), 0.0f);
}

void SynthPresetsTest::test_preset_outOfRange_shouldChangeNothing()
{
    SynthDevice synth { "Test Synth" };
    synth.setLpfResonance(0.6f);

    synth.loadPreset(-1);
    QCOMPARE(synth.lpfResonance(), 0.6f);

    synth.loadPreset(static_cast<int>(SynthPresets::presets().size()));
    QCOMPARE(synth.lpfResonance(), 0.6f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SynthPresetsTest)
