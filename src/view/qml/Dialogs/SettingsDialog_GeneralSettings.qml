import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."

GroupBox {
    title: qsTr("General")
    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        GroupBox {
            title: qsTr("Editor")
            Layout.fillWidth: true
            GridLayout {
                columns: 9
                rows: 2
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

        Item {
            Layout.fillHeight: true
        }
    }
}