import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import Noteahead 1.0

Dialog {
    id: rootItem
    title: qsTr("Import MIDI file")
    modal: true
    standardButtons: DialogButtonBox.Open | DialogButtonBox.Cancel
    visible: false

    FileDialog {
        id: fileDialog
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("MIDI files") + " (*.mid *.midi)"]
        currentFolder: {
            const lastDir = applicationService.lastImportDirectory();
            return lastDir ? "file://" + lastDir : StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0];
        }
        onAccepted: {
            const path = selectedFile.toString();
            const lastSlash = path.lastIndexOf("/");
            if (lastSlash !== -1) {
                const dir = path.substring(7, lastSlash); // Remove file:// prefix and keep directory
                applicationService.setLastImportDirectory(dir);
            }
            applicationService.importMidiFile(selectedFile, importModeComboBox.currentIndex, patternLengthSpinBox.value, quantizeNoteOnCheckBox.checked, quantizeNoteOffCheckBox.checked)
            rootItem.close()
        }
    }

    onAccepted: {
        fileDialog.open()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 20

        Label {
            text: qsTr("Select import options:")
            font.bold: true
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 10
            Label {
                text: qsTr("Import mode:")
            }
            ComboBox {
                id: importModeComboBox
                model: [qsTr("Clear project and import"), qsTr("Merge into project")]
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Choose whether to replace the current project or merge MIDI data into it.")
            }
        }

        RowLayout {
            spacing: 10
            Label {
                text: qsTr("New pattern length:")
            }
            SpinBox {
                id: patternLengthSpinBox
                from: 4
                to: 999
                value: 64
                editable: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set the default length (number of lines) for newly created patterns.")
            }
        }

        CheckBox {
            id: quantizeNoteOnCheckBox
            text: qsTr("Quantize Note ON")
            checked: false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("If enabled, Note ON events will be aligned exactly to the line timestamp (zero delay).")
        }

        CheckBox {
            id: quantizeNoteOffCheckBox
            text: qsTr("Quantize Note OFF")
            checked: false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("If enabled, Note OFF events will be aligned exactly to the line timestamp (zero delay).")
        }

        Label {
            text: qsTr("Note: Importing will create new tracks and patterns as needed.")
            font.italic: true
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
