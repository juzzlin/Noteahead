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

#include "preset_service_test.hpp"

#include "../../application/service/preset_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/synth_device.hpp"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>
#include <string>

namespace noteahead {

namespace {

const auto TypeId = QString::fromStdString(SynthDevice::typeIdString());

std::string vco1Pitch()
{
    return Constants::NahdXml::xmlKeyVco1Pitch().toStdString();
}

std::string lpfCutoff()
{
    return Constants::NahdXml::xmlKeyLpfCutoff().toStdString();
}

float parameterValue(const SynthDevice & device, const std::string & name)
{
    const auto parameter = device.parameter(name);
    return parameter ? parameter->get().value() : -1.0f;
}

void setParameterValue(SynthDevice & device, const std::string & name, float value)
{
    if (const auto parameter = device.parameter(name); parameter) {
        parameter->get().setValue(value);
    }
}

} // namespace

void PresetServiceTest::test_saveUserPreset_shouldRoundTripParameters()
{
    QTemporaryDir root;
    PresetService service { root.path() };

    SynthDevice source { "Source" };
    setParameterValue(source, vco1Pitch(), 0.75f);
    setParameterValue(source, lpfCutoff(), 0.25f);

    QVERIFY(service.saveUserPreset(TypeId, "My Lead", source));

    SynthDevice target { "Target" };
    QVERIFY(service.applyUserPreset(TypeId, "My Lead", target));

    // The XML carries integer positions rather than the float itself, so the round trip is exact
    // only to the resolution of the parameter's own range
    QVERIFY(std::abs(parameterValue(target, vco1Pitch()) - 0.75f) < 0.001f);
    QVERIFY(std::abs(parameterValue(target, lpfCutoff()) - 0.25f) < 0.001f);
}

void PresetServiceTest::test_saveUserPreset_shouldNotifyOfTheChange()
{
    QTemporaryDir root;
    PresetService service { root.path() };
    SynthDevice device { "Device" };

    QSignalSpy spy { &service, &PresetService::userPresetsChanged };

    QVERIFY(service.saveUserPreset(TypeId, "My Lead", device));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), TypeId);
}

void PresetServiceTest::test_saveUserPreset_emptyName_shouldSaveNothing()
{
    QTemporaryDir root;
    PresetService service { root.path() };
    SynthDevice device { "Device" };

    QVERIFY(!service.saveUserPreset(TypeId, "", device));
    QVERIFY(service.userPresetNames(TypeId).isEmpty());
}

void PresetServiceTest::test_saveUserPreset_sameName_shouldReplaceTheStoredPreset()
{
    QTemporaryDir root;
    PresetService service { root.path() };

    SynthDevice source { "Source" };
    setParameterValue(source, vco1Pitch(), 0.75f);
    QVERIFY(service.saveUserPreset(TypeId, "My Lead", source));

    setParameterValue(source, vco1Pitch(), 0.25f);
    QVERIFY(service.saveUserPreset(TypeId, "My Lead", source));

    // Saving twice under one name leaves one preset holding what was saved last, not two
    QCOMPARE(service.userPresetNames(TypeId).size(), 1);

    SynthDevice target { "Target" };
    QVERIFY(service.applyUserPreset(TypeId, "My Lead", target));
    QVERIFY(std::abs(parameterValue(target, vco1Pitch()) - 0.25f) < 0.001f);
}

void PresetServiceTest::test_saveUserPreset_nameWithPathCharacters_shouldKeepTheNameAsGiven()
{
    QTemporaryDir root;
    PresetService service { root.path() };
    SynthDevice device { "Device" };

    // The file name has to give way to what a file system accepts. The name the user typed does
    // not: it is stored inside the file and is what the dropdown shows.
    const QString name { "Lead / Bass: take #2" };
    QVERIFY(service.saveUserPreset(TypeId, name, device));

    QCOMPARE(service.userPresetNames(TypeId), QStringList { name });
    QVERIFY(service.userPresetExists(TypeId, name));
}

void PresetServiceTest::test_userPresetNames_shouldBeSortedCaseInsensitively()
{
    QTemporaryDir root;
    PresetService service { root.path() };
    SynthDevice device { "Device" };

    QVERIFY(service.saveUserPreset(TypeId, "zither", device));
    QVERIFY(service.saveUserPreset(TypeId, "Bass", device));
    QVERIFY(service.saveUserPreset(TypeId, "apex", device));

    QCOMPARE(service.userPresetNames(TypeId), (QStringList { "apex", "Bass", "zither" }));
}

