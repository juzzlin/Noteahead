import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import ".."

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
            canvas.requestPaint();
        }
    }

    function requestWaveform() {
        if (width > 0) {
            const availableWidth = canvasContainer.width;
            if (availableWidth > 0) {
                const data = audioService.getWaveformData(availableWidth);
                waveformData = data || [];
                canvas.requestPaint();
            } else {
                waveformData = [];
                canvas.requestPaint();
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
            id: canvasContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            opacity: !UiService.isPlaying() ? 1.0 : 0.5

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                enabled: !UiService.isPlaying()
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                cursorShape: audioService.latestRecordingFileName && !audioService.isRecording && !UiService.isPlaying() ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: mouse => {
                    if (mouse.button === Qt.RightButton) {
                        contextMenu.popup();
                    } else if (mouse.button === Qt.LeftButton) {
                        const totalTicks = editorService.totalTicks();
                        if (audioService.latestRecordingFileName && !audioService.isRecording && totalTicks > 0) {
                            const pos = mouse.x / width;
                            audioService.playbackPosition = pos;

                            // Seek main sequencer
                            // Calculate target tick based on the actual recorded range
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
            }

            Canvas {
                id: canvas
                anchors.fill: parent
                visible: !audioService.isRecording
                onPaint: {
                    const ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);

                    if (!rootItem.waveformData || rootItem.waveformData.length === 0)
                        return;

                    let maxPeak = 0;
                    for (let i = 0; i < rootItem.waveformData.length; i++) {
                        maxPeak = Math.max(maxPeak, rootItem.waveformData[i]);
                    }

                    ctx.strokeStyle = themeService.accentColor;
                    ctx.lineWidth = 1;
                    ctx.beginPath();

                    const midY = height / 2;
                    const len = rootItem.waveformData.length;
                    const stepX = width / len;

                    for (let i = 0; i < len; i++) {
                        let val = rootItem.waveformData[i];
                        if (maxPeak > 0) {
                            val /= maxPeak;
                        }
                        const x = i * stepX;
                        const h = val * height * 0.9;

                        ctx.moveTo(x, midY - h / 2);
                        ctx.lineTo(x, midY + h / 2);
                    }
                    ctx.stroke();
                }
            }

            Rectangle {
                id: playhead
                width: 2
                height: parent.height
                color: "white"
                opacity: 0.8
                x: audioService.playbackPosition * (parent.width - width)
                visible: audioService.latestRecordingFileName && !audioService.isRecording
            }

            Text {
                anchors.centerIn: parent
                text: qsTr("Recording...")
                color: "red"
                font.pixelSize: rootItem.height * 0.5
                font.bold: true
                visible: audioService.isRecording
            }

            Text {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: 4
                text: audioService.latestRecordingFileName ? audioService.latestRecordingFileName.split("/").pop() : ""
                color: "white"
                font.pixelSize: 10
                visible: text !== "" && !audioService.isRecording
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
