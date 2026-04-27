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
    onWaveformDataChanged: canvas.requestPaint()
    property double playbackPosition: 0.0
    property bool showPlayhead: false
    property string fileName: ""
    property alias accentColor: canvas.accentColor

    signal seekRequested(double position)

    function requestPaint() {
        canvas.requestPaint();
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        anchors.margins: 6
        property color accentColor: themeService.accentColor

        onPaint: {
            const ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            if (!rootItem.waveformData || rootItem.waveformData.length === 0)
                return;

            let maxPeak = 0;
            for (let i = 0; i < rootItem.waveformData.length; i++) {
                maxPeak = Math.max(maxPeak, rootItem.waveformData[i]);
            }

            ctx.strokeStyle = accentColor;
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
        height: parent.height - 12
        anchors.verticalCenter: parent.verticalCenter
        x: 6 + rootItem.playbackPosition * (parent.width - 12 - width)
        color: "white"
        opacity: 0.8
        visible: rootItem.showPlayhead
    }

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 10
        text: rootItem.fileName
        color: "white"
        font.pixelSize: 10
        visible: text !== ""
    }

    MouseArea {
        anchors.fill: parent
        anchors.margins: 6
        onClicked: mouse => {
            rootItem.seekRequested(mouse.x / width);
        }
    }
}
