import QtCore
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

AnimatedDialog {
    id: rootItem
    title: "<strong>" + qsTr("Render audio") + "</strong>"
    modal: true
    width: 600
    height: 580

    property string outputFileName
    property string outputDirectory
    property bool customFileName: false

    onOpened: {
        customFileName = false;
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
                        text: qsTr("Individual Tracks (one file per track)")
                    }
                }
            }

            GroupBox {
                title: qsTr("Audio Settings")
                Layout.fillWidth: true
                RowLayout {
                    width: parent.width
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        Label {
                            text: qsTr("Format:")
                        }
                        ComboBox {
                            id: formatComboBox
                            Layout.fillWidth: true
                            model: [qsTr("WAV"), qsTr("FLAC")]
                            currentIndex: renderSettingsModel.format
                            onActivated: {
                                renderSettingsModel.format = index;
                                if (!rootItem.customFileName) rootItem.outputFileName = renderService.defaultRenderFileName;
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        Label {
                            text: qsTr("Sample Rate:")
                        }
                        ComboBox {
                            id: sampleRateComboBox
                            Layout.fillWidth: true
                            model: [44100, 48000, 88200, 96000, 176400, 192000]
                            currentIndex: model.indexOf(renderSettingsModel.sampleRate)
                            onActivated: {
                                renderSettingsModel.sampleRate = model[index];
                                if (!rootItem.customFileName) rootItem.outputFileName = renderService.defaultRenderFileName;
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        Label {
                            text: qsTr("Bit Depth:")
                        }
                        ComboBox {
                            id: bitDepthComboBox
                            Layout.fillWidth: true
                            textRole: "text"
                            valueRole: "value"
                            model: renderSettingsModel.format === 1 ? [
                                { text: qsTr("16-bit PCM"), value: 0 },
                                { text: qsTr("24-bit PCM"), value: 1 }
                            ] : [
                                { text: qsTr("16-bit PCM"), value: 0 },
                                { text: qsTr("24-bit PCM"), value: 1 },
                                { text: qsTr("32-bit PCM"), value: 2 },
                                { text: qsTr("32-bit Float"), value: 3 }
                            ]
                            currentIndex: renderSettingsModel.bitDepth
                            onActivated: {
                                renderSettingsModel.bitDepth = valueAt(index);
                                if (!rootItem.customFileName) rootItem.outputFileName = renderService.defaultRenderFileName;
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        Label {
                            text: qsTr("Quality:")
                        }
                        ComboBox {
                            id: qualityComboBox
                            Layout.fillWidth: true
                            readonly property var factors: [1, 2, 4]
                            model: [qsTr("Draft (1x)"), qsTr("Normal (2x)"), qsTr("High (4x)")]
                            currentIndex: Math.max(0, factors.indexOf(renderSettingsModel.oversampleFactor))
                            onActivated: index => renderSettingsModel.oversampleFactor = factors[index]
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Export Options")
                Layout.fillWidth: true
                RowLayout {
                    width: parent.width
                    spacing: 12

                    // Column 1: Normalize
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        CheckBox {
                            id: normalizeCheckBox
                            text: qsTr("Normalize audio")
                            checked: renderSettingsModel.normalizeEnabled
                            onToggled: renderSettingsModel.normalizeEnabled = checked
                        }

                        Label {
                            text: qsTr("Target Level:")
                            enabled: normalizeCheckBox.checked
                        }

                        SpinBox {
                            id: normalizeLevelSpinBox
                            Layout.fillWidth: true
                            enabled: normalizeCheckBox.checked
                            from: -300
                            to: 0
                            stepSize: 1
                            value: renderSettingsModel.normalizeLevelTenthsDb
                            editable: true
                            onValueModified: renderSettingsModel.normalizeLevelTenthsDb = value
                            textFromValue: function(value, locale) {
                                return Number(value / 10).toLocaleString(locale, 'f', 1)
                            }
                            valueFromText: function(text, locale) {
                                return Number.fromLocaleString(locale, text) * 10
                            }
                        }

                        Label {
                            text: "dB"
                            enabled: normalizeCheckBox.checked
                        }
                    }

                    // Column 2: Trim duration
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        CheckBox {
                            id: trimCheckBox
                            text: qsTr("Trim duration")
                            checked: renderSettingsModel.trimEnabled
                            onToggled: renderSettingsModel.trimEnabled = checked
                        }

                        SpinBox {
                            id: trimMinSpinBox
                            Layout.fillWidth: true
                            enabled: trimCheckBox.checked
                            from: 0
                            to: 59
                            value: renderSettingsModel.trimMinutes
                            editable: true
                            onValueModified: renderSettingsModel.trimMinutes = value
                        }

                        Label {
                            text: qsTr("min")
                            enabled: trimCheckBox.checked
                        }

                        SpinBox {
                            id: trimSecSpinBox
                            Layout.fillWidth: true
                            enabled: trimCheckBox.checked
                            from: 0
                            to: 59
                            value: renderSettingsModel.trimSeconds
                            editable: true
                            onValueModified: renderSettingsModel.trimSeconds = value
                        }

                        Label {
                            text: qsTr("s")
                            enabled: trimCheckBox.checked
                        }
                    }

                    // Column 3: Analyze loudness
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        CheckBox {
                            id: analyzeCheckBox
                            text: qsTr("Analyze loudness (LUFS, LRA, dBTP)")
                            checked: renderSettingsModel.analyzeEnabled
                            onToggled: renderSettingsModel.analyzeEnabled = checked
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Metadata")
                Layout.fillWidth: true
                GridLayout {
                    columns: 6
                    rowSpacing: 6
                    columnSpacing: 12
                    width: parent.width

                    Label { text: qsTr("Title") }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataTitle
                        onTextEdited: editorService.songMetadataTitle = text
                    }

                    Label { text: qsTr("Artist") }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataArtist
                        onTextEdited: editorService.songMetadataArtist = text
                    }

                    Label { text: qsTr("Album") }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataAlbum
                        onTextEdited: editorService.songMetadataAlbum = text
                    }

                    Label { text: qsTr("Date") }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataDate
                        onTextEdited: editorService.songMetadataDate = text
                    }

                    Label { text: qsTr("Genre") }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataGenre
                        onTextEdited: editorService.songMetadataGenre = text
                    }

                    Label { text: qsTr("Track Number") }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.songMetadataTrackNumber
                        onTextEdited: editorService.songMetadataTrackNumber = text
                    }

                    Label { text: qsTr("Comment") }
                    TextField {
                        Layout.fillWidth: true
                        Layout.columnSpan: 5
                        text: editorService.songMetadataComment
                        onTextEdited: editorService.songMetadataComment = text
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
                        onTextEdited: {
                            rootItem.outputFileName = text;
                            rootItem.customFileName = true;
                        }
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
                        onTextEdited: {
                            rootItem.outputDirectory = text;
                        }
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
        nameFilters: [qsTr("Audio files") + " (*.wav *.flac)", qsTr("WAV files") + " (*.wav)", qsTr("FLAC files") + " (*.flac)"]
        onAccepted: {
            rootItem.outputFileName = utilService.urlToLocalFile(selectedFile);
            rootItem.customFileName = true;
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
                if (message !== "") {
                    applicationService.requestLoudnessReportDialog(message);
                } else {
                    uiLogger.info("Render", qsTr("Rendering finished successfully."));
                }
                rootItem.accept();
            } else {
                uiLogger.error("Render", qsTr("Rendering failed: ") + message);
                applicationService.requestAlertDialog(qsTr("Rendering failed: ") + message);
            }
        }
    }
}
