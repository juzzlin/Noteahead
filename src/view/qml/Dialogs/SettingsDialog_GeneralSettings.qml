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
    title: qsTr("General")
    width: parent.width
    ColumnLayout {
        width: parent.width
        spacing: 10

        GroupBox {
            title: qsTr("Editor")
            Layout.fillWidth: true
            GridLayout {
                columns: 9
                rows: 5
                width: parent.width
                Label {
                    text: qsTr("Number of lines visible:")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 0
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: visibleLinesSpinBox
                    from: 16
                    to: 32
                    Layout.column: 4
                    Layout.columnSpan: 5
                    Layout.row: 0
                    Layout.fillWidth: true
                    value: settingsService.visibleLines
                    editable: true
                    onValueChanged: settingsService.setVisibleLines(value)
                    Keys.onReturnPressed: focus = false
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set number of visible lines in the editor view")
                }
                Label {
                    text: qsTr("Track header font size:")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 1
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: trackHeaderFontSizeSpinBox
                    from: 8
                    to: 64
                    Layout.column: 4
                    Layout.columnSpan: 5
                    Layout.row: 1
                    Layout.fillWidth: true
                    value: settingsService.trackHeaderFontSize
                    editable: true
                    onValueChanged: settingsService.setTrackHeaderFontSize(value)
                    Keys.onReturnPressed: focus = false
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set font size for track/column headers")
                }
                Label {
                    text: qsTr("Pattern Peek:")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 2
                    Layout.fillWidth: true
                }
                CheckBox {
                    id: patternPeekCheckBox
                    Layout.column: 4
                    Layout.columnSpan: 5
                    Layout.row: 2
                    Layout.fillWidth: true
                    checked: settingsService.patternPeekEnabled
                    onCheckedChanged: settingsService.setPatternPeekEnabled(checked)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Show a dimmed glimpse of the previous/next patterns in the editor's offset areas")
                }
                Label {
                    text: qsTr("Automation display:")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 3
                    Layout.fillWidth: true
                }
                ComboBox {
                    id: automationDisplayModeComboBox
                    Layout.column: 4
                    Layout.columnSpan: 5
                    Layout.row: 3
                    Layout.fillWidth: true
                    model: [qsTr("Tint"), qsTr("Curve")]
                    currentIndex: settingsService.automationDisplayMode
                    onActivated: i => settingsService.automationDisplayMode = i
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Tint colours each automated line by its value. Curve draws every automation on the column as its own vertical trace, value running left to right.")
                }
                Label {
                    text: qsTr("Automation line thickness:")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 4
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: automationCurveThicknessSpinBox
                    Layout.column: 4
                    Layout.columnSpan: 5
                    Layout.row: 4
                    Layout.fillWidth: true
                    // Tenths of a pixel: the useful range is finer than a whole pixel
                    from: 5
                    to: 50
                    stepSize: 5
                    editable: true
                    enabled: settingsService.automationDisplayMode === 1
                    value: settingsService.automationCurveThicknessTenths
                    onValueModified: settingsService.automationCurveThicknessTenths = value
                    textFromValue: function (value, locale) {
                        return Number(value / 10).toLocaleString(locale, 'f', 1);
                    }
                    valueFromText: function (text, locale) {
                        return Number.fromLocaleString(locale, text) * 10;
                    }
                    Keys.onReturnPressed: focus = false
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Width of a drawn automation curve, in pixels")
                }
            }
        }

        GroupBox {
            title: qsTr("Device Rack")
            Layout.fillWidth: true
            GridLayout {
                columns: 9
                rows: 1
                width: parent.width
                Label {
                    text: qsTr("Gain staging target (dBFS):")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 0
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: gainStagingTargetSpinBox
                    Layout.column: 4
                    Layout.columnSpan: 5
                    Layout.row: 0
                    Layout.fillWidth: true
                    from: -30
                    to: 0
                    stepSize: 1
                    editable: true
                    value: settingsService.gainStagingTargetDb
                    Keys.onReturnPressed: focus = false
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Where the marker sits on the Device Rack level meters, i.e. the level a gain staged device should read. Common choices are -18 (EBU R128), -20 (SMPTE), -14 and -12.")
                    onValueModified: settingsService.gainStagingTargetDb = value
                }
            }
        }

        GroupBox {
            title: qsTr("Playback")
            Layout.fillWidth: true
            GridLayout {
                columns: 9
                rows: 1
                width: parent.width
                Label {
                    text: qsTr("Disable UI updates during playback:")
                    Layout.column: 0
                    Layout.columnSpan: 2
                    Layout.row: 0
                    Layout.fillWidth: true
                }
                CheckBox {
                    id: uiUpdatesDisabledCheckBox
                    Layout.column: 4
                    Layout.columnSpan: 5
                    Layout.row: 0
                    Layout.fillWidth: true
                    checked: settingsService.uiUpdatesDisabledDuringPlayback
                    onCheckedChanged: settingsService.setUiUpdatesDisabledDuringPlayback(checked)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Reduces CPU usage by disabling UI updates (except time) during playback")
                }
            }
        }
    }
}