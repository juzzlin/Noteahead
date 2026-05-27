import QtCore
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Render audio") + "</strong>"
    modal: true
    width: 600
    height: 500

    property string outputFileName
    property string outputDirectory

    onOpened: {
        outputFileName = renderService.defaultRenderFileName;
        outputDirectory = renderService.defaultRenderDirectory;
    }

    ScrollView {
        id: renderScrollView
        anchors.fill: parent
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        rightPadding: 10

        ColumnLayout {
            width: renderScrollView.availableWidth
            spacing: 20

            GroupBox {
                title: qsTr("Render Mode")
                Layout.fillWidth: true
                ColumnLayout {
                    RadioButton {
                        id: masterMixRadioButton
                        text: qsTr("Master Mix")
                        checked: true
                    }
                    RadioButton {
                        id: individualTracksRadioButton
                        text: qsTr("Individual Tracks (WAV per track)")
                    }
                }
            }

            GroupBox {
                title: qsTr("Audio Settings")
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    Label {
                        text: qsTr("Sample Rate:")
                    }
                    ComboBox {
                        id: sampleRateComboBox
                        Layout.fillWidth: true
                        model: [44100, 48000, 88200, 96000, 176400, 192000]
                        currentIndex: model.indexOf(settingsService.renderSampleRate)
                        onActivated: settingsService.renderSampleRate = model[index]
                    }
                    Label {
                        text: qsTr("Bit Depth:")
                    }
                    ComboBox {
                        id: bitDepthComboBox
                        Layout.fillWidth: true
                        textRole: "text"
                        valueRole: "value"
                        model: [
                            { text: qsTr("16-bit PCM"), value: 0 },
                            { text: qsTr("24-bit PCM"), value: 1 },
                            { text: qsTr("32-bit PCM"), value: 2 },
                            { text: qsTr("32-bit Float"), value: 3 }
                        ]
                        currentIndex: settingsService.renderBitDepth
                        onActivated: settingsService.renderBitDepth = valueAt(index)
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: masterMixRadioButton.checked
                Label {
                    text: qsTr("Output file:")
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: fileNameTextField
                        Layout.fillWidth: true
                        text: rootItem.outputFileName
                    }
                    Button {
                        text: qsTr("Browse...")
                        onClicked: audioRenderFileNameDialog.open()
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: individualTracksRadioButton.checked
                Label {
                    text: qsTr("Output directory:")
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: directoryTextField
                        Layout.fillWidth: true
                        text: rootItem.outputDirectory
                    }
                    Button {
                        text: qsTr("Browse...")
                        onClicked: audioRenderDirectoryDialog.open()
                    }
                }
            }

            ProgressBar {
                Layout.fillWidth: true
                value: renderService.progress
                visible: renderService.isRendering
            }

            Label {
                text: renderService.isRendering ? qsTr("Rendering...") : ""
                visible: renderService.isRendering
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Cancel")
            onClicked: rootItem.reject()
            enabled: !renderService.isRendering
        }
        Button {
            text: qsTr("Render")
            enabled: !renderService.isRendering && (masterMixRadioButton.checked ? rootItem.outputFileName !== "" : rootItem.outputDirectory !== "")
            onClicked: {
                if (masterMixRadioButton.checked) {
                    renderService.renderMaster(rootItem.outputFileName);
                } else {
                    renderService.renderIndividualTracks(rootItem.outputDirectory);
                }
            }
        }
    }

    FileDialog {
        id: audioRenderFileNameDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("WAV files") + " (*.wav)"]
        onAccepted: {
            rootItem.outputFileName = utilService.urlToLocalFile(selectedFile);
        }
    }

    FolderDialog {
        id: audioRenderDirectoryDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        onAccepted: {
            rootItem.outputDirectory = utilService.urlToLocalFile(selectedFolder);
        }
    }

    Connections {
        target: renderService
        function onRenderingFinished(success, message) {
            if (success) {
                uiLogger.info("Render", qsTr("Rendering finished successfully."));
                rootItem.accept();
            } else {
                uiLogger.error("Render", qsTr("Rendering failed: ") + message);
                applicationService.requestAlertDialog(qsTr("Rendering failed: ") + message);
            }
        }
    }
}
