import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import ".."
import "../Components"

Rectangle {
    id: rootItem
    color: "#222222"
    border.color: "#444444"
    border.width: 1

    property var waveformData: []

    onWidthChanged: requestWaveform()

    Connections {
        target: audioService
        function onLatestRecordingFileNameChanged() {
            requestWaveform();
        }
    }

    Connections {
        target: themeService
        function onAccentColorChanged() {
            waveform.requestPaint();
        }
    }

    function requestWaveform() {
        if (width > 0) {
            const availableWidth = waveform.width - 12;
            if (availableWidth > 0) {
                const data = audioService.getWaveformData(availableWidth);
                waveformData = data || [];
                waveform.requestPaint();
            } else {
                waveformData = [];
                waveform.requestPaint();
            }
        }
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: qsTr("Open location...")
            enabled: audioService.latestRecordingFileName && !audioService.isRecording
            onTriggered: {
                if (audioService.latestRecordingFileName) {
                    const fileUrl = "file://" + audioService.latestRecordingFileName.substring(0, audioService.latestRecordingFileName.lastIndexOf('/'));
                    Qt.openUrlExternally(fileUrl);
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            opacity: !UiService.isPlaying() ? 1.0 : 0.5

            WaveformView {
                id: waveform
                anchors.fill: parent
                waveformData: rootItem.waveformData
                playbackPosition: audioService.playbackPosition
                showPlayhead: audioService.latestRecordingFileName && !audioService.isRecording
                fileName: audioService.latestRecordingFileName ? audioService.latestRecordingFileName.split("/").pop() : ""
                visible: !audioService.isRecording

                onSeekRequested: pos => {
                    if (!audioService.isRecording && !UiService.isPlaying()) {
                        const totalTicks = editorService.totalTicks();
                        if (audioService.latestRecordingFileName && totalTicks > 0) {
                            audioService.playbackPosition = pos;
                            const startTick = audioService.latestRecordingStartTick;
                            const endTick = audioService.latestRecordingEndTick;
                            const recordedTicks = (endTick > startTick) ? (endTick - startTick) : 0;

                            if (recordedTicks > 0) {
                                const targetTick = startTick + Math.floor(pos * recordedTicks);
                                editorService.requestPositionByTick(targetTick);
                            }
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: mouse => {
                        if (mouse.button === Qt.RightButton) {
                            contextMenu.popup();
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                text: qsTr("Recording...")
                color: "red"
                font.pixelSize: rootItem.height * 0.5
                font.bold: true
                visible: audioService.isRecording
            }
        }

        Button {
            id: playbackButton
            Layout.fillHeight: true
            Layout.preferredWidth: height
            enabled: audioService.latestRecordingFileName && !audioService.isRecording && !UiService.isPlaying()
            opacity: enabled ? 1.0 : 0.5
            focusPolicy: Qt.NoFocus
            onClicked: {
                if (audioService.isPlayingPlayback) {
                    audioService.stopPlayback();
                } else {
                    audioService.startPlayback(audioService.latestRecordingFileName, settingsService.audioBufferSize());
                }
            }

            ToolTip.visible: hovered
            ToolTip.text: audioService.isPlayingPlayback ? qsTr("Stop playback") : qsTr("Start playback")

            background: Rectangle {
                radius: height / 2
                color: audioService.isPlayingPlayback ? "#004400" : "#333333"
                border.color: audioService.isPlayingPlayback ? "#00FF00" : "#555555"
                border.width: 1
            }

            contentItem: Item {
                Image {
                    source: audioService.isPlayingPlayback ? "../Graphics/stop.png" : "../Graphics/play.png"
                    width: parent.height * 0.8
                    height: width
                    sourceSize.width: width
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    x: Math.floor((parent.width - width) / 2)
                    y: Math.floor((parent.height - height) / 2)
                }
            }
        }

        Button {
            id: enableRecordingButton
            Layout.fillHeight: true
            Layout.preferredWidth: height

            checkable: true
            checked: settingsService.recordingEnabled
            enabled: !UiService.isPlaying()
            opacity: enabled ? 1.0 : 0.5
            focusPolicy: Qt.NoFocus
            onClicked: {
                if (settingsService.recordingEnabled !== checked) {
                    settingsService.recordingEnabled = checked;
                }
            }

            ToolTip.visible: hovered
            ToolTip.text: qsTr("Enable audio recording")

            background: Rectangle {
                radius: height / 2
                color: enableRecordingButton.checked ? "#440000" : "#333333"
                border.color: enableRecordingButton.checked ? "#FF0000" : "#555555"
                border.width: 1
            }

            contentItem: Item {
                Image {
                    source: "../Graphics/record.png"
                    width: parent.height * 0.8
                    height: width
                    sourceSize.width: width
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    x: Math.floor((parent.width - width) / 2)
                    y: Math.floor((parent.height - height) / 2)
                }
            }
        }
    }
}
