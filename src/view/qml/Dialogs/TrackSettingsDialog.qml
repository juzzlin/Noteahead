import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."
import "../Components"

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Settings for track %1: '%2'").arg(trackSettingsModel.trackIndex + 1).arg(editorService.trackName(trackSettingsModel.trackIndex))
    modal: true
    property bool _initializing: false
    readonly property string _tag: "TrackSettingsDialog"
    function setTrackIndex(trackIndex) {
        trackSettingsModel.trackIndex = trackIndex;
        trackSettingsModel.requestInstrumentData();
    }
    function initialize() {
        uiLogger.info(_tag, "Initializing");
        midiInstrumentSettings.initialize();
        midiCcSettingsPredefined.initialize();
        midiCcSettingsGeneric.initialize();
        midiSettingsMiscellaneous.initialize();
    }
    function saveSettings() {
        trackSettingsModel.applyAll();
        trackSettingsModel.save();
    }
    Timer {
        id: testSoundTimer
        interval: 125
        running: false
        repeat: false
        onTriggered: trackSettingsModel.requestTestSound(UiService._activeVelocity)
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
            TrackSettingsDialog_MidiInstrumentSettings {
                id: midiInstrumentSettings
                Layout.fillWidth: true
            }
            TrackSettingsDialog_MidiCcSettings_Standard {
                id: midiCcSettingsPredefined
                Layout.fillWidth: true
            }
            TrackSettingsDialog_MidiCcSettings_Custom {
                id: midiCcSettingsGeneric
                Layout.fillWidth: true
            }
            TrackSettingsDialog_MidiSettingsMiscellaneous {
                id: midiSettingsMiscellaneous
                Layout.fillWidth: true
            }
        }
        TabBar {
            id: tabBar
            width: parent.width
            TabButton {
                text: qsTr("MIDI Instrument")
            }
            TabButton {
                text: qsTr("Standard MIDI CC")
            }
            TabButton {
                text: qsTr("Custom MIDI CC")
            }
            TabButton {
                text: qsTr("Miscellaneous MIDI")
            }
        }
    }
    Component.onCompleted: {
        visible = false;
        trackSettingsModel.instrumentDataReceived.connect(initialize);
    }
}
