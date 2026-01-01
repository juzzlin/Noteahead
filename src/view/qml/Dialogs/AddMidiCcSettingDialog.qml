import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."

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
        ComboBox {
            id: controllerComboBox
            Layout.fillWidth: true
            model: propertyService.availableMidiControllers
            textRole: "name"
            valueRole: "number"
            currentIndex: 0
            editable: true
            delegate: ItemDelegate {
                text: modelData.name
                highlighted: controllerComboBox.highlightedIndex === index
                Universal.theme: Universal.Dark
            }
            popup: Popup {
                y: controllerComboBox.height - 1
                width: controllerComboBox.width
                implicitHeight: contentItem.implicitHeight > 300 ? 300 : contentItem.implicitHeight
                padding: 1
                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: controllerComboBox.popup.visible ? controllerComboBox.delegateModel : null
                    currentIndex: controllerComboBox.highlightedIndex

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AlwaysOn
                    }
                }
                background: Rectangle {
                    color: "#303030"
                    border.color: "#606060"
                }
            }
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