void PresetServiceTest::test_userPresetNames_otherType_shouldNotBeListed()
{
    QTemporaryDir root;
    PresetService service { root.path() };
    SynthDevice device { "Device" };

    QVERIFY(service.saveUserPreset(TypeId, "My Lead", device));
    QVERIFY(service.saveUserPreset("someOtherType", "Not Mine", device));

    QCOMPARE(service.userPresetNames(TypeId), QStringList { "My Lead" });
    QCOMPARE(service.userPresetNames("someOtherType"), QStringList { "Not Mine" });
}

void PresetServiceTest::test_applyUserPreset_shouldDefaultWhatThePresetDoesNotName()
{
    QTemporaryDir root;
    PresetService service { root.path() };

    SynthDevice source { "Source" };
    setParameterValue(source, vco1Pitch(), 0.75f);
    QVERIFY(service.saveUserPreset(TypeId, "My Lead", source));

    SynthDevice target { "Target" };
    const auto defaultCutoff = target.parameter(lpfCutoff())->get().defaultValue();
    setParameterValue(target, lpfCutoff(), 0.1f);

    QVERIFY(service.applyUserPreset(TypeId, "My Lead", target));

    // A preset is the whole panel: what the target had dialled in before is gone, not merged
    QVERIFY(std::abs(parameterValue(target, lpfCutoff()) - defaultCutoff) < 0.001f);
}

void PresetServiceTest::test_applyUserPreset_otherType_shouldChangeNothing()
{
    QTemporaryDir root;
    PresetService service { root.path() };

    SynthDevice source { "Source" };
    setParameterValue(source, vco1Pitch(), 0.75f);
    QVERIFY(service.saveUserPreset("someOtherType", "Not Mine", source));

    // The file sits where this type's presets live, but it says it is someone else's
    const QDir ownDirectory { service.presetDirectory(TypeId) };
    QVERIFY(ownDirectory.mkpath("."));
    const auto otherPath = QDir { service.presetDirectory("someOtherType") }.filePath("Not Mine" + Constants::presetFileExtension());
    QVERIFY(QFile::copy(otherPath, ownDirectory.filePath("Not Mine" + Constants::presetFileExtension())));

    SynthDevice target { "Target" };
    setParameterValue(target, vco1Pitch(), 0.1f);

    QVERIFY(!service.applyUserPreset(TypeId, "Not Mine", target));
    QVERIFY(service.userPresetNames(TypeId).isEmpty());
    QVERIFY(std::abs(parameterValue(target, vco1Pitch()) - 0.1f) < 0.001f);
}

void PresetServiceTest::test_applyUserPreset_missing_shouldFailWithoutThrowing()
{
    QTemporaryDir root;
    PresetService service { root.path() };
    SynthDevice device { "Device" };

    QVERIFY(!service.applyUserPreset(TypeId, "Nothing Here", device));
}

void PresetServiceTest::test_applyUserPreset_garbageFile_shouldFailWithoutThrowing()
{
    QTemporaryDir root;
    PresetService service { root.path() };

    const QDir directory { service.presetDirectory(TypeId) };
    QVERIFY(directory.mkpath("."));
    QFile file { directory.filePath("Garbage" + Constants::presetFileExtension()) };
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not xml at all");
    file.close();

    SynthDevice device { "Device" };
    QVERIFY(!service.applyUserPreset(TypeId, "Garbage", device));
    QVERIFY(service.userPresetNames(TypeId).isEmpty());
}

void PresetServiceTest::test_deleteUserPreset_shouldRemoveItFromTheListing()
{
    QTemporaryDir root;
    PresetService service { root.path() };
    SynthDevice device { "Device" };

    QVERIFY(service.saveUserPreset(TypeId, "My Lead", device));
    QVERIFY(service.saveUserPreset(TypeId, "My Pad", device));

    QSignalSpy spy { &service, &PresetService::userPresetsChanged };

    QVERIFY(service.deleteUserPreset(TypeId, "My Lead"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(service.userPresetNames(TypeId), QStringList { "My Pad" });
    QVERIFY(!service.userPresetExists(TypeId, "My Lead"));
}

void PresetServiceTest::test_deleteUserPreset_missing_shouldFail()
{
    QTemporaryDir root;
    PresetService service { root.path() };

    QVERIFY(!service.deleteUserPreset(TypeId, "Nothing Here"));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PresetServiceTest)
