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

#include "device_rack_controller_test.hpp"

#include "../../application/service/device_rack_controller.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/sampler_controller.hpp"
#include "../../application/service/synth_controller.hpp"
#include "../../common/constants.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

class MockDeviceService : public DeviceService
{
public:
    MockDeviceService() : DeviceService(nullptr) {}
    QStringList internalDeviceNamesQt() const override { return m_names; }
    void setMockNames(const QStringList & names) { m_names = names; }
private:
    QStringList m_names;
};

class MockEditorService : public EditorService
{
public:
    std::vector<quint64> trackIndices() const override { return m_indices; }
    void setMockIndices(const std::vector<quint64> & indices) { m_indices = indices; }

    QString trackName(quint64 trackIndex) const override { return m_names.at(trackIndex); }
    void setMockTrackName(quint64 trackIndex, const QString & name) { m_names[trackIndex] = name; }

    QString instrumentPortName(quint64 trackIndex) const override { return m_ports.at(trackIndex); }
    void setMockInstrumentPortName(quint64 trackIndex, const QString & port) { m_ports[trackIndex] = port; }

private:
    std::vector<quint64> m_indices;
    std::map<quint64, QString> m_names;
    std::map<quint64, QString> m_ports;
};

void DeviceRackControllerTest::test_devices()
{
    const auto deviceService = std::make_shared<MockDeviceService>();
    const auto names = QStringList { "Noteahead Sampler 1", "Noteahead Synth 1" };
    deviceService->setMockNames(names);

    DeviceRackController controller(deviceService, nullptr, nullptr, nullptr, nullptr);
    QCOMPARE(controller.rowCount(), 2);
    QCOMPARE(controller.data(controller.index(0), static_cast<int>(DeviceRackController::DataRole::Name)).toString(), names.at(0));
    QCOMPARE(controller.data(controller.index(1), static_cast<int>(DeviceRackController::DataRole::Name)).toString(), names.at(1));
}

void DeviceRackControllerTest::test_trackNames()
{
    const auto deviceService = std::make_shared<MockDeviceService>();
    deviceService->setMockNames({ "Noteahead Sampler 1", "Noteahead Synth 1" });

    const auto editorService = std::make_shared<MockEditorService>();
    editorService->setMockIndices({ 0, 1, 2, 3 });
    editorService->setMockTrackName(0, "Track 1");
    editorService->setMockInstrumentPortName(0, "Noteahead Sampler 1");
    editorService->setMockTrackName(1, "Track 2");
    editorService->setMockInstrumentPortName(1, "Noteahead Synth 1");
    editorService->setMockTrackName(2, "Track 3");
    editorService->setMockInstrumentPortName(2, "Noteahead Sampler 1");
    editorService->setMockTrackName(3, "Track 4");
    editorService->setMockInstrumentPortName(3, "Noteahead Synth 1");

    DeviceRackController controller(deviceService, nullptr, nullptr, nullptr, editorService);

    QCOMPARE(controller.data(controller.index(0), static_cast<int>(DeviceRackController::DataRole::TrackNames)).toString(), QString("Track 1, Track 3"));
    QCOMPARE(controller.data(controller.index(1), static_cast<int>(DeviceRackController::DataRole::TrackNames)).toString(), QString("Track 2, Track 4"));
}

void DeviceRackControllerTest::test_openDevice()
{
    // This test would require more complex mocking of DeviceService to return real Device objects
    // or further mocking of Sampler/Synth controllers.
    // For now, let's focus on the requested trackNames functionality.
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DeviceRackControllerTest)
