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

AnimatedDialog {
    id: gainConverterDialog
    title: qsTr("Gain Converter (dB => linear)")
    modal: true
    clip: true
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
    property real dbValue: 0.0
    property real linearValue: 1.0
    property bool updating: false
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        clip: true
        RowLayout {
            Layout.fillWidth: true
            SpinBox {
                id: dbSpinBox
                from: -60
                to: 24
                value: dbValue
                stepSize: 1
                editable: true
                onValueChanged: {
                    gainConverterDialog.dbValue = value;
                    gainConverterDialog.calculateFromDb();
                }
            }
            Label {
                text: qsTr("dB")
                verticalAlignment: Label.AlignVCenter
                padding: 4
            }
            Label {
                id: linearFromDbLabel
                Layout.fillWidth: true
                text: `= ${gainConverterDialog.linearValue.toFixed(3)}`
                font.bold: true
                font.pixelSize: dbSpinBox.height
            }
        }
    }
    function calculateFromDb(): void {
        linearValue = Math.pow(10, dbValue / 20);
        linearFromDbLabel.text = `= ${linearValue.toFixed(3)}`;
    }
    Component.onCompleted: calculateFromDb()
}
