import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

Dialog {
    id: rootItem
    modal: true
    title: "<strong>" + qsTr("Add MIDI CC Setting") + "</strong>"
    standardButtons: Dialog.Ok | Dialog.Cancel
    ColumnLayout {
        width: parent.width
        spacing: 10
        Label {
            text: qsTr("Controller:")
        }
        MidiCcComboBox {
            id: controllerComboBox
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("Value:")
        }
        SpinBox {
            id: valueSpinBox
            Layout.fillWidth: true
            from: propertyService.minValue(controllerComboBox.currentValue)
            to: propertyService.maxValue(controllerComboBox.currentValue)
            value: 0
            editable: true
        }
    }
    onAccepted: {
        trackSettingsModel.midiCcModel.addMidiCcSetting(controllerComboBox.currentValue, valueSpinBox.value);
        trackSettingsModel.applyAll();
    }
}
