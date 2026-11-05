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
import "../Dialogs/ParameterAutomationTargets.js" as Targets

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
            readonly property real scrollBarSpace: 10
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
                // Room for the scroll bar is kept whether it shows or not. Taking it only while it
                // shows froze the application: shorter elements have narrower square pads, which
                // could hide the bar again, and so on forever.
                height: elementList.height - elementList.scrollBarSpace
                opacity: root.dragFrom === index ? 0.5 : 1.0
                onRemoveRequested: performancePanelModel.removeElement(index)
                onDragStarted: title => root._beginDrag(index, title)
                onDragMoved: scenePosition => root._moveDrag(scenePosition)
                onDragFinished: root._endDrag()
                onMenuRequested: {
                    elementMenu.row = index;
                    elementMenu.popup();
                }
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
                text: qsTr("Play pads and watch meters here while the song plays. Add one with +.")
                color: Theme.textMuted
                font.pixelSize: Theme.bodyPixelSize
            }
        }
    }

    //! Where the dragged element would land, between two others
    Rectangle {
        id: insertionLine
        visible: root.dragFrom >= 0 && root.dropGap >= 0 && root.dropGap !== root.dragFrom && root.dropGap !== root.dragFrom + 1
        width: 3
        radius: 1
        color: themeService.accentColor
        y: root.listOrigin.y
        height: elementList.height - elementList.scrollBarSpace
        x: root.listOrigin.x + root._gapX(root.dropGap) - width / 2
        z: 2
    }

    //! The dragged element's title, following the pointer
    Rectangle {
        id: dragGhost
        visible: root.dragFrom >= 0
        width: Math.min(ghostLabel.implicitWidth + 16, 240)
        height: ghostLabel.implicitHeight + 8
        color: Theme.surfaceInset
        border.color: themeService.accentColor
        radius: 2
        opacity: 0.85
        y: root.listOrigin.y
        z: 3
        Label {
            id: ghostLabel
            anchors.centerIn: parent
            width: parent.width - 16
            text: root.dragTitle
            color: Theme.textSecondary
            font.pixelSize: Theme.bodyPixelSize
            elide: Text.ElideRight
        }
    }

    // Near either end of a panel wider than its window, the drag scrolls it
    Timer {
        interval: 16
        repeat: true
        running: root.dragFrom >= 0 && root.scrollStep !== 0
        onTriggered: {
            elementList.contentX = Math.max(0, Math.min(elementList.contentWidth - elementList.width, elementList.contentX + root.scrollStep));
            root._updateGap();
        }
    }

    FittedMenu {
        id: elementMenu
        property int row: -1
        Universal.theme: Universal.Dark
        Universal.accent: themeService.accentColor
        delegate: MenuItemDelegate {}
        MenuItem {
            text: qsTr("Move Left")
            enabled: elementMenu.row > 0
            onTriggered: performancePanelModel.moveElement(elementMenu.row, elementMenu.row - 1)
        }
        MenuItem {
            text: qsTr("Move Right")
            enabled: elementMenu.row >= 0 && elementMenu.row < elementList.count - 1
            onTriggered: performancePanelModel.moveElement(elementMenu.row, elementMenu.row + 1)
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Remove from the panel")
            onTriggered: performancePanelModel.removeElement(elementMenu.row)
        }
    }

    FittedMenu {
        id: addMenu
        Universal.theme: Universal.Dark
        Universal.accent: themeService.accentColor
        delegate: MenuItemDelegate {}

        //! The XY pads first, then the meters, as availableElements() lists them
        property var elements: []
        readonly property int effectCount: elements.filter(element => performancePanelController.isEffectType(element.type)).length
        readonly property int xyPadCount: elements.length - effectCount
        onAboutToShow: elements = performancePanelController.availableElements()

        Instantiator {
            model: addMenu.elements
            // A meter goes after the separator that follows the XY pads
            onObjectAdded: (index, object) => addMenu.insertItem(index < addMenu.xyPadCount ? index : index + 1, object)
            onObjectRemoved: (index, object) => addMenu.removeItem(object)
            delegate: MenuItem {
                required property var modelData
                text: performancePanelController.isEffectType(modelData.type) ? Targets.targetLabel(modelData.location) : qsTr("Slot %1: %2").arg(modelData.slot + 1).arg(deviceRackController.deviceDisplayName(modelData.slot))
                enabled: !modelData.onPanel
                onTriggered: {
                    if (performancePanelController.isEffectType(modelData.type)) {
                        performancePanelController.addEffect(modelData.sourceId);
                    } else {
                        performancePanelController.addDevice(modelData.slot);
                    }
                }
            }
        }
        MenuSeparator {
            visible: addMenu.xyPadCount > 0 && addMenu.effectCount > 0
            height: visible ? implicitHeight : 0
        }
        MenuSeparator {
            visible: addMenu.elements.length > 0
            height: visible ? implicitHeight : 0
        }
        MenuItem {
            text: qsTr("New XY Pad")
            onTriggered: root._addNewXyPad()
        }
        MenuItem {
            text: qsTr("New RTA")
            onTriggered: root._addNewEffect(effectRackController.rtaType)
        }
        MenuItem {
            text: qsTr("New LUFS Meter")
            onTriggered: root._addNewEffect(effectRackController.lufsMeterType)
        }
        MenuItem {
            text: qsTr("New dBTP Meter")
            onTriggered: root._addNewEffect(effectRackController.dbtpMeterType)
        }
        MenuItem {
            text: qsTr("New Stereo Field Meter")
            onTriggered: root._addNewEffect(effectRackController.stereoFieldMeterType)
        }
    }

    //! The row of the element being dragged, or -1
    property int dragFrom: -1
    property string dragTitle: ""
    //! The gap the element would be dropped into: 0 before the first element, count after the last
    property int dropGap: -1
    //! The pointer, in the list's own coordinates
    property real dragX: 0
    //! Where the list sits in the panel, taken when a drag starts
    property point listOrigin: Qt.point(0, 0)
    readonly property real scrollMargin: 40
    readonly property real scrollStep: {
        if (root.dragFrom < 0 || elementList.contentWidth <= elementList.width) {
            return 0;
        }
        if (root.dragX < root.scrollMargin) {
            return -8;
        }
        return root.dragX > elementList.width - root.scrollMargin ? 8 : 0;
    }

    function _beginDrag(row: int, title: string): void {
        const origin = elementList.mapToItem(root, 0, 0);
        root.listOrigin = Qt.point(origin.x, origin.y);
        root.dragTitle = title;
        root.dragFrom = row;
        root.dropGap = row;
    }

    function _moveDrag(scenePosition: point): void {
        const local = elementList.mapFromItem(null, scenePosition.x, scenePosition.y);
        root.dragX = local.x;
        dragGhost.x = Math.max(0, Math.min(root.width - dragGhost.width, root.listOrigin.x + local.x - dragGhost.width / 2));
        root._updateGap();
    }

    //! The gap whose element midpoints straddle the pointer
    function _updateGap(): void {
        const x = root.dragX + elementList.contentX;
        let gap = elementList.count;
        for (let row = 0; row < elementList.count; row++) {
            const item = elementList.itemAtIndex(row);
            if (item && x < item.x + item.width / 2) {
                gap = row;
                break;
            }
        }
        root.dropGap = gap;
    }

    //! The middle of the space before element @p gap, or after the last, in the list's coordinates
    function _gapX(gap: int): real {
        if (gap < 0 || elementList.count === 0) {
            return 0;
        }
        const item = elementList.itemAtIndex(Math.min(gap, elementList.count - 1));
        if (!item) {
            return 0;
        }
        const x = gap < elementList.count ? item.x - elementList.spacing / 2 : item.x + item.width + elementList.spacing / 2;
        return x - elementList.contentX;
    }

    function _endDrag(): void {
        const from = root.dragFrom;
        const gap = root.dropGap;
        root.dragFrom = -1;
        root.dropGap = -1;
        if (from < 0 || gap < 0 || from >= elementList.count) {
            return;
        }
        // Closing up behind the element shifts every gap after it one to the left
        const to = gap > from ? gap - 1 : gap;
        if (to !== from) {
            performancePanelModel.moveElement(from, Math.min(to, elementList.count - 1));
        }
    }

    //! Puts a new meter of @p effectType in the first free master insert slot, where it hears the
    //! whole mix, and on the panel
    function _addNewEffect(effectType: string): void {
        effectRackController.targetDeviceName = "";
        effectRackController.isInsertRack = true;
        effectRackController.targetSubIndex = -1;
        for (let slot = 0; slot < effectRackController.effectCount; slot++) {
            if (effectRackController.effectType(slot) === "") {
                effectRackController.setEffect(slot, effectType);
                performancePanelController.addEffect(effectRackController.effectId(slot));
                return;
            }
        }
        applicationService.requestAlertDialog(qsTr("The master insert rack is full."));
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
