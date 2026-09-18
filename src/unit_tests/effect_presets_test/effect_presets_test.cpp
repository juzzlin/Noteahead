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

#include "effect_presets_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/effects/auto_ducker.hpp"
#include "../../domain/effects/auto_ducker_presets.hpp"
#include "../../domain/effects/delay.hpp"
#include "../../domain/effects/delay_presets.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QBuffer>
#include <QTest>

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

//! Every name a preset is allowed to set, so a typo in a key is caught here rather than by the patch
//! quietly doing nothing.
std::set<std::string> parameterNamesOf(const Effect & effect)
{
    std::set<std::string> names;
    for (const auto & [name, parameter] : effect.parameters()) {
        names.insert(name);
    }
    return names;
}

//! A preset's value for one parameter, or nothing when the patch leaves it alone.
std::optional<float> valueOf(const EffectPreset & preset, const QString & key)
{
    if (const auto it = preset.parameters.find(key.toStdString()); it != preset.parameters.end()) {
        return it->second;
    }
    return std::nullopt;
}

//! Applies a preset by name and hands back the effect it was applied to.
template<typename EffectType>
EffectType applied(const EffectPresetList & presets, const std::string & name)
{
    EffectType effect;
    const auto it = std::ranges::find_if(presets, [&name](const auto & preset) { return preset.name == name; });
    Q_ASSERT(it != presets.end());
    effect.applyFactoryPreset(static_cast<size_t>(std::distance(presets.begin(), it)));
    return effect;
}

double parameterOf(const Effect & effect, const QString & key)
{
    const auto p = effect.parameter(key.toStdString());
    return p ? static_cast<double>(p->get().value()) : 0.0;
}

void checkNamesUniqueAndAlphabetical(const EffectPresetList & presets)
{
    QVERIFY(!presets.empty());

    std::set<std::string> seen;
    for (const auto & preset : presets) {
        QVERIFY(!preset.name.empty());
        QVERIFY2(seen.insert(preset.name).second, preset.name.c_str());
    }

    // A list is read by name, which only works if the names are in an order a reader can predict.
    for (size_t i = 1; i < presets.size(); i++) {
        QVERIFY2(presets[i - 1].name < presets[i].name, presets[i].name.c_str());
    }
}

void checkNamesOnlyKnownParameters(const EffectPresetList & presets, const Effect & effect)
{
    const auto known = parameterNamesOf(effect);
    for (const auto & preset : presets) {
        for (const auto & [name, value] : preset.parameters) {
            QVERIFY2(known.count(name), (preset.name + ": " + name).c_str());
        }
    }
}

} // namespace

void EffectPresetsTest::test_delayPresets_names_shouldBeUniqueAndAlphabetical()
{
    checkNamesUniqueAndAlphabetical(DelayPresets::presets());
}

void EffectPresetsTest::test_delayPresets_shouldNameOnlyKnownParameters()
{
    checkNamesOnlyKnownParameters(DelayPresets::presets(), Delay {});
}

void EffectPresetsTest::test_delayPresets_shouldAllSetAMixAboveZero()
{
    // The effect's own default is fully dry, and a preset resets everything it does not name: one
    // that forgot the mix would load as a delay that cannot be heard at all.
    for (const auto & preset : DelayPresets::presets()) {
        const auto mix = valueOf(preset, C::xmlKeyDelayMix());
        QVERIFY2(mix.has_value(), preset.name.c_str());
        QVERIFY2(mix.value() > 0.0f, preset.name.c_str());
    }
}

void EffectPresetsTest::test_delayPresets_syncedDivisions_shouldLandOnNoteValues()
{
    // The sync slider steps through a fixed set of divisions. A patch that named anything else would
    // snap to the nearest one the moment the slider was touched, so the table has to use them.
    const std::vector<double> divisions { 1.0, 0.75, 0.5, 0.375, 1.0 / 3.0, 0.25, 0.1875, 1.0 / 6.0,
                                          0.125, 0.09375, 1.0 / 12.0, 0.0625, 0.046875, 1.0 / 24.0, 0.03125, 0.015625 };

    for (const auto & preset : DelayPresets::presets()) {
        const auto sync = valueOf(preset, C::xmlKeyDelaySync());
        QVERIFY2(sync.has_value(), preset.name.c_str());
        if (sync.value() < 0.5f) {
            continue;
        }
        const auto division = valueOf(preset, C::xmlKeyDelaySyncDivision());
        QVERIFY2(division.has_value(), preset.name.c_str());
        const bool onTheGrid = std::ranges::any_of(divisions, [&division](double candidate) {
            return std::abs(candidate - static_cast<double>(division.value())) < 1.0e-6;
        });
        QVERIFY2(onTheGrid, preset.name.c_str());
    }
}

void EffectPresetsTest::test_delayPresets_dottedEighth_shouldMatchTheSourceNotes()
{
    const auto delay = applied<Delay>(DelayPresets::presets(), "Dotted Eighth");

    // Three sixteenths of a whole note, which at 120 BPM is 375 ms.
    QVERIFY(parameterOf(delay, C::xmlKeyDelaySync()) > 0.5);
    QVERIFY(std::abs(parameterOf(delay, C::xmlKeyDelaySyncDivision()) - 0.1875) < 1.0e-6);
    QCOMPARE(delay.parameter(C::xmlKeyDelayType().toStdString())->get().xmlValue(), static_cast<int>(Delay::Type::Stereo));
    QVERIFY(std::abs(parameterOf(delay, C::xmlKeyDelayFeedback()) - 0.40) < 1.0e-6);
    QVERIFY(std::abs(parameterOf(delay, C::xmlKeyDelayMix()) - 0.30) < 1.0e-6);
}

