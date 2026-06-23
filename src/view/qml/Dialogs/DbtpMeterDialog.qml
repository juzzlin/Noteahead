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
    title: "<strong>" + qsTr("dBTP Meter (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 320
    height: 460

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

    property real currentL: -70.0
    property real currentR: -70.0
    property real holdL: -70.0
    property real holdR: -70.0

    readonly property real dbtpMin: -60.0
    readonly property real dbtpMax: 0.0

    function toFraction(dbtp) {
        return Math.max(0.0, Math.min(1.0, (dbtp - dbtpMin) / (dbtpMax - dbtpMin)));
    }

    function barColor(dbtp) {
        if (dbtp >= -1.0) return "#ff4444";
        if (dbtp >= -6.0) return "#ffaa00";
        return themeService.accentColor;
    }

    function formatDbtp(dbtp) {
        return dbtp <= dbtpMin ? "−∞" : dbtp.toFixed(1);
    }

    // Reusable channel meter component
    component ChannelMeter: ColumnLayout {
        id: chanMeter
        required property string label
        required property real level
        required property real hold
        spacing: 6
        Layout.alignment: Qt.AlignTop

        Label {
            text: chanMeter.label
            font.bold: true
            font.pointSize: 11
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
            id: meterRect
            width: 60
            height: 300
            color: "#111"
            border.color: "#333"
            Layout.alignment: Qt.AlignHCenter

            // Scale tick marks
            Item {
                anchors.fill: parent
                Repeater {
                    model: [0, -1, -3, -6, -12, -18, -24, -36, -48, -60]
                    Rectangle {
                        property real frac: root.toFraction(modelData)
                        width: parent.width
                        height: 1
                        color: modelData === -1 ? "#ff8888" : "#555"
                        opacity: modelData === -1 ? 0.9 : 0.6
                        y: (1.0 - frac) * (parent.height - 4) + 2
                        Label {
                            text: modelData === 0 ? "0" : modelData.toString()
                            color: modelData === -1 ? "#ff8888" : "#777"
                            font.pointSize: 7
                            anchors.right: parent.right
                            anchors.rightMargin: 2
                            anchors.bottom: parent.top
                        }
                    }
                }
            }

            // Running bar (grows upward)
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 8
                height: root.toFraction(chanMeter.level) * (parent.height - 4)
                color: root.barColor(chanMeter.level)
                Behavior on height { SmoothedAnimation { velocity: 800 } }
            }

            // Peak hold line
            Rectangle {
                property real holdFrac: root.toFraction(chanMeter.hold)
                visible: chanMeter.hold > root.dbtpMin
                x: 2
                width: parent.width - 4
                height: 2
                y: (1.0 - holdFrac) * (parent.height - 4) + 2
                color: root.barColor(chanMeter.hold)
                opacity: 0.9
            }
        }

        Label {
            text: root.formatDbtp(chanMeter.hold)
            color: root.barColor(chanMeter.hold)
            font.family: "Monospace"
            font.pointSize: 10
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: qsTr("dBTP")
            color: "#888"
            font.pointSize: 8
            Layout.alignment: Qt.AlignHCenter
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
            spacing: 20
            Layout.alignment: Qt.AlignHCenter

            ChannelMeter {
                label: qsTr("L")
                level: root.currentL
                hold: root.holdL
            }

            ChannelMeter {
                label: qsTr("R")
                level: root.currentR
                hold: root.holdR
            }
        }
    }

    Timer {
        interval: 33
        running: root.opened
        repeat: true
        onTriggered: {
            root.currentL = effectRackController.dbtpMeterTruePeakL(root.effectIndex);
            root.currentR = effectRackController.dbtpMeterTruePeakR(root.effectIndex);
            root.holdL = effectRackController.dbtpMeterTruePeakHoldL(root.effectIndex);
            root.holdR = effectRackController.dbtpMeterTruePeakHoldR(root.effectIndex);
        }
    }
}
