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
    visible: false

    property string inputFileName

    onOpened: {
        const initialPath = applicationService.initialFilePath();
        if (initialPath) {
            rootItem.inputFileName = initialPath;
        } else {
            const selectedFile = applicationService.selectedFile();
            if (applicationService.isMidiFile(selectedFile)) {
                rootItem.inputFileName = selectedFile;
            }
        }
    }

    FileDialog {
        id: fileDialog
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("MIDI files") + " (*.mid *.midi)"]
        currentFolder: {
            const lastDir = applicationService.lastImportDirectory();
            return lastDir ? "file://" + lastDir : StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0];
        }
        onAccepted: {
            rootItem.inputFileName = selectedFile.toString();
        }
    }

    ScrollView {
        id: importScrollView
        anchors.fill: parent
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        rightPadding: 10
        ColumnLayout {
            width: importScrollView.availableWidth
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: fileNameTextField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Select MIDI file to import")
                    text: rootItem.inputFileName
                }
                Button {
                    text: qsTr("Browse...")
                    onClicked: fileDialog.open()
                }
            }

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

            GroupBox {
                title: qsTr("Options")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
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

                    CheckBox {
                        id: connectMidiPortsCheckBox
                        text: qsTr("Connect MIDI ports")
                        checked: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Automatically try to match track names with available MIDI output ports.")
                    }
                }
            }

            Label {
                text: qsTr("Note: Importing will create new tracks and patterns as needed.")
                font.italic: true
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Cancel")
            onClicked: rootItem.reject()
        }
        Button {
            text: qsTr("Import")
            enabled: rootItem.inputFileName
            onClicked: {
                const path = rootItem.inputFileName.toString();
                const lastSlash = path.lastIndexOf("/");
                if (lastSlash !== -1) {
                    const dir = path.substring(7, lastSlash); // Remove file:// prefix and keep directory
                    applicationService.setLastImportDirectory(dir);
                }
                applicationService.importMidiFile(rootItem.inputFileName, importModeComboBox.currentIndex, patternLengthSpinBox.value, quantizeNoteOnCheckBox.checked, quantizeNoteOffCheckBox.checked, connectMidiPortsCheckBox.checked)
                rootItem.accept()
            }
        }
    }
}
