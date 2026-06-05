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

#ifndef DEVICE_RACK_CONTROLLER_TEST_HPP
#define DEVICE_RACK_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class DeviceRackControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_devices_shouldReturnDeviceNames();
    void test_trackNames_shouldReturnTrackNamesForDevice();
    void test_setDevice_shouldAddDeviceAndNotify();
    void test_clearDevice_shouldRemoveDeviceAndNotify();
    void test_addMethods_shouldAddDevicesToFirstEmptySlot();
    void test_availableDevices_shouldReturnCorrectList();
    void test_removeDeviceByName_shouldClearCorrectSlot();
};

} // namespace noteahead

#endif // DEVICE_RACK_CONTROLLER_TEST_HPP
