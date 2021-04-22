import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    title: qsTr("MIDI Effects")
    Layout.fillWidth: true
    width: parent.width
    property var _sideChainTargets: []
    function initialize(): void {
        velocityJitterSpinBox.value = trackSettingsModel.velocityJitter;

        sideChainService.trackIndex = trackSettingsModel.trackIndex;
        sideChainEnabledCheckBox.checked = sideChainService.sideChainEnabled;
        sideChainSourceTrackSpinBox.value = sideChainService.sideChainSourceTrack + 1;
        sideChainSourceColumnSpinBox.value = sideChainService.sideChainSourceColumn + 1;
        sideChainLookaheadSpinBox.value = sideChainService.sideChainLookahead;
        sideChainReleaseSpinBox.value = sideChainService.sideChainRelease;

        for (const sideChainTarget of _sideChainTargets) {
            sideChainTarget.initialize(sideChainService.sideChainTargetEnabled(sideChainTarget.targetIndex), sideChainService.sideChainTargetController(sideChainTarget.targetIndex), sideChainService.sideChainTargetTargetValue(sideChainTarget.targetIndex), sideChainService.sideChainTargetReleaseValue(sideChainTarget.targetIndex));
        }
    }
    ColumnLayout {
        spacing: 8
        width: parent.width
        GridLayout {
            columns: 9
            rows: 1
            width: parent.width
            Layout.fillWidth: true
            Label {
                text: qsTr("Random velocity jitter (%):")
                Layout.column: 0
                Layout.row: 0
                Layout.fillWidth: true
            }
            SpinBox {
                id: velocityJitterSpinBox
                from: 0
                to: 100
                editable: true
                Layout.column: 2
                Layout.row: 0
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set jitter on MIDI velocity to simulate e.g. a more natural piano")
                onValueChanged: {
                    trackSettingsModel.velocityJitter = value;
                }
                Keys.onReturnPressed: focus = false
            }
        }
        LayoutSeparator {}
        GroupBox {
            id: sideChainGroup
            title: qsTr("MIDI Side-chain")
            Layout.fillWidth: true
            width: parent.widthidth
            ColumnLayout {
                spacing: 8
                anchors.fill: parent
                CheckBox {
                    id: sideChainEnabledCheckBox
                    text: qsTr("Enabled")
                    onCheckedChanged: sideChainService.sideChainEnabled = checked
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Enable or disable MIDI side-chaining for this track.")
                }
                GridLayout {
                    columns: 4
                    width: parent.width
                    enabled: sideChainEnabledCheckBox.checked
                    Label {
                        text: qsTr("Source track:")
                    }
                    SpinBox {
                        id: sideChainSourceTrackSpinBox
                        from: 1
                        to: 255
                        editable: true
                        onValueChanged: sideChainService.sideChainSourceTrack = value - 1
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set the track from which to receive MIDI notes for side-chaining.")
                    }
                    Label {
                        text: qsTr("Source column:")
                    }
                    SpinBox {
                        id: sideChainSourceColumnSpinBox
                        from: 1
                        to: 255
                        editable: true
                        onValueChanged: sideChainService.sideChainSourceColumn = value - 1
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set the column within the source track from which to receive MIDI notes for side-chaining.")
                    }
                    Label {
                        text: qsTr("Lookahead (ms):")
                    }
                    SpinBox {
                        id: sideChainLookaheadSpinBox
                        from: 0
                        to: 1000
                        editable: true
                        onValueChanged: sideChainService.sideChainLookahead = value
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set the lookahead time in milliseconds. This allows the side-chain effect to react to notes before they are played.")
                    }
                    Label {
                        text: qsTr("Release (ms):")
                    }
                    SpinBox {
                        id: sideChainReleaseSpinBox
                        from: 0
                        to: 1000
                        editable: true
                        onValueChanged: sideChainService.sideChainRelease = value
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set the release time in milliseconds. This controls how long the side-chain effect remains active after the source event ends.")
                    }
                }
                // Targets
                Repeater {
                    model: sideChainService.sideChainTargetCount()
                    delegate: SideChainTargetDelegate {
                        targetIndex: index
                        Layout.fillWidth: true
                        enabled: sideChainEnabledCheckBox.checked
                    }
                    onItemAdded: (index, item) => {
                        _sideChainTargets.push(item);
                    }
                }
            }
        }
    }
}
