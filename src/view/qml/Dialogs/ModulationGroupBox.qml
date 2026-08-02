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

GroupBox {
    title: qsTr("Modulation")
    Layout.fillWidth: true

    property alias modulationType: modulationTypeComboBox.currentIndex
    property alias cycles: cyclesSpinBox.value
    property alias amplitude: amplitudeSpinBox.value
    property alias offset: offsetSpinBox.value
    property alias inverted: invertedCheckBox.checked

    GridLayout {
        rowSpacing: 10
        width: parent.width
        columns: 5

        Label {
            text: qsTr("Type")
            Layout.row: 0
            Layout.column: 0
        }
        ComboBox {
            id: modulationTypeComboBox
            model: [qsTr("Sine Wave"), qsTr("Random")]
            Layout.row: 1
            Layout.column: 0
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("The type of modulation to apply (Sine Wave or Random)")
        }

        Label {
            text: qsTr("Cycles")
            Layout.row: 0
            Layout.column: 1
        }
        SpinBox {
            id: cyclesSpinBox
            from: 0
            to: 127
            value: 0
            editable: true
            Keys.onReturnPressed: {
                focus = false;
            }
            Layout.row: 1
            Layout.column: 1
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("The number of modulation cycles over the automation range")
        }

        Label {
            text: qsTr("Amplitude (%)")
            Layout.row: 0
            Layout.column: 2
        }
        SpinBox {
            id: amplitudeSpinBox
            from: 0
            to: 200
            value: 0
            editable: true
            stepSize: 1
            Keys.onReturnPressed: {
                focus = false;
            }
            Layout.row: 1
            Layout.column: 2
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("The strength of the modulation")
        }

        Label {
            text: qsTr("Offset (%)")
            Layout.row: 0
            Layout.column: 3
        }
        SpinBox {
            id: offsetSpinBox
            from: -100
            to: 100
            value: 0
            editable: true
            stepSize: 1
            Keys.onReturnPressed: {
                focus = false;
            }
            Layout.row: 1
            Layout.column: 3
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("An additive constant value applied to the modulation")
        }

        Label {
            text: qsTr("Inverted")
            Layout.row: 0
            Layout.column: 4
        }
        CheckBox {
            id: invertedCheckBox
            Layout.row: 1
            Layout.column: 4
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Invert the phase of the modulation")
        }
    }
}
