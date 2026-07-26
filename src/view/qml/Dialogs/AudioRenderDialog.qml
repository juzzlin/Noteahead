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
    height: 580

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
                    Label {
                        text: qsTr("Quality:")
                    }
                    ComboBox {
                        id: qualityComboBox
                        Layout.fillWidth: true
                        readonly property var factors: [1, 2, 4]
                        model: [qsTr("Draft (1x)"), qsTr("Normal (2x)"), qsTr("High (4x)")]
                        currentIndex: Math.max(0, factors.indexOf(settingsService.renderOversampleFactor))
                        onActivated: settingsService.renderOversampleFactor = factors[currentIndex]
                    }
                }
            }

            GroupBox {
                title: qsTr("Export Options")
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    RowLayout {
                        spacing: 10
                        Layout.fillWidth: true
                        CheckBox {
                            id: normalizeCheckBox
                            text: qsTr("Normalize audio")
                            checked: settingsService.renderNormalizeEnabled
                            onToggled: settingsService.renderNormalizeEnabled = checked
                        }
                        Label {
                            text: qsTr("Target Level:")
                            enabled: normalizeCheckBox.checked
                        }
                        SpinBox {
                            id: normalizeLevelSpinBox
                            enabled: normalizeCheckBox.checked
                            from: -300
                            to: 0
                            stepSize: 1
                            value: settingsService.renderNormalizeLevel * 10
                            editable: true
                            onValueModified: settingsService.renderNormalizeLevel = value / 10
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

                    RowLayout {
                        spacing: 10
                        Layout.fillWidth: true
                        CheckBox {
                            id: trimCheckBox
                            text: qsTr("Trim duration")
                            checked: settingsService.renderTrimEnabled
                            onToggled: settingsService.renderTrimEnabled = checked
                        }
                        SpinBox {
                            id: trimMinSpinBox
                            enabled: trimCheckBox.checked
                            from: 0
                            to: 59
                            value: settingsService.renderTrimMinutes
                            editable: true
                            onValueModified: settingsService.renderTrimMinutes = value
                        }
                        Label {
                            text: qsTr("min")
                            enabled: trimCheckBox.checked
                        }
                        SpinBox {
                            id: trimSecSpinBox
                            enabled: trimCheckBox.checked
                            from: 0
                            to: 59
                            value: settingsService.renderTrimSeconds
                            editable: true
                            onValueModified: settingsService.renderTrimSeconds = value
                        }
                        Label {
                            text: qsTr("s")
                            enabled: trimCheckBox.checked
                        }
                    }

                    CheckBox {
                        id: analyzeCheckBox
                        text: qsTr("Analyze loudness (LUFS, LRA, dBTP)")
                        checked: settingsService.renderAnalyzeEnabled
                        onToggled: settingsService.renderAnalyzeEnabled = checked
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
