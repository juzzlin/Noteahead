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
import QtQuick.Dialogs
import QtQuick.Layouts

AnimatedDialog {
    id: rootItem
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    function setTitle(text) {
        title = "<strong>" + text + "</strong>";
    }
    function setMinValue(value) {
        spinBox.from = value;
    }
    function setMaxValue(value) {
        spinBox.to = value;
    }
    function setValue(value) {
        spinBox.value = value;
    }
    function value() {
        return spinBox.value;
    }
    contentItem: RowLayout {
        spacing: 10
        width: parent.width
        Label {
            text: qsTr("Choose a value (%1-%2):").arg(spinBox.from).arg(spinBox.to)
            width: parent.width
        }
        SpinBox {
            id: spinBox
            width: parent.width * 0.6
            from: 0
            to: 100
            value: 50
            editable: true
            Keys.onReturnPressed: {
                focus = false;
                rootItem.accept();
            }
            Layout.fillWidth: true
        }
    }
}
