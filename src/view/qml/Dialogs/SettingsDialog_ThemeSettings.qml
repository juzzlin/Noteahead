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
import ".."
import "../Components"

GroupBox {
    title: qsTr("Theme")
    width: parent.width
    label: Label {
        text: parent.title
        color: themeService.mainMenuTextColor
        font.bold: true
    }

    ColumnLayout {
        width: parent.width
        spacing: 10

        GroupBox {
            title: qsTr("Colors")
            Layout.fillWidth: true
            label: Label {
                text: parent.title
                color: themeService.mainMenuTextColor
                font.bold: true
            }

            ColumnLayout {
                spacing: 10
                width: parent.width

                // The group is already titled "Colors", so the labels only need to name the target.
                // Equal preferred widths split the row into even cells, and centering the swatch in
                // its cell puts the two boxes at 25% and 75% of the row regardless of label lengths.
                RowLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 0
                        implicitHeight: Math.max(accentColorLabel.implicitHeight, accentColorPreview.height)
                        Label {
                            id: accentColorLabel
                            text: qsTr("Accent:")
                            color: themeService.mainMenuTextColor
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Rectangle {
                            id: accentColorPreview
                            width: 40
                            height: 20
                            color: themeService.accentColor
                            border.color: themeService.mainMenuTextColor
                            border.width: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: accentColorDialog.open()
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 0
                        implicitHeight: Math.max(cursorColorLabel.implicitHeight, cursorColorPreview.height)
                        Label {
                            id: cursorColorLabel
                            text: qsTr("Cursor:")
                            color: themeService.mainMenuTextColor
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Rectangle {
                            id: cursorColorPreview
                            width: 40
                            height: 20
                            color: themeService.cursorColor
                            border.color: themeService.mainMenuTextColor
                            border.width: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: cursorColorDialog.open()
                            }
                        }
                    }
                }

                LayoutSeparator {}

                // Track and automation colors are blended towards the accent color. This decides how far:
                // 0 % is the original fixed palette, 100 % puts every entry on the accent hue.
                RowLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Accent blend:")
                        color: themeService.mainMenuTextColor
                    }
                    Slider {
                        id: accentBlendSlider
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        stepSize: 1
                        snapMode: Slider.SnapAlways
                        value: themeService.paletteAccentBlend
                        onMoved: themeService.paletteAccentBlend = value
                        // Same look as the device knobs, which set both of these themselves
                        Universal.theme: Universal.Dark
                        Universal.accent: themeService.accentColor
                        // ...and the same feel: the wheel nudges it like it does on a knob
                        WheelHandler {
                            onWheel: wheel => {
                                const delta = wheel.angleDelta.y > 0 ? 1 : -1;
                                themeService.paletteAccentBlend = Math.max(accentBlendSlider.from, Math.min(accentBlendSlider.to, accentBlendSlider.value + delta));
                            }
                        }
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("How far the track and automation colors are pulled towards the accent color. 0 % is the original palette, 100 % puts every color on the accent hue.")
                    }
                    Label {
                        text: qsTr("%1 %").arg(Math.round(accentBlendSlider.value))
                        color: themeService.mainMenuTextColor
                        horizontalAlignment: Text.AlignRight
                        Layout.preferredWidth: 40
                    }
                }
            }
        }
    }

    ColorDialog {
        id: accentColorDialog
        title: qsTr("Select Accent Color")
        selectedColor: themeService.accentColor
        onAccepted: {
            themeService.accentColor = selectedColor
        }
    }

    ColorDialog {
        id: cursorColorDialog
        title: qsTr("Select Cursor Color")
        selectedColor: themeService.cursorColor
        onAccepted: {
            themeService.cursorColor = selectedColor
        }
    }
}
