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

#ifndef DEVICE_SERVICE_TEST_HPP
#define DEVICE_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class DeviceServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_midiCc_shouldNotEmitDataChanged();
    void test_midiCc_drumSynthVoice_shouldNotEmitDataChanged();
    void test_allNotesOff_sampler_shouldNotEmitDataChanged();
    void test_exportDeviceSettings_shouldGenerateCorrectXml();
    void test_importDeviceSettings_shouldRestoreParameters();
    void test_importDeviceSettings_shouldReplaceDeviceIfTypeDiffers();
    void test_importDeviceSettings_emptySlot_shouldCreateDeviceFromFile();
    void test_copyDevice_shouldDuplicateParametersIntoTargetSlot();
    void test_copyDevice_differentType_shouldReplaceTargetDevice();
    void test_copyDevice_emptySource_shouldFail();
    void test_copyDevice_sameSlot_shouldFail();
    void test_exportImport_withEmbeddedData_shouldWork();
    void test_importDeviceSettings_embeddedData_emptySlot_shouldExtractDataBeforeLoadingSamples();
    void test_peekDeviceTypeInfo_synth_shouldReturnCorrectTypeInfo();
    void test_peekDeviceTypeInfo_nonexistentFile_shouldReturnEmpty();
    void test_reverbSends_shouldSaveAndLoadCorrectly();
    void test_masterRackEnabled_shouldSaveAndLoadCorrectly();
};

} // namespace noteahead

#endif // DEVICE_SERVICE_TEST_HPP
