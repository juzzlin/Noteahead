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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

AnimatedDialog {
    id: rootItem
    modal: true
    title: "<strong>" + qsTr("Add MIDI CC Setting") + "</strong>"
    standardButtons: Dialog.Ok | Dialog.Cancel
    property string portName: ""
    ColumnLayout {
        width: parent.width
        spacing: 10
        Label {
            text: qsTr("Controller:")
        }
        MidiCcComboBox {
            id: controllerComboBox
            portName: rootItem.portName
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("Value:")
        }
        SpinBox {
            id: valueSpinBox
            Layout.fillWidth: true
            from: propertyService.minValue(controllerComboBox.currentValue, rootItem.portName)
            to: propertyService.maxValue(controllerComboBox.currentValue, rootItem.portName)
            value: 0
            editable: true
        }
    }
    onAccepted: {
        trackSettingsModel.midiCcModel.addMidiCcSetting(controllerComboBox.currentValue, valueSpinBox.value);
        trackSettingsModel.applyAll();
    }
}
