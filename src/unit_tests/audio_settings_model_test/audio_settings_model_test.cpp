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

#include "audio_settings_model_test.hpp"

#include "../../application/models/audio_settings_model.hpp"
#include "../../application/service/audio_service.hpp"
#include "../../application/service/jack_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include <vector>

namespace noteahead {

namespace {

//! Settings of this test's own, since the model reads and writes the persisted device choice.
QTemporaryDir & settingsDirectory()
{
    static QTemporaryDir directory;
    return directory;
}

//! An AudioService that records what it was told to use instead of opening anything.
//!
//! autoInitialize is off, so there is no worker and no stream: what is left is the choice being
//! handed down, which is the whole subject here.
class RecordingAudioService : public AudioService
{
public:
    RecordingAudioService(SettingsServiceS settingsService, AudioEngineS audioEngine)
      : AudioService { settingsService, std::make_shared<JackService>(settingsService, audioEngine), audioEngine, nullptr, false }
    {
    }

    void setInputDevice(int deviceId) override
    {
        inputDeviceIds.push_back(deviceId);
    }

    void setOutputDevice(int deviceId) override
    {
        outputDeviceIds.push_back(deviceId);
    }

    //! Every device this service was told to use, in order.
    std::vector<int> inputDeviceIds;
    std::vector<int> outputDeviceIds;
};

} // namespace

void AudioSettingsModelTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("AudioSettingsModelTest");
    QVERIFY(settingsDirectory().isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory().path());
}

void AudioSettingsModelTest::test_construction_shouldApplyTheSavedDevices()
{
    const auto settingsService = std::make_shared<SettingsService>();
    settingsService->setAudioInputDeviceId(7);
    settingsService->setAudioOutputDeviceId(9);

    const auto audioService = std::make_shared<RecordingAudioService>(settingsService, std::make_shared<AudioEngine>());
    AudioSettingsModel model { audioService, settingsService };

    QCOMPARE(model.selectedInputDeviceId(), 7);
    QCOMPARE(model.selectedOutputDeviceId(), 9);
    QCOMPARE(audioService->inputDeviceIds, std::vector<int> { 7 });
    QCOMPARE(audioService->outputDeviceIds, std::vector<int> { 9 });
}

void AudioSettingsModelTest::test_reinitialized_shouldPutTheChosenDevicesBack()
{
    // Reinitializing builds a new recorder and player, which come up on the system default. The
    // choice was only refreshed in the lists and never handed down again, so the boxes went on
    // showing a device nothing was recording from -- and the application reinitializes once at
    // startup, right after this model has applied the saved one. Changing the box and changing it back
    // was the only way to make it take.
    const auto settingsService = std::make_shared<SettingsService>();
    settingsService->setAudioInputDeviceId(7);
    settingsService->setAudioOutputDeviceId(9);

    const auto audioService = std::make_shared<RecordingAudioService>(settingsService, std::make_shared<AudioEngine>());
    AudioSettingsModel model { audioService, settingsService };
    QCOMPARE(audioService->inputDeviceIds.size(), static_cast<size_t>(1));

    emit audioService->reinitialized();

    QCOMPARE(audioService->inputDeviceIds.size(), static_cast<size_t>(2));
    QCOMPARE(audioService->inputDeviceIds.back(), 7);
    QCOMPARE(audioService->outputDeviceIds.size(), static_cast<size_t>(2));
    QCOMPARE(audioService->outputDeviceIds.back(), 9);

    // And what the user picked since, rather than what was saved at startup.
    model.setSelectedInputDeviceId(11);
    emit audioService->reinitialized();
    QCOMPARE(audioService->inputDeviceIds.back(), 11);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AudioSettingsModelTest)
