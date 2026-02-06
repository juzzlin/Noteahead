import QtQuick 2.15
import QtQuick.Controls 2.15
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

    function requestWaveform() {
        if (width > 0) {
            const availableWidth = canvasContainer.width;
            if (availableWidth > 0) {
                waveformData = audioService.getWaveformData(availableWidth);
                canvas.requestPaint();
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

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: audioService.latestRecordingFileName && !audioService.isRecording ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: {
                    if (audioService.latestRecordingFileName && !audioService.isRecording) {
                        const fileUrl = "file://" + audioService.latestRecordingFileName.substring(0, audioService.latestRecordingFileName.lastIndexOf('/'));
                        Qt.openUrlExternally(fileUrl);
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

                    if (rootItem.waveformData.length === 0)
                        return;

                    let maxPeak = 0;
                    for (let i = 0; i < rootItem.waveformData.length; i++) {
                        maxPeak = Math.max(maxPeak, rootItem.waveformData[i]);
                    }

                    ctx.strokeStyle = "orange";
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
            id: enableRecordingButton
            Layout.fillHeight: true
            Layout.preferredWidth: height

            checkable: true
            checked: settingsService.recordingEnabled
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
                    anchors.centerIn: parent
                    source: "../Graphics/record.svg"
                    width: Math.min(parent.width, parent.height) * 0.6
                    height: width
                    fillMode: Image.PreserveAspectFit
                }
            }
        }
    }
}
