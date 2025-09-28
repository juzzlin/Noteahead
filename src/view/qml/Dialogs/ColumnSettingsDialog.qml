import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."
import "../Components"

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Column settings for track %1, column %2").arg(columnSettingsModel.trackIndex + 1).arg(columnSettingsModel.columnIndex + 1) + "</strong>"
    modal: true
    function initialize() {
        midiEffects.initialize();
    }
    function saveSettings() {
        columnSettingsModel.save();
    }
    function setColumn(trackIndex, columnIndex) {
        columnSettingsModel.trackIndex = trackIndex;
        columnSettingsModel.columnIndex = columnIndex;
        columnSettingsModel.requestData();
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                saveSettings();
                rootItem.accepted();
            }
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: rootItem.rejected()
        }
    }
    Column {
        anchors.fill: parent
        spacing: 10
        ColumnSettingsDialog_MidiEffects {
            id: midiEffects
            Layout.fillWidth: true
        }
    }

    Component.onCompleted: {
        columnSettingsModel.dataReceived.connect(initialize);
    }
}
