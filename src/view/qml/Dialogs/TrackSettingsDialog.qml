import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."
import "../Components"

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Settings for track") + ` '${editorService.trackName(trackSettingsModel.trackIndex)}'` + "</strong>"
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
        midiCcSettingsMiscellaneous.initialize();
        midiCcSettingsGeneric.initialize();
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
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        TrackSettingsDialogMidiInstrumentSettings {
            id: midiInstrumentSettings
            Layout.fillWidth: true
        }
        TrackSettingsDialogMidiCcSettingsPredefined {
            id: midiCcSettingsPredefined
            Layout.fillWidth: true
        }
        TrackSettingsDialogMidiCcSettingsGeneric {
            id: midiCcSettingsGeneric
            Layout.fillWidth: true
        }
        TrackSettingsDialogMidiSettingsMiscellaneous {
            id: midiCcSettingsMiscellaneous
            Layout.fillWidth: true
        }
    }
    Component.onCompleted: {
        visible = false;
        trackSettingsModel.instrumentDataReceived.connect(initialize);
    }
}
