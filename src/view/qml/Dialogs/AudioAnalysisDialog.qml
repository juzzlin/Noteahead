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
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts 1.15
import Noteahead 1.0
import ".."
import "../Components"

// Measures two audio files and draws what separates them. Each file's bands are relative to its own
// midrange, so the difference is a balance difference whatever either was mastered to, and it reads
// as the equalizer move that would take one to the other.
AnimatedDialog {
    id: root
    title: "<strong>" + qsTr("Analyse audio files") + "</strong>"
    modal: true
    width: parent ? parent.width * Constants.largeDialogScale : 900
    height: parent ? parent.height * Constants.largeDialogScale : 640

    readonly property color leftColor: "#4CAF50"
    readonly property color rightColor: "#2196F3"

    function refresh() {
        audioAnalysisController.updateRenderer(compareRenderer);
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Swap")
            implicitWidth: Constants.defaultButtonWidth
            enabled: audioAnalysisController.hasLeft || audioAnalysisController.hasRight
            onClicked: audioAnalysisController.swap()
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
        }
        AppButton {
            text: qsTr("Save report...")
            implicitWidth: Constants.defaultButtonWidth
            enabled: (audioAnalysisController.hasLeft || audioAnalysisController.hasRight) && !audioAnalysisController.isAnalyzing
            onClicked: {
                // Named after the two files, and set as the dialog opens: a name bound as a property
                // is resolved against a folder that does not hold it yet, which the dialog rejects
                // out loud.
                saveReportDialog.selectedFile = saveReportDialog.currentFolder + "/" + audioAnalysisController.defaultReportFileName();
                saveReportDialog.open();
            }
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
        }
        AppButton {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    contentItem: ColumnLayout {
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            AudioAnalysisDialog_Side {
                id: leftSide
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                label: qsTr("A")
                accentColor: root.leftColor
                fileName: audioAnalysisController.leftFileName
                report: audioAnalysisController.leftReport
                busy: audioAnalysisController.isAnalyzing && !audioAnalysisController.hasLeft
                onOpenRequested: openLeftDialog.open()
                onClearRequested: audioAnalysisController.clearLeft()
            }

            // The two sides are read against each other, so the eye needs the boundary.
            Rectangle {
                Layout.fillHeight: true
                width: 1
                color: "#3a3a3a"
            }

            AudioAnalysisDialog_Side {
                id: rightSide
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                label: qsTr("B")
                accentColor: root.rightColor
                fileName: audioAnalysisController.rightFileName
                report: audioAnalysisController.rightReport
                busy: audioAnalysisController.isAnalyzing && !audioAnalysisController.hasRight
                onOpenRequested: openRightDialog.open()
                onClearRequested: audioAnalysisController.clearRight()
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Third-octave balance. Bars are B - A: above the line B has more, and that is the cut B needs to match A.")
            color: "#aaaaaa"
            font.pixelSize: 11
            wrapMode: Text.Wrap
        }

        SpectrumCompareRenderer {
            id: compareRenderer
            Layout.fillWidth: true
            Layout.preferredHeight: root.height * 0.32
            Layout.minimumHeight: 140
            dbRange: 18
            leftColor: root.leftColor
            rightColor: root.rightColor
        }
    }

    FileDialog {
        id: openLeftDialog
        title: qsTr("Open audio file A")
        currentFolder: StandardPaths.standardLocations(StandardPaths.MusicLocation)[0]
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Audio files") + " (*.wav *.flac)", qsTr("WAV files") + " (*.wav)", qsTr("FLAC files") + " (*.flac)", qsTr("All files") + " (*)"]
        onAccepted: audioAnalysisController.analyzeLeft(selectedFile)
    }

    FileDialog {
        id: openRightDialog
        title: qsTr("Open audio file B")
        currentFolder: StandardPaths.standardLocations(StandardPaths.MusicLocation)[0]
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Audio files") + " (*.wav *.flac)", qsTr("WAV files") + " (*.wav)", qsTr("FLAC files") + " (*.flac)", qsTr("All files") + " (*)"]
        onAccepted: audioAnalysisController.analyzeRight(selectedFile)
    }

    FileDialog {
        id: saveReportDialog
        title: qsTr("Save analysis report")
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.SaveFile
        defaultSuffix: "txt"
        nameFilters: [qsTr("Text files") + " (*.txt)", qsTr("All files") + " (*)"]
        onAccepted: {
            if (!audioAnalysisController.saveReport(selectedFile)) {
                applicationService.requestAlertDialog(qsTr("Failed to save the report."));
            }
        }
    }

    Connections {
        target: audioAnalysisController
        function onAnalysisChanged() {
            root.refresh();
        }
        function onErrorOccurred(message) {
            applicationService.requestAlertDialog(message);
        }
    }

    onOpened: refresh()

    Component.onCompleted: visible = false
}
