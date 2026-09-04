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
    property double startOffset: 0.0
    onStartOffsetChanged: canvas.requestPaint()
    //! Where playback stops, as a fraction of the file. One means it runs to the end.
    property double endOffset: 1.0
    onEndOffsetChanged: canvas.requestPaint()
    property bool showPlayhead: false
    property string fileName: ""
    property alias accentColor: canvas.accentColor

    //! Where a looping range comes back round to, as a fraction of the file. Negative hides the marker.
    property double loopPosition: -1.0
    onLoopPositionChanged: canvas.requestPaint()

    //! Amplitude envelope drawn over the waveform, in the seconds the envelope actually runs for. The
    //! view needs the file's own length to put those seconds on the same axis as the waveform.
    property bool showEnvelope: false
    property double duration: 0.0
    property double envelopeAttack: 0.0
    property double envelopeDecay: 0.0
    property double envelopeSustain: 1.0
    property double envelopeRelease: 0.0
    onShowEnvelopeChanged: canvas.requestPaint()
    onDurationChanged: canvas.requestPaint()
    onEnvelopeAttackChanged: canvas.requestPaint()
    onEnvelopeDecayChanged: canvas.requestPaint()
    onEnvelopeSustainChanged: canvas.requestPaint()
    onEnvelopeReleaseChanged: canvas.requestPaint()

    //! Draws the start, the end and the loop point as handles the mouse can drag. The view reports
    //! where a handle was dragged as a fraction of the file and leaves the writing to its owner.
    property bool draggableMarkers: false
    signal startOffsetMoved(double newPosition)
    signal endOffsetMoved(double newPosition)
    signal loopPositionMoved(double newPosition)

    signal seekRequested(double position)

    function requestPaint() {
        canvas.requestPaint();
    }

    //! The amp envelope as [x, level] points across the pad's range: attack and decay run from where
    //! the range starts, the sustain holds to where it ends and the release falls from there, which is
    //! where a pad held to the end of its range would be released. A segment that would run past the
    //! end of the range is cut there, at the level it had reached, so a long attack on a short sample
    //! shows how little of the envelope the pad ever plays.
    function envelopePoints(x0, x1, pixelsPerSecond) {
        const points = [[x0, 0]];
        let x = x0;
        let level = 0;
        const segment = (seconds, target) => {
            const end = x + seconds * pixelsPerSecond;
            if (end >= x1) {
                level += (target - level) * (end > x ? (x1 - x) / (end - x) : 1);
                x = x1;
                points.push([x, level]);
                return false;
            }
            x = end;
            level = target;
            points.push([x, level]);
            return true;
        };
        if (segment(rootItem.envelopeAttack, 1) && segment(rootItem.envelopeDecay, rootItem.envelopeSustain)) {
            level = rootItem.envelopeSustain;
            points.push([x1, level]);
        }
        points.push([x1 + rootItem.envelopeRelease * pixelsPerSecond, 0]);
        return points;
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

            // Render start offset overlay
            if (rootItem.startOffset > 0) {
                ctx.fillStyle = accentColor;
                ctx.globalAlpha = 0.3;
                ctx.fillRect(0, 0, Math.min(width, rootItem.startOffset * width), height);
                ctx.globalAlpha = 1.0;
            }
            if (rootItem.endOffset < 1.0) {
                const trimStart = Math.max(0, rootItem.endOffset * width);
                ctx.fillStyle = accentColor;
                ctx.globalAlpha = 0.3;
                ctx.fillRect(trimStart, 0, width - trimStart, height);
                ctx.globalAlpha = 1.0;
            }

            const rangeStart = Math.max(0, rootItem.startOffset * width);
            const rangeEnd = Math.min(width, rootItem.endOffset * width);

            // The envelope rides on the same amplitude scale as the waveform, mirrored around the
            // centre line so it reads as the shape the sample is played through.
            if (rootItem.showEnvelope && rootItem.duration > 0 && rangeEnd > rangeStart) {
                const points = rootItem.envelopePoints(rangeStart, rangeEnd, width / rootItem.duration);
                ctx.strokeStyle = "white";
                ctx.lineWidth = 1;
                // Held back a little: on a pad with no envelope dialled in the shape is a plain box,
                // and at full strength it reads as a second border around the waveform.
                ctx.globalAlpha = 0.6;
                for (const side of [-1, 1]) {
                    ctx.beginPath();
                    for (let i = 0; i < points.length; i++) {
                        const y = midY + side * points[i][1] * height * 0.45;
                        if (i === 0) {
                            ctx.moveTo(points[i][0], y);
                        } else {
                            ctx.lineTo(points[i][0], y);
                        }
                    }
                    ctx.stroke();
                }
                ctx.globalAlpha = 1.0;
            }
        }
    }

    //! The markers sit on the same strip the canvas paints, so a marker's own fraction of the strip
    //! is the fraction of the file it stands on.
    Item {
        id: markerTrack
        anchors.fill: parent
        anchors.margins: 6
        visible: rootItem.draggableMarkers
        // Above the seek area at the bottom of the file, which would otherwise swallow the drags.
        z: 1

        WaveformMarker {
            position: rootItem.startOffset
            maximumPosition: rootItem.endOffset
            onMoved: newPosition => rootItem.startOffsetMoved(newPosition)
        }

        WaveformMarker {
            position: rootItem.endOffset
            minimumPosition: rootItem.startOffset
            direction: -1
            onMoved: newPosition => rootItem.endOffsetMoved(newPosition)
        }

        // Dashed, so that the loop point is taken neither for one of the trims nor for the playhead
        // running past it.
        WaveformMarker {
            position: rootItem.loopPosition
            minimumPosition: rootItem.startOffset
            maximumPosition: rootItem.endOffset
            dashed: true
            visible: rootItem.loopPosition >= 0
            onMoved: newPosition => rootItem.loopPositionMoved(newPosition)
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
