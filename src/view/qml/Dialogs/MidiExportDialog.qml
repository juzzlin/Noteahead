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
    ColumnLayout {
        anchors.fill: parent
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
                applicationService.exportMidiFile(rootItem.outputFileName, startPositionSpinBox.value, endPositionSpinBox.value);
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
