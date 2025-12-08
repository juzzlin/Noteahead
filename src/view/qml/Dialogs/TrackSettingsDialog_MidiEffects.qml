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
        sideChainEnabledCheckBox.checked = trackSettingsModel.sideChainEnabled;
        sideChainSourceTrackSpinBox.value = trackSettingsModel.sideChainSourceTrack + 1;
        sideChainSourceColumnSpinBox.value = trackSettingsModel.sideChainSourceColumn + 1;
        sideChainLookaheadSpinBox.value = trackSettingsModel.sideChainLookahead;
        sideChainReleaseSpinBox.value = trackSettingsModel.sideChainRelease;
        for (const sideChainTarget of _sideChainTargets) {
            sideChainTarget.initialize(trackSettingsModel.sideChainTargetEnabled(sideChainTarget.targetIndex), trackSettingsModel.sideChainTargetController(sideChainTarget.targetIndex), trackSettingsModel.sideChainTargetTargetValue(sideChainTarget.targetIndex), trackSettingsModel.sideChainTargetReleaseValue(sideChainTarget.targetIndex));
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
                    onCheckedChanged: trackSettingsModel.sideChainEnabled = checked
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
                        onValueChanged: trackSettingsModel.sideChainSourceTrack = value - 1
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Source column:")
                    }
                    SpinBox {
                        id: sideChainSourceColumnSpinBox
                        from: 1
                        to: 255
                        editable: true
                        onValueChanged: trackSettingsModel.sideChainSourceColumn = value - 1
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Lookahead (ms):")
                    }
                    SpinBox {
                        id: sideChainLookaheadSpinBox
                        from: 0
                        to: 1000
                        editable: true
                        onValueChanged: trackSettingsModel.sideChainLookahead = value
                        Layout.fillWidth: true
                    }
                    Label {
                        text: qsTr("Release (ms):")
                    }
                    SpinBox {
                        id: sideChainReleaseSpinBox
                        from: 0
                        to: 1000
                        editable: true
                        onValueChanged: trackSettingsModel.sideChainRelease = value
                        Layout.fillWidth: true
                    }
                }
                // Targets
                Repeater {
                    model: trackSettingsModel.sideChainTargetCount()
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
