import QtCore
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Export song as a MIDI file") + "</strong>"
    modal: true
    property string outputFileName
    onOpened: {
        startPositionSpinBox.value = editorService.songPosition;
        endPositionSpinBox.value = editorService.songLength;
    }
    ScrollView {
        id: exportScrollView
        anchors.fill: parent
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        rightPadding: 10
        ColumnLayout {
            width: exportScrollView.availableWidth
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: fileNameTextField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Select output file")
                    text: rootItem.outputFileName
                }
                Button {
                    text: qsTr("Browse...")
                    onClicked: midiExportFileNameDialog.open()
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Label {
                    text: qsTr("Start position:")
                }
                SpinBox {
                    id: startPositionSpinBox
                    from: 0
                    to: editorService.songLength - 1
                }
                Label {
                    text: qsTr("End position:")
                }
                SpinBox {
                    id: endPositionSpinBox
                    from: 1
                    to: editorService.songLength
                }
            }
            GroupBox {
                title: qsTr("Options")
                Layout.fillWidth: true
                RowLayout {
                    anchors.fill: parent
                    spacing: 10
                    GroupBox {
                        title: qsTr("Events")
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ColumnLayout {
                            anchors.fill: parent
                            CheckBox {
                                id: exportBankCheckBox
                                text: qsTr("Export Bank")
                                checked: true
                            }
                            CheckBox {
                                id: exportProgramChangeCheckBox
                                text: qsTr("Export Program Change")
                                checked: true
                            }
                            CheckBox {
                                id: exportMidiCcCheckBox
                                text: qsTr("Export MIDI CC")
                                checked: true
                            }
                            CheckBox {
                                id: exportPitchBendCheckBox
                                text: qsTr("Export Pitch Bend")
                                checked: true
                            }
                        }
                    }
                    GroupBox {
                        title: qsTr("Channels")
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ColumnLayout {
                            anchors.fill: parent
                            CheckBox {
                                id: forceDrumChannel10CheckBox
                                text: qsTr("Force drum tracks as channel #10")
                                checked: settingsService.midiExportForceDrumChannel10
                                onCheckedChanged: settingsService.midiExportForceDrumChannel10 = checked
                            }
                            CheckBox {
                                id: autoAssignChannelsCheckBox
                                text: qsTr("Auto-assign channels")
                                checked: settingsService.midiExportAutoAssignChannels
                                onCheckedChanged: settingsService.midiExportAutoAssignChannels = checked
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }
        }
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("Cancel")
            onClicked: rootItem.reject()
        }
        Button {
            text: qsTr("Export")
            enabled: rootItem.outputFileName
            onClicked: {
                applicationService.exportMidiFile(rootItem.outputFileName, startPositionSpinBox.value, endPositionSpinBox.value, exportBankCheckBox.checked, exportProgramChangeCheckBox.checked, exportMidiCcCheckBox.checked, exportPitchBendCheckBox.checked, forceDrumChannel10CheckBox.checked, autoAssignChannelsCheckBox.checked);
                rootItem.accept();
            }
        }
    }
    FileDialog {
        id: midiExportFileNameDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("MIDI files") + ` (*${applicationService.midiFileExtension()})`]
        onAccepted: {
            uiLogger.info("_tag", "MIDI export target selected: " + selectedFile);
            rootItem.outputFileName = selectedFile;
        }
    }
}