void EffectPresetsTest::test_delayPresets_slapback_shouldMatchTheSourceNotes()
{
    const auto delay = applied<Delay>(DelayPresets::presets(), "Slapback");

    // 110 ms, free-running, and in the middle: the line is mono, so the repeat sits where the
    // source does. The parameter is the fraction it takes of the line's ten seconds.
    QVERIFY(parameterOf(delay, C::xmlKeyDelaySync()) < 0.5);
    QVERIFY(std::abs(parameterOf(delay, C::xmlKeyDelayTime()) * 10.0 - 0.110) < 1.0e-6);
    QCOMPARE(delay.parameter(C::xmlKeyDelayType().toStdString())->get().xmlValue(), static_cast<int>(Delay::Type::Mono));
    QVERIFY(parameterOf(delay, C::xmlKeyDelayFeedback()) < 0.1);
}

void EffectPresetsTest::test_autoDuckerPresets_names_shouldBeUniqueAndAlphabetical()
{
    checkNamesUniqueAndAlphabetical(AutoDuckerPresets::presets());
}

void EffectPresetsTest::test_autoDuckerPresets_shouldNameOnlyKnownParameters()
{
    checkNamesOnlyKnownParameters(AutoDuckerPresets::presets(), AutoDucker {});
}

void EffectPresetsTest::test_autoDuckerPresets_shouldAllDuckRatherThanBoost()
{
    // The control runs either way about a transparent centre, and every patch here is a duck. One
    // that landed in the upper half would raise the track under the kick instead of lowering it.
    for (const auto & preset : AutoDuckerPresets::presets()) {
        const auto amount = valueOf(preset, C::xmlKeyAmount());
        QVERIFY2(amount.has_value(), preset.name.c_str());
        QVERIFY2(amount.value() < 0.5f, preset.name.c_str());
    }
}

void EffectPresetsTest::test_autoDuckerPresets_classicPump_shouldMatchTheSourceNotes()
{
    const auto ducker = applied<AutoDucker>(AutoDuckerPresets::presets(), "Classic Pump");

    // -20 dB threshold, 9 dB of duck, a fast attack and a release that lets go over a quarter note.
    QVERIFY(std::abs(-60.0 + parameterOf(ducker, C::xmlKeyThreshold()) * 60.0 + 20.0) < 0.01);
    QVERIFY(std::abs(-24.0 + parameterOf(ducker, C::xmlKeyAmount()) * 48.0 + 9.0) < 0.01);
    QVERIFY(std::abs(ParameterMapper::mapExponential(parameterOf(ducker, C::xmlKeyAttack()), 0.1, 500.0) - 1.0) < 0.01);
    QVERIFY(std::abs(ParameterMapper::mapExponential(parameterOf(ducker, C::xmlKeyRelease()), 1.0, 2000.0) - 180.0) < 1.0);
}

void EffectPresetsTest::test_autoDuckerPresets_shouldNeverNameASideChainSource()
{
    // A device slot means nothing outside the project it was saved in, so no patch may carry one.
    for (const auto & preset : AutoDuckerPresets::presets()) {
        QVERIFY2(!valueOf(preset, C::xmlKeySideChainSourceDevice()).has_value(), preset.name.c_str());
    }
}

void EffectPresetsTest::test_applyFactoryPreset_shouldKeepTheSideChainSource()
{
    AutoDucker ducker;
    if (const auto p = ducker.parameter(C::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(3.0f);
    }
    ducker.sync();
    QCOMPARE(ducker.sidechainSourceDeviceIndex().value(), size_t { 3 });

    QVERIFY(ducker.applyFactoryPreset(0));

    // The patch is a sound; the source is the wiring, and loading a patch may not pull the plug.
    QCOMPARE(ducker.sidechainSourceDeviceIndex().value(), size_t { 3 });
}

void EffectPresetsTest::test_applyPresetParametersFromXml_shouldKeepTheSideChainSource()
{
    // A preset saved with device 1 as its source, the way a user preset comes off disk.
    AutoDucker saved;
    if (const auto p = saved.parameter(C::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(1.0f);
    }
    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    writer.writeStartDocument();
    writer.writeStartElement(C::xmlKeyParameters());
    saved.serializeParametersToXml(writer);
    writer.writeEndElement();
    writer.writeEndDocument();
    buffer.close();

    AutoDucker ducker;
    if (const auto p = ducker.parameter(C::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(5.0f);
    }
    ducker.sync();

    buffer.open(QIODevice::ReadOnly);
    NahdXmlReader reader { buffer };
    while (reader.readNextStartElement() && reader.name() != C::xmlKeyParameters()) {
    }
    ducker.applyPresetParametersFromXml(reader);
    buffer.close();

    // The slot the preset was saved against is meaningless here: this project's wiring stands.
    QCOMPARE(ducker.sidechainSourceDeviceIndex().value(), size_t { 5 });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectPresetsTest)
