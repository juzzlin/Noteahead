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
import "../Components"

AnimatedDialog {
    id: root
    title: "<strong>" + qsTr("Song Overview") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 900
    height: parent ? parent.height * Constants.largeDialogScale : 600

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    // Nothing is cached in the controller, so the map is built fresh each time it is shown and
    // cannot show a rack that has since been changed.
    onAboutToShow: {
        songOverviewController.refresh();
        mapRenderer.hoveredNode = -1;
    }

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        Universal.theme: Universal.Dark
        Universal.accent: themeService.accentColor
        AppButton {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            text: qsTr("Signal flow as the engine runs it. Each device's chain is drawn in its real order, so a device whose fader sits after its inserts reads differently from one whose fader comes first. Click a box to open its editor.")
            wrapMode: Text.WordWrap
            color: "#aaaaaa"
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Flickable {
            id: flickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: Math.max(mapRenderer.contentWidth, width)
            contentHeight: Math.max(mapRenderer.contentHeight, height)
            ScrollBar.vertical: ScrollBar {}
            ScrollBar.horizontal: ScrollBar {}

            SongOverviewRenderer {
                id: mapRenderer
                width: flickable.contentWidth
                height: flickable.contentHeight
                accentColor: themeService.accentColor
                nodes: {
                    songOverviewController.graphChanged;
                    return songOverviewController.nodes;
                }
                edges: {
                    songOverviewController.graphChanged;
                    return songOverviewController.edges;
                }

                // Hit testing lives here rather than in the painter: the renderer hands back the
                // rects it worked out, and the mouse areas are laid over them, which keeps hover
                // and cursor behaviour declarative like the rest of the application's QML.
                Repeater {
                    model: mapRenderer.boxRects
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        x: modelData.x
                        y: modelData.y
                        width: modelData.width
                        height: modelData.height
                        radius: 4
                        color: "transparent"
                        border.color: mouseArea.containsMouse ? themeService.accentColor : "transparent"
                        border.width: 1

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: songOverviewController.openNode(parent.index)
                            // Lighting the whole downstream route is the renderer's job; all this
                            // has to say is which box the pointer is over.
                            onEntered: mapRenderer.hoveredNode = parent.index
                            onExited: {
                                if (mapRenderer.hoveredNode === parent.index) {
                                    mapRenderer.hoveredNode = -1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
