// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

import QtCore
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal
import QtQuick.Dialogs
import QtQuick.Layouts
import Noteahead 1.0
import ".."
import "../Components"

AnimatedDialog {
    id: rootItem
    title: "<strong>" + qsTr("Render audio") + "</strong>"
    modal: true
    width: 600
    height: 660

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
                                if (!rootItem.customFileName)
                                    rootItem.outputFileName = renderService.defaultRenderFileName;
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
                                if (!rootItem.customFileName)
                                    rootItem.outputFileName = renderService.defaultRenderFileName;
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
                                {
                                    text: qsTr("16-bit PCM"),
                                    value: 0
                                },
                                {
                                    text: qsTr("24-bit PCM"),
                                    value: 1
                                }
                            ] : [
                                {
                                    text: qsTr("16-bit PCM"),
                                    value: 0
                                },
                                {
                                    text: qsTr("24-bit PCM"),
                                    value: 1
                                },
                                {
                                    text: qsTr("32-bit PCM"),
                                    value: 2
                                },
                                {
                                    text: qsTr("32-bit Float"),
                                    value: 3
                                }
                            ]
                            currentIndex: renderSettingsModel.bitDepth
                            onActivated: {
                                renderSettingsModel.bitDepth = valueAt(index);
                                if (!rootItem.customFileName)
                                    rootItem.outputFileName = renderService.defaultRenderFileName;
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
                ColumnLayout {
                    width: parent.width
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
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
                                Keys.onReturnPressed: focus = false
                                textFromValue: function (value, locale) {
                                    return Number(value / 10).toLocaleString(locale, 'f', 1);
                                }
                                valueFromText: function (text, locale) {
                                    return Number.fromLocaleString(locale, text) * 10;
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
                                Keys.onReturnPressed: focus = false
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
                                Keys.onReturnPressed: focus = false
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

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        // Column 1: Fade out
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 6

                            CheckBox {
                                id: fadeOutCheckBox
                                text: qsTr("Fade out")
                                checked: renderSettingsModel.fadeOutEnabled
                                onToggled: renderSettingsModel.fadeOutEnabled = checked
                            }

                            SpinBox {
                                id: fadeOutSecSpinBox
                                Layout.fillWidth: true
                                enabled: fadeOutCheckBox.checked
                                from: 0
                                to: 59
                                value: renderSettingsModel.fadeOutSeconds
                                editable: true
                                onValueModified: renderSettingsModel.fadeOutSeconds = value
                                Keys.onReturnPressed: focus = false
                            }

                            Label {
                                text: qsTr("s")
                                enabled: fadeOutCheckBox.checked
                            }

                            SpinBox {
                                id: fadeOutTenthsSpinBox
                                Layout.fillWidth: true
                                enabled: fadeOutCheckBox.checked
                                from: 0
                                to: 9
                                value: renderSettingsModel.fadeOutTenths
                                editable: true
                                onValueModified: renderSettingsModel.fadeOutTenths = value
                                Keys.onReturnPressed: focus = false
                            }

                            Label {
                                text: qsTr("/10 s")
                                enabled: fadeOutCheckBox.checked
                            }
                        }

                        // Column 2: Silence
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 6

                            CheckBox {
                                id: silenceCheckBox
                                text: qsTr("Silence")
                                checked: renderSettingsModel.silenceEnabled
                                onToggled: renderSettingsModel.silenceEnabled = checked
                            }

                            SpinBox {
                                id: silenceSecSpinBox
                                Layout.fillWidth: true
                                enabled: silenceCheckBox.checked
                                from: 0
                                to: 59
                                value: renderSettingsModel.silenceSeconds
                                editable: true
                                onValueModified: renderSettingsModel.silenceSeconds = value
                                Keys.onReturnPressed: focus = false
                            }

                            Label {
                                text: qsTr("s")
                                enabled: silenceCheckBox.checked
                            }

                            SpinBox {
                                id: silenceTenthsSpinBox
                                Layout.fillWidth: true
                                enabled: silenceCheckBox.checked
                                from: 0
                                to: 9
                                value: renderSettingsModel.silenceTenths
                                editable: true
                                onValueModified: renderSettingsModel.silenceTenths = value
                                Keys.onReturnPressed: focus = false
                            }

                            Label {
                                text: qsTr("/10 s")
                                enabled: silenceCheckBox.checked
                            }
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

                    Label {
                        text: qsTr("Title")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.exportMetadataTitle
                        placeholderText: editorService.songMetadataTitle
                        onTextEdited: editorService.exportMetadataTitle = text
                        Keys.onReturnPressed: focus = false
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Written into the rendered audio file. Leave empty to use the song's own value from Song > Metadata, shown greyed out.")
                    }

                    Label {
                        text: qsTr("Artist")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.exportMetadataArtist
                        placeholderText: editorService.songMetadataArtist
                        onTextEdited: editorService.exportMetadataArtist = text
                        Keys.onReturnPressed: focus = false
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Written into the rendered audio file. Leave empty to use the song's own value from Song > Metadata, shown greyed out.")
                    }

                    Label {
                        text: qsTr("Album")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.exportMetadataAlbum
                        placeholderText: editorService.songMetadataAlbum
                        onTextEdited: editorService.exportMetadataAlbum = text
                        Keys.onReturnPressed: focus = false
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Written into the rendered audio file. Leave empty to use the song's own value from Song > Metadata, shown greyed out.")
                    }

                    Label {
                        text: qsTr("Date")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.exportMetadataDate
                        placeholderText: editorService.songMetadataDate
                        onTextEdited: editorService.exportMetadataDate = text
                        Keys.onReturnPressed: focus = false
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Written into the rendered audio file. Leave empty to use the song's own value from Song > Metadata, shown greyed out.")
                    }

                    Label {
                        text: qsTr("Genre")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.exportMetadataGenre
                        placeholderText: editorService.songMetadataGenre
                        onTextEdited: editorService.exportMetadataGenre = text
                        Keys.onReturnPressed: focus = false
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Written into the rendered audio file. Leave empty to use the song's own value from Song > Metadata, shown greyed out.")
                    }

                    Label {
                        text: qsTr("Track Number")
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: editorService.exportMetadataTrackNumber
                        placeholderText: editorService.songMetadataTrackNumber
                        onTextEdited: editorService.exportMetadataTrackNumber = text
                        Keys.onReturnPressed: focus = false
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Written into the rendered audio file. Leave empty to use the song's own value from Song > Metadata, shown greyed out.")
                    }

                    Label {
                        text: qsTr("Comment")
                    }
                    TextField {
                        Layout.fillWidth: true
                        Layout.columnSpan: 5
                        text: editorService.exportMetadataComment
                        placeholderText: editorService.songMetadataComment
                        onTextEdited: editorService.exportMetadataComment = text
                        Keys.onReturnPressed: focus = false
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Written into the rendered audio file. Leave empty to use the song's own value from Song > Metadata, shown greyed out.")
                    }

                    Label {
                        Layout.columnSpan: 6
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: themeService.accentColor
                        text: qsTr("These tag the rendered audio file. An empty field uses the song's own value from Song > Metadata, shown greyed out.")
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
                        Keys.onReturnPressed: focus = false
                    }
                    AppButton {
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
                        Keys.onReturnPressed: focus = false
                    }
                    AppButton {
                        text: qsTr("Browse...")
                        onClicked: audioRenderDirectoryDialog.open()
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            onClicked: rootItem.reject()
            enabled: !renderService.isRendering
        }
        AppButton {
            text: qsTr("Render")
            implicitWidth: Constants.defaultButtonWidth
            enabled: !renderService.isRendering && (masterMixRadioButton.checked ? rootItem.outputFileName !== "" : rootItem.outputDirectory !== "")
            onClicked: {
                if (masterMixRadioButton.checked) {
                    renderService.renderMaster(rootItem.outputFileName);
                } else {
                    renderService.renderIndividualTracks(rootItem.outputDirectory);
                }
                UiService.requestRenderProgressDialog();
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
