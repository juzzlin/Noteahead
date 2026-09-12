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

#include "eq_8_band_parametric_presets_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/dsp/svf_filter.hpp"
#include "../../domain/effects/eq_8_band_parametric.hpp"
#include "../../domain/effects/eq_8_band_parametric_presets.hpp"
#include "../../domain/effects/gain.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <vector>

namespace noteahead {

namespace {

constexpr size_t BandCount = 8;
constexpr double SampleRate = 48000.0;

//! One band read back out of an effect in the units the source notes are written in. The whole point
//! of the table is that these come back as the hertz and decibels that went in, so the test reads
//! them the way a user would rather than comparing knob positions.
struct Band
{
    SvfFilter::Type type { SvfFilter::Type::Bypass };
    double hz { 0.0 };
    double db { 0.0 };
    double q { 0.0 };
};

double parameterOf(const Eq8BandParametric & eq, const QString & key)
{
    const auto p = eq.parameter(key.toStdString());
    return p ? static_cast<double>(p->get().value()) : 0.0;
}

Band bandOf(const Eq8BandParametric & eq, size_t index)
{
    namespace C = Constants::NahdXml;
    return Band {
        static_cast<SvfFilter::Type>(std::lround(parameterOf(eq, C::xmlKeyBandType(index)))),
        ParameterMapper::mapLogFrequency(parameterOf(eq, C::xmlKeyBandFreq(index)), 20.0, 20000.0),
        -24.0 + parameterOf(eq, C::xmlKeyBandGain(index)) * 48.0,
        ParameterMapper::mapExponential(parameterOf(eq, C::xmlKeyBandQ(index)), 0.1, 10.0)
    };
}

//! Every band of the named preset that is doing something, in the order the preset put them in.
std::vector<Band> activeBandsOf(const QString & presetName)
{
    const auto & presets = Eq8BandParametricPresets::presets();
    const auto it = std::ranges::find_if(presets, [&](const EffectPreset & preset) {
        return preset.name == presetName.toStdString();
    });
    if (it == presets.end()) {
        return {};
    }

    Eq8BandParametric eq;
    eq.setSampleRate(SampleRate);
    eq.applyFactoryPreset(static_cast<size_t>(std::distance(presets.begin(), it)));

    std::vector<Band> bands;
    for (size_t i = 0; i < BandCount; i++) {
        if (const auto band = bandOf(eq, i); band.type != SvfFilter::Type::Bypass) {
            bands.push_back(band);
        }
    }
    return bands;
}

//! Level the effect passes at \a hz, in dB relative to the input. Measured rather than derived, so
//! it covers the filters actually being configured and not merely the parameters being written.
double responseDb(Eq8BandParametric & eq, double hz)
{
    // Long enough for the filters to settle before anything is measured, and an exact number of
    // cycles of the measured tone so the sum is not biased by a partial one.
    const auto cycles = 200;
    const auto samplesPerCycle = SampleRate / hz;
    const auto settle = static_cast<int>(samplesPerCycle * 50);
    const auto measured = static_cast<int>(samplesPerCycle * cycles);

    double inputSquared = 0.0;
    double outputSquared = 0.0;
    for (int i = 0; i < settle + measured; i++) {
        const auto phase = 2.0 * std::numbers::pi * hz * static_cast<double>(i) / SampleRate;
        const auto sample = std::sin(phase);
        auto left = sample;
        auto right = sample;
        eq.process(left, right);
        if (i >= settle) {
            inputSquared += sample * sample;
            outputSquared += left * left;
        }
    }
    if (inputSquared <= 0.0 || outputSquared <= 0.0) {
        return -200.0;
    }
    return 10.0 * std::log10(outputSquared / inputSquared);
}

Eq8BandParametric presetLoaded(const QString & presetName)
{
    const auto & presets = Eq8BandParametricPresets::presets();
    const auto it = std::ranges::find_if(presets, [&](const EffectPreset & preset) {
        return preset.name == presetName.toStdString();
    });
    Eq8BandParametric eq;
    eq.setSampleRate(SampleRate);
    if (it != presets.end()) {
        eq.applyFactoryPreset(static_cast<size_t>(std::distance(presets.begin(), it)));
    }
    return eq;
}

} // namespace

void Eq8BandParametricPresetsTest::test_presets_shouldNotBeEmpty()
{
    QVERIFY(Eq8BandParametricPresets::presets().size() > 1);
}

void Eq8BandParametricPresetsTest::test_presets_names_shouldBeUniqueAndNonEmpty()
{
    std::set<std::string> seen;
    for (const auto & preset : Eq8BandParametricPresets::presets()) {
        QVERIFY(!preset.name.empty());
        QVERIFY2(seen.insert(preset.name).second, preset.name.c_str());
    }
}

void Eq8BandParametricPresetsTest::test_presets_flat_shouldComeFirst()
{
    // Flat is the way back from any other preset, so it is the one entry whose position matters.
    QCOMPARE(Eq8BandParametricPresets::presets().front().name, std::string { "Flat" });
}

void Eq8BandParametricPresetsTest::test_presets_afterFlat_shouldBeAlphabetical()
{
    const auto & presets = Eq8BandParametricPresets::presets();
    for (size_t i = 2; i < presets.size(); i++) {
        QVERIFY2(presets.at(i - 1).name < presets.at(i).name,
                 (presets.at(i - 1).name + " should sort before " + presets.at(i).name).c_str());
    }
}

void Eq8BandParametricPresetsTest::test_presets_shouldNameOnlyKnownParameters()
{
    // A key the effect does not have is applied to nothing and fails silently, so a typo in the
    // table would otherwise only show up as a preset that does less than it says.
    const Eq8BandParametric eq;
    for (const auto & preset : Eq8BandParametricPresets::presets()) {
        for (const auto & [name, value] : preset.parameters) {
            QVERIFY2(eq.parameter(name).has_value(), (preset.name + ": " + name).c_str());
        }
    }
}

void Eq8BandParametricPresetsTest::test_presets_shouldFitTheAvailableBands()
{
    for (const auto & preset : Eq8BandParametricPresets::presets()) {
        const auto bands = activeBandsOf(QString::fromStdString(preset.name));
        QVERIFY2(bands.size() <= BandCount, preset.name.c_str());
    }
}

void Eq8BandParametricPresetsTest::test_presets_bandsInUse_shouldRunLowToHigh()
{
    // The dialog draws band 1 to band 8 left to right, so a preset whose bands ascend in frequency
    // reads across like the curve it makes.
    for (const auto & preset : Eq8BandParametricPresets::presets()) {
        const auto bands = activeBandsOf(QString::fromStdString(preset.name));
        for (size_t i = 1; i < bands.size(); i++) {
            QVERIFY2(bands.at(i).hz >= bands.at(i - 1).hz * 0.999, preset.name.c_str());
        }
    }
}

void Eq8BandParametricPresetsTest::test_presets_gains_shouldStayWithinAFewDecibels()
{
    // These are starting points, and a starting point that is already 12 dB into a decision is not
    // one. The widest move in the source notes is the +6 dB shelf on a kick.
    for (const auto & preset : Eq8BandParametricPresets::presets()) {
        for (const auto & band : activeBandsOf(QString::fromStdString(preset.name))) {
            QVERIFY2(std::abs(band.db) <= 6.0, preset.name.c_str());
        }
    }
}

void Eq8BandParametricPresetsTest::test_flat_shouldBypassEveryBand()
{
    QVERIFY(activeBandsOf("Flat").empty());

    auto eq = presetLoaded("Flat");
    QVERIFY(std::abs(responseDb(eq, 50.0)) < 0.01);
    QVERIFY(std::abs(responseDb(eq, 1000.0)) < 0.01);
    QVERIFY(std::abs(responseDb(eq, 10000.0)) < 0.01);
}

void Eq8BandParametricPresetsTest::test_leadVocal_shouldMatchTheSourceNotes()
{
    // 100 Hz high pass, -3 dB at 300 Hz, +3 dB at 120 Hz, +3 dB at 6 kHz, +3 dB shelf at 12 kHz.
    const auto bands = activeBandsOf("Lead Vocal");
    QCOMPARE(bands.size(), size_t { 5 });

    QCOMPARE(bands.at(0).type, SvfFilter::Type::LowCut);
    QVERIFY(std::abs(bands.at(0).hz - 100.0) < 1.0);

    QCOMPARE(bands.at(1).type, SvfFilter::Type::Bell);
    QVERIFY(std::abs(bands.at(1).hz - 120.0) < 1.0);
    QVERIFY(std::abs(bands.at(1).db - 3.0) < 0.05);

    QCOMPARE(bands.at(2).type, SvfFilter::Type::Bell);
    QVERIFY(std::abs(bands.at(2).hz - 300.0) < 2.0);
    QVERIFY(std::abs(bands.at(2).db + 3.0) < 0.05);

    QCOMPARE(bands.at(3).type, SvfFilter::Type::Bell);
    QVERIFY(std::abs(bands.at(3).hz - 6000.0) < 30.0);
    QVERIFY(std::abs(bands.at(3).db - 3.0) < 0.05);

    QCOMPARE(bands.at(4).type, SvfFilter::Type::HighShelf);
    QVERIFY(std::abs(bands.at(4).hz - 12000.0) < 60.0);
    QVERIFY(std::abs(bands.at(4).db - 3.0) < 0.05);
}

void Eq8BandParametricPresetsTest::test_masterBus_shouldMatchTheSourceNotes()
{
    // 30 Hz high pass, +3 dB at 80 Hz, -3 dB at 500 Hz, and an air shelf at 16 kHz held to +3 dB
    // rather than the +8 dB the notes offer as the top of the range.
    const auto bands = activeBandsOf("Master Bus");
    QCOMPARE(bands.size(), size_t { 4 });

    QCOMPARE(bands.at(0).type, SvfFilter::Type::LowCut);
    QVERIFY(std::abs(bands.at(0).hz - 30.0) < 0.5);
    QVERIFY(std::abs(bands.at(1).hz - 80.0) < 1.0);
    QVERIFY(std::abs(bands.at(1).db - 3.0) < 0.05);
    QVERIFY(std::abs(bands.at(2).hz - 500.0) < 3.0);
    QVERIFY(std::abs(bands.at(2).db + 3.0) < 0.05);

    QCOMPARE(bands.at(3).type, SvfFilter::Type::HighShelf);
    QVERIFY(std::abs(bands.at(3).hz - 16000.0) < 80.0);
    QVERIFY(std::abs(bands.at(3).db - 3.0) < 0.05);
}

void Eq8BandParametricPresetsTest::test_highPass_shouldBe12DbPerOctave()
{
    // One EQ band is a second-order section, so the high pass in every preset is 12 dB/octave, with
    // its corner 3 dB down. Measured rather than derived, since a Q other than 0.707 would dip or
    // peak there instead and nothing in the parameters would say so.
    auto eq = presetLoaded("Hi-Hats & Cymbals");

    const auto atCorner = responseDb(eq, 150.0);
    const auto oneOctaveBelow = responseDb(eq, 75.0);
    const auto twoOctavesBelow = responseDb(eq, 37.5);

    QVERIFY2(std::abs(atCorner + 3.0) < 1.0, qPrintable(QString::number(atCorner)));

    const auto secondOctave = oneOctaveBelow - twoOctavesBelow;
    QVERIFY2(secondOctave > 9.0 && secondOctave < 15.0, qPrintable(QString::number(secondOctave)));
}

void Eq8BandParametricPresetsTest::test_highPass_shouldCostOneBand()
{
    // A 24 dB/octave slope would take two bands at two different Q values, which reads in the dialog
    // as a band duplicated by mistake and leaves less room for the moves a starting point is for.
    for (const auto & preset : Eq8BandParametricPresets::presets()) {
        size_t lowCuts = 0;
        for (const auto & band : activeBandsOf(QString::fromStdString(preset.name))) {
            if (band.type == SvfFilter::Type::LowCut) {
                lowCuts++;
            }
        }
        QVERIFY2(lowCuts <= 1, preset.name.c_str());
    }
}

void Eq8BandParametricPresetsTest::test_oneOctaveBell_shouldUseTheQThatSpansAnOctave()
{
    // "A one-octave bell" is Q of about 1.41, not the 1.0 the phrase invites, and every preset that
    // the notes give without a Q depends on that.
    const auto bands = activeBandsOf("Lead Vocal");
    QVERIFY(std::abs(bands.at(1).q - 1.4142) < 0.02);
    QVERIFY(std::abs(bands.at(2).q - 1.4142) < 0.02);
}

void Eq8BandParametricPresetsTest::test_applyFactoryPreset_shouldReachTheAudio()
{
    // A preset that writes parameters but never reaches the filters would pass every check above.
    auto eq = presetLoaded("Hi-Hats & Cymbals");
    // 50 Hz is a third of the 150 Hz corner, where a second-order high pass is about 19 dB down.
    const auto belowCorner = responseDb(eq, 50.0);
    QVERIFY2(belowCorner < -15.0, qPrintable(QString::number(belowCorner)));
    QVERIFY(responseDb(eq, 9500.0) > 1.0);
}

void Eq8BandParametricPresetsTest::test_applyFactoryPreset_shouldReplaceThePreviousPreset()
{
    // A preset is the whole panel: what the next one does not name has to go back to its default,
    // or the two would pile up.
    const auto & presets = Eq8BandParametricPresets::presets();
    const auto indexOf = [&presets](const std::string & name) {
        return static_cast<size_t>(std::distance(presets.begin(), std::ranges::find_if(presets, [&](const EffectPreset & p) {
                                                     return p.name == name;
                                                 })));
    };

    Eq8BandParametric eq;
    eq.setSampleRate(SampleRate);
    QVERIFY(eq.applyFactoryPreset(indexOf("Lead Vocal")));
    QVERIFY(eq.applyFactoryPreset(indexOf("Strings")));

    // Strings uses two bands; the vocal's further three must not have survived into it.
    size_t active = 0;
    for (size_t i = 0; i < BandCount; i++) {
        if (bandOf(eq, i).type != SvfFilter::Type::Bypass) {
            active++;
        }
    }
    QCOMPARE(active, size_t { 2 });

    QVERIFY2(responseDb(eq, 50.0) > -0.5, "Strings has no high pass, so the vocal's must be gone");
}

void Eq8BandParametricPresetsTest::test_applyFactoryPreset_outOfRange_shouldFail()
{
    Eq8BandParametric eq;
    QVERIFY(!eq.applyFactoryPreset(Eq8BandParametricPresets::presets().size()));
}

void Eq8BandParametricPresetsTest::test_factoryPresets_defaultEffect_shouldOfferNone()
{
    // An effect that ships no patches gets an empty list from the base class rather than having to
    // say so, which is what keeps the preset row out of the dialogs that have nothing to show.
    const Gain gain;
    QVERIFY(gain.factoryPresets().empty());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::Eq8BandParametricPresetsTest)
