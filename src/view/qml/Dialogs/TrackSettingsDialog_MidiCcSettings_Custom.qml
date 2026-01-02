import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."

GroupBox {
    title: qsTr("Custom MIDI CC Settings")
    Layout.fillWidth: true
    Layout.fillHeight: true // Allow the GroupBox to grow vertically
    ColumnLayout {
        anchors.fill: parent // Make the layout fill the entire GroupBox
        spacing: 8
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true // This makes the scrollable area take up all available space
            clip: true
            ListView {
                id: midiCcListView
                model: trackSettingsModel.midiCcModel
                spacing: 4
                delegate: MidiCcSelector {
                    width: midiCcListView.width // Make delegate fill width
                    showRemoveButton: true
                    index: index
                    Component.onCompleted: {
                        initialize(model.enabled, model.controller, model.value);
                    }
                    onSettingsChanged: {
                        model.controller = controller();
                        model.value = value();
                        model.enabled = enabled();
                        trackSettingsModel.applyAll();
                    }
                    onRemoveRequested: {
                        trackSettingsModel.midiCcModel.removeMidiCcSetting(index);
                        trackSettingsModel.applyAll();
                    }
                }
            }
        }
        Button {
            text: qsTr("Add MIDI CC Setting")
            Layout.alignment: Qt.AlignRight
            onClicked: addMidiCcSettingDialog.open()
        }
    }
    AddMidiCcSettingDialog {
        id: addMidiCcSettingDialog
        anchors.centerIn: parent
        width: parent.width * 0.5
    }
}
