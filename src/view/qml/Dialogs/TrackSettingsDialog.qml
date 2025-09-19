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
    property var _midiCcSelectors: []
    property bool _initializing: false
    readonly property string _tag: "TrackSettingsDialog"
    function setTrackIndex(trackIndex) {
        trackSettingsModel.trackIndex = trackIndex;
        trackSettingsModel.requestInstrumentData();
    }
    function initialize() {
        uiLogger.info(_tag, "Initializing");
        portNameDropdown.setSelected(trackSettingsModel.portName);
        channelDropdown.setSelected(trackSettingsModel.channel + 1);
        enableBankCheckbox.checked = trackSettingsModel.bankEnabled;
        bankLsbSpinBox.value = trackSettingsModel.bankLsb;
        bankMsbSpinBox.value = trackSettingsModel.bankMsb;
        swapBankByteOrderCheckBox.checked = trackSettingsModel.bankByteOrderSwapped;
        enableCutoffCheckbox.checked = trackSettingsModel.cutoffEnabled;
        cutoffSpinBox.value = trackSettingsModel.cutoff;
        enablePatchCheckbox.checked = trackSettingsModel.patchEnabled;
        patchSpinBox.value = trackSettingsModel.patch;
        enablePanCheckbox.checked = trackSettingsModel.panEnabled;
        panSpinBox.value = trackSettingsModel.pan;
        enableVolumeCheckbox.checked = trackSettingsModel.volumeEnabled;
        volumeSpinBox.value = trackSettingsModel.volume;
        sendMidiClockCheckbox.checked = trackSettingsModel.sendMidiClock;
        delaySpinBox.value = trackSettingsModel.delay;
        transposeSpinBox.value = trackSettingsModel.transpose;
        initializeMidiCcSelectors();
    }
    function initializeMidiCcSelectors() {
        for (const midiCcSelector of _midiCcSelectors) {
            midiCcSelector.initialize(trackSettingsModel.midiCcEnabled(midiCcSelector.index), trackSettingsModel.midiCcController(midiCcSelector.index), trackSettingsModel.midiCcValue(midiCcSelector.index));
        }
    }
    function saveSettings() {
        trackSettingsModel.applyAll();
        trackSettingsModel.save();
    }
    function _requestApplyAll() {
        trackSettingsModel.applyAll();
    }
    function _requestApplyMidiCc() {
        trackSettingsModel.applyMidiCc();
    }
    function _requestTestSound() {
        if (rootItem.visible) {
            _requestApplyAll();
            testSoundTimer.restart();
        }
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
            Layout.fillWidth: true
        }
        TrackSettingsDialogMidiCcSettingsPredefined {
            Layout.fillWidth: true
        }
        TrackSettingsDialogMidiCcSettingsGeneric {
            Layout.fillWidth: true
        }
        TrackSettingsDialogMidiSettingsMiscellaneous {
            Layout.fillWidth: true
        }
    }
    Component.onCompleted: {
        visible = false;
        trackSettingsModel.instrumentDataReceived.connect(initialize);
    }
}
