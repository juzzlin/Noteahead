import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

RowLayout {
    id: root
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    property bool isSaving: false
    property var presetNameDialog

    Layout.fillWidth: true

    Label {
        text: isSaving ? qsTr("Select slot to save preset...") : qsTr("A general purpose 6-voice synthesizer")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.fillWidth: true
        Layout.topMargin: 10
    }

    Label {
        text: qsTr("Bank:")
    }
    ComboBox {
        id: bankCombo
        model: [qsTr("Factory"), qsTr("User")]
        currentIndex: synthController.currentBank
        onActivated: index => {
            synthController.currentBank = index;
        }
    }

    Label {
        text: qsTr("Preset:")
    }
    ComboBox {
        id: presetCombo
        implicitWidth: 300
        model: synthController.currentBank === 0 ? synthController.presetNames : synthController.userPresetNames
        currentIndex: synthController.currentPresetIndex
        onActivated: index => {
            if (isSaving) {
                synthController.currentPresetIndex = index;
                presetNameDialog.setTitle(qsTr("Save User Preset"));
                presetNameDialog.text = synthController.userPresetNames[index].split(": ")[1];
                presetNameDialog.open();
            } else {
                synthController.currentPresetIndex = index;
                synthController.loadPreset(index);
            }
        }

        delegate: ItemDelegate {
            width: 235
            highlighted: presetCombo.highlightedIndex === index
            contentItem: Label {
                text: modelData
                color: presetCombo.currentIndex === index ? themeService.accentColor : "white"
                font.bold: presetCombo.currentIndex === index
                elide: Text.ElideRight
                verticalAlignment: Image.AlignVCenter
            }
            background: Rectangle {
                color: highlighted ? "#333" : "transparent"
            }
            onClicked: {
                presetCombo.currentIndex = index;
                presetCombo.activated(index);
                presetCombo.popup.close();
            }
        }

        popup: Popup {
            y: presetCombo.height
            x: (980 - width) / 2 - parent.x - 15
            width: 980
            implicitHeight: 500
            padding: 5

            contentItem: GridView {
                clip: true
                model: presetCombo.delegateModel
                cellWidth: 240
                cellHeight: 40
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }
            }

            background: Rectangle {
                color: "#252525"
                border.color: themeService.accentColor
                radius: 4
            }
        }
    }

    Button {
        text: qsTr("Save")
        highlighted: isSaving
        onClicked: {
            root.isSaving = true;
            synthController.currentBank = 1;
            presetCombo.popup.open();
        }
    }

    Button {
        text: qsTr("Reset")
        onClicked: {
            root.isSaving = false;
            synthController.reset();
        }
    }
}
