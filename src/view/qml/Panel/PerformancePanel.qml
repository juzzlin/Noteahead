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
import QtQuick.Layouts 1.15
import Noteahead 1.0
import ".."
import "../Components"

//! A strip under the editor for playing the project without opening a window: a row of elements,
//! each showing one device. Not modal and never takes the keyboard, so the tracker keeps it while
//! a pad is played. What it shows is saved with the project; whether it is shown, and how tall, are
//! the user's settings.
Rectangle {
    id: root

    //! The tallest the panel may be dragged, set by the window: the editor needs the rest.
    property real maximumHeight: 400
    readonly property real minimumHeight: 120
    readonly property real panelHeight: Math.max(root.minimumHeight, Math.min(settingsService.performancePanelHeight, root.maximumHeight))

    color: Theme.surfaceDialog
    border.color: Theme.borderMedium
    border.width: 1

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    // The top edge drags the panel taller or shorter, saved when let go
    MouseArea {
        id: resizeHandle
        property real startHeight: 0
        property real startY: 0
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 6
        z: 1
        cursorShape: Qt.SizeVerCursor
        onPressed: mouse => {
            startHeight = root.panelHeight;
            startY = mapToItem(null, mouse.x, mouse.y).y;
        }
        onPositionChanged: mouse => {
            if (pressed) {
                const height = startHeight - (mapToItem(null, mouse.x, mouse.y).y - startY);
                settingsService.performancePanelHeight = Math.round(Math.max(root.minimumHeight, Math.min(height, root.maximumHeight)));
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        anchors.topMargin: 8
        spacing: 6

        ListView {
            id: elementList
            orientation: ListView.Horizontal
            model: performancePanelModel
            spacing: 6
            clip: true
            // Dragging the list would take the drag from a pad: it scrolls with its bar instead
            interactive: false
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.horizontal: ScrollBar {
                policy: elementList.contentWidth > elementList.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
            }
            delegate: PanelElement {
                required property int index
                height: elementList.height - (elementList.contentWidth > elementList.width ? 10 : 0)
                onRemoveRequested: performancePanelModel.removeElement(index)
            }
            footer: Item {
                width: addButton.width + 6
                height: elementList.height
                AppButton {
                    id: addButton
                    x: elementList.count > 0 ? 6 : 0
                    text: "+"
                    width: 32
                    height: 32
                    focusPolicy: Qt.NoFocus
                    toolTipText: qsTr("Add to the panel")
                    onClicked: addMenu.popup(addButton, 0, addButton.height)
                }
            }

            Label {
                visible: elementList.count === 0
                // Beside the + it talks about
                x: 44
                height: 32
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Play devices from here while the song plays. Add one with +.")
                color: Theme.textMuted
                font.pixelSize: Theme.bodyPixelSize
            }
        }
    }

    Menu {
        id: addMenu
        Universal.theme: Universal.Dark
        Universal.accent: themeService.accentColor
        delegate: MenuItemDelegate {}

        property var elements: []
        onAboutToShow: elements = performancePanelController.availableElements()

        Instantiator {
            model: addMenu.elements
            onObjectAdded: (index, object) => addMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => addMenu.removeItem(object)
            delegate: MenuItem {
                required property var modelData
                text: qsTr("Slot %1: %2").arg(modelData.slot + 1).arg(deviceRackController.deviceDisplayName(modelData.slot))
                enabled: !modelData.onPanel
                onTriggered: performancePanelController.addDevice(modelData.slot)
            }
        }
        MenuSeparator {
            visible: addMenu.elements.length > 0
            height: visible ? implicitHeight : 0
        }
        MenuItem {
            text: qsTr("New XY Pad")
            onTriggered: root._addNewXyPad()
        }
    }

    //! Puts a new XY pad device in the first free slot of the rack, and on the panel
    function _addNewXyPad(): void {
        for (let slot = 0; slot < deviceRackController.deviceCount; slot++) {
            if (deviceRackController.deviceType(slot) === "") {
                deviceRackController.setDevice(slot, performancePanelController.xyPadDeviceTypeId);
                performancePanelController.addDevice(slot);
                return;
            }
        }
        applicationService.requestAlertDialog(qsTr("The device rack is full."));
    }
}
