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

Dialog {
    id: root
    property int effectIndex: -1
    title: "<strong>" + qsTr("LUFS Meter (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 320
    height: 420

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    // Live readings
    property real currentMomentary: -70.0
    property real currentShortTerm: -70.0

    // Meter range
    readonly property real lufsMin: -60.0
    readonly property real lufsMax: 0.0

    function lufsToFraction(lufs) {
        return Math.max(0.0, Math.min(1.0, (lufs - lufsMin) / (lufsMax - lufsMin)));
    }

    function lufsColor(lufs) {
        if (lufs >= -6) return "#ff4444";
        if (lufs >= -12) return "#ffaa00";
        return themeService.accentColor;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // The two meters side by side
        RowLayout {
            spacing: 20
            Layout.alignment: Qt.AlignHCenter

            // Momentary meter (400 ms)
            ColumnLayout {
                spacing: 6
                Layout.alignment: Qt.AlignTop

                Label {
                    text: qsTr("M")
                    font.bold: true
                    font.pointSize: 11
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: qsTr("400 ms")
                    color: "#888"
                    font.pointSize: 8
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle {
                    width: 50
                    height: 280
                    color: "#111"
                    border.color: "#333"
                    Layout.alignment: Qt.AlignHCenter

                    // Scale tick marks and labels
                    Item {
                        anchors.fill: parent
                        Repeater {
                            model: [0, -6, -12, -18, -24, -36, -48, -60]
                            Rectangle {
                                property real frac: root.lufsToFraction(modelData)
                                width: parent.width
                                height: 1
                                color: "#555"
                                opacity: 0.6
                                y: (1.0 - frac) * (parent.height - 4) + 2
                                Label {
                                    text: modelData === 0 ? "0" : modelData.toString()
                                    color: "#777"
                                    font.pointSize: 7
                                    anchors.right: parent.right
                                    anchors.rightMargin: 2
                                    anchors.bottom: parent.top
                                }
                            }
                        }
                    }

                    // Fill bar (grows upward from bottom)
                    Rectangle {
                        id: momentaryFill
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width - 8
                        height: root.lufsToFraction(root.currentMomentary) * (parent.height - 4)
                        color: root.lufsColor(root.currentMomentary)
                        Behavior on height { SmoothedAnimation { velocity: 600 } }
                    }
                }

                Label {
                    text: root.currentMomentary <= -60 ? "−∞" : root.currentMomentary.toFixed(1)
                    color: root.lufsColor(root.currentMomentary)
                    font.family: "Monospace"
                    font.pointSize: 10
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: qsTr("LUFS")
                    color: "#888"
                    font.pointSize: 8
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Short-term meter (3 s)
            ColumnLayout {
                spacing: 6
                Layout.alignment: Qt.AlignTop

                Label {
                    text: qsTr("S")
                    font.bold: true
                    font.pointSize: 11
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: qsTr("3 s")
                    color: "#888"
                    font.pointSize: 8
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle {
                    width: 50
                    height: 280
                    color: "#111"
                    border.color: "#333"
                    Layout.alignment: Qt.AlignHCenter

                    Item {
                        anchors.fill: parent
                        Repeater {
                            model: [0, -6, -12, -18, -24, -36, -48, -60]
                            Rectangle {
                                property real frac: root.lufsToFraction(modelData)
                                width: parent.width
                                height: 1
                                color: "#555"
                                opacity: 0.6
                                y: (1.0 - frac) * (parent.height - 4) + 2
                                Label {
                                    text: modelData === 0 ? "0" : modelData.toString()
                                    color: "#777"
                                    font.pointSize: 7
                                    anchors.right: parent.right
                                    anchors.rightMargin: 2
                                    anchors.bottom: parent.top
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: shortTermFill
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width - 8
                        height: root.lufsToFraction(root.currentShortTerm) * (parent.height - 4)
                        color: root.lufsColor(root.currentShortTerm)
                        Behavior on height { SmoothedAnimation { velocity: 200 } }
                    }
                }

                Label {
                    text: root.currentShortTerm <= -60 ? "−∞" : root.currentShortTerm.toFixed(1)
                    color: root.lufsColor(root.currentShortTerm)
                    font.family: "Monospace"
                    font.pointSize: 10
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: qsTr("LUFS")
                    color: "#888"
                    font.pointSize: 8
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }

    Timer {
        interval: 33
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentMomentary = effectRackController.lufsMeterMomentary(root.effectIndex);
            root.currentShortTerm = effectRackController.lufsMeterShortTerm(root.effectIndex);
        }
    }
}
