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
import Noteahead 1.0
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import ".."

Rectangle {
    id: rootItem
    color: "#222222"
    border.color: Theme.borderMedium
    border.width: 1

    property var waveformData: []
    property double playbackPosition: 0.0
    property double startOffset: 0.0
    //! Where playback stops, as a fraction of the file. One means it runs to the end.
    property double endOffset: 1.0
    property bool showPlayhead: false
    property string fileName: ""
    //! Said in the middle of the view while there is no waveform to draw. Empty for a view that would
    //! rather show nothing than explain itself.
    property string placeholderText: ""

    //! How long the sound actually lasts, in seconds. Not the same as duration, which is what the
    //! picture spans: an envelope that closes early shortens what is heard without shortening the
    //! material it was drawn from. Zero hides the readout.
    property double audibleLength: 0.0
    property alias accentColor: renderer.accentColor

    //! Where a looping range comes back round to, as a fraction of the file. Negative hides the marker.
    property double loopPosition: -1.0

    //! Amplitude envelope drawn over the waveform, in the seconds the envelope actually runs for. The
    //! view needs the file's own length to put those seconds on the same axis as the waveform.
    property bool showEnvelope: false
    property double duration: 0.0
    property double envelopeAttack: 0.0
    //! Time held at the top between the attack and the decay. Zero, the default, draws nothing at
    //! all, which is the ADSR this view drew before the stage existed.
    property double envelopeHold: 0.0
    property double envelopeDecay: 0.0
    property double envelopeSustain: 1.0
    property double envelopeRelease: 0.0
    //! Bend of every segment, 0..1, matching AdsrEnvelope::setCurve(). Zero draws the straight lines
    //! this view has always drawn.
    property double envelopeCurve: 0.0
    //! How far a full curve bends. Defaults to the drum pads' envelope; the Sampler's bends further.
    property alias envelopeMaxCurvature: renderer.envelopeMaxCurvature

    //! Draws the start, the end and the loop point as handles the mouse can drag. The view reports
    //! where a handle was dragged as a fraction of the file and leaves the writing to its owner.
    property bool draggableMarkers: false
    signal startOffsetMoved(double newPosition)
    signal endOffsetMoved(double newPosition)
    signal loopPositionMoved(double newPosition)

    signal seekRequested(double position)

    Item {
        id: canvasArea
        anchors.fill: parent
        anchors.margins: 6

        // The peaks, the shaded trims and the envelope, drawn in C++ in that order
        WaveformRenderer {
            id: renderer
            anchors.fill: parent
            accentColor: themeService.accentColor
            waveformData: rootItem.waveformData || []
            startOffset: rootItem.startOffset
            endOffset: rootItem.endOffset
            showEnvelope: rootItem.showEnvelope
            duration: rootItem.duration
            envelopeAttack: rootItem.envelopeAttack
            envelopeHold: rootItem.envelopeHold
            envelopeDecay: rootItem.envelopeDecay
            envelopeSustain: rootItem.envelopeSustain
            envelopeRelease: rootItem.envelopeRelease
            envelopeCurve: rootItem.envelopeCurve
        }
    }

    //! The markers sit on the same strip the renderer draws on, so a marker's own fraction of the strip
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

    Text {
        anchors.centerIn: parent
        text: rootItem.placeholderText
        // The thing you see in an empty view, so it carries the weight the clock that replaces it does.
        // Grey rather than accented: it is a hint about what to do, not a value being reported.
        color: Theme.textMuted
        font.pixelSize: Constants.waveViewOverlayFontSize
        font.bold: true
        visible: text !== "" && (!rootItem.waveformData || rootItem.waveformData.length === 0)
    }

    // How long it lasts, and the tempo at which one hit fills a beat. Past that tempo the sound
    // runs into the next beat, which is the question being asked of a drum or a one-shot.
    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 10
        // Higher than the file name opposite it: the waveform is drawn to within a few pixels of
        // the frame, and at the file name's own margin this sat on the border.
        anchors.bottomMargin: 18
        text: {
            const ms = rootItem.audibleLength * 1000;
            const bpm = Math.round(60000 / ms);
            return Math.round(ms) + " ms \u00b7 " + qsTr("max %1 BPM").arg(bpm);
        }
        color: Theme.textSecondary
        font.pixelSize: 10
        visible: rootItem.audibleLength > 0
    }

    MouseArea {
        anchors.fill: parent
        anchors.margins: 6
        onClicked: mouse => {
            rootItem.seekRequested(mouse.x / width);
        }
    }
}
