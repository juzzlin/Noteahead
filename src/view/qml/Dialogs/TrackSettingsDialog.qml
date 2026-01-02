import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."
import "../Components"

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Track settings for track %1: '%2'").arg(trackSettingsModel.trackIndex + 1).arg(editorService.trackName(trackSettingsModel.trackIndex)) + "</strong>"
    modal: true
    property bool _initializing: false
    readonly property string _tag: "TrackSettingsDialog"
    function setTrackIndex(trackIndex) {
        trackSettingsModel.trackIndex = trackIndex;
        trackSettingsModel.requestInstrumentData();
    }
    function initialize() {
        uiLogger.info(_tag, "Initializing");
        instrumentSettings.initialize();
        timingSettings.initialize();
        midiEffects.initialize();
        tabBar.currentIndex = 0;
    }
    function saveSettings() {
        trackSettingsModel.applyAll();
        trackSettingsModel.save();
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                saveSettings();
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Save current settings")
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: rootItem.rejected()
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Don't save current settings")
        }
    }
    Column {
        anchors.fill: parent
        spacing: 10
        StackLayout {
            height: parent.height - tabBar.height
            width: parent.width
            currentIndex: tabBar.currentIndex
            TrackSettingsDialog_InstrumentSettings {
                id: instrumentSettings
                Layout.fillWidth: true
            }
            TrackSettingsDialog_TimingSettings {
                id: timingSettings
                Layout.fillWidth: true
            }
            TrackSettingsDialog_MidiEffects {
                id: midiEffects
                Layout.fillWidth: true
            }
            TrackSettingsDialog_MidiCcSettings_Custom {
                id: midiCcSettingsGeneric
                Layout.fillWidth: true
            }
        }
        TabBar {
            id: tabBar
            width: parent.width
            TabButton {
                text: qsTr("Instrument")
            }
            TabButton {
                text: qsTr("Timing")
            }
            TabButton {
                text: qsTr("MIDI Effects")
            }
            TabButton {
                text: qsTr("MIDI CC (Custom)")
            }
        }
    }
    Component.onCompleted: {
        visible = false;
        trackSettingsModel.instrumentDataReceived.connect(initialize);
    }
}
