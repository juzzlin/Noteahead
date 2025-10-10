import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Item {
    id: rootItem
    width: 1600
    height: width * 0.1

    signal noteOnRequested(int note)
    signal noteOffRequested(int note)

    // Real piano range: A0 (21) => C8 (108)
    readonly property int baseNote: 21
    readonly property int topNote: 108

    // 52 white keys from A0 to C8
    readonly property int whiteKeyCount: 52
    property real whiteKeyWidth: width / whiteKeyCount

    // White-key intervals pattern (A–B–C–D–E–F–G)
    // Between A-B=2, B-C=1, C-D=2, D-E=2, E-F=1, F-G=2, G-A=2
    readonly property var whiteIntervals: [2, 1, 2, 2, 1, 2, 2]

    // Compute semitone offset from the first white key (A0)
    function whiteOffset(i) {
        var s = 0;
        for (var k = 0; k < i; ++k)
            s += whiteIntervals[k % 7];
        return s;
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        // White keys
        Item {
            id: whiteLayer
            anchors.fill: parent
            z: 1

            Repeater {
                model: rootItem.whiteKeyCount
                delegate: Rectangle {
                    id: wkey
                    x: index * rootItem.whiteKeyWidth
                    width: rootItem.whiteKeyWidth
                    height: parent.height
                    color: pressed ? "#e6e6e6" : "#ffffff"
                    border.color: "#222"
                    border.width: 1
                    property bool pressed: false
                    property int note: rootItem.baseNote + rootItem.whiteOffset(index)

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onPressed: {
                            wkey.pressed = true;
                            rootItem.noteOnRequested(wkey.note);
                        }
                        onReleased: {
                            wkey.pressed = false;
                            rootItem.noteOffRequested(wkey.note);
                        }
                    }
                }
            }
        }

        // Black keys overlay
        Item {
            id: blackLayer
            anchors.fill: parent
            z: 2

            // A–B has black (A#), B–C none, C–D (C#), D–E (D#), E–F none, F–G (F#), G–A (G#)
            // Represent as 1 if black key between current white and next
            readonly property var blackMap: [1, 0, 1, 1, 0, 1, 1]

            Repeater {
                model: rootItem.whiteKeyCount - 1
                delegate: Rectangle {
                    visible: blackLayer.blackMap[index % 7] === 1
                    width: rootItem.whiteKeyWidth * 0.6
                    height: parent.height * 0.6
                    radius: 3
                    color: pressed ? "#222" : "#000"
                    border.color: "#111"
                    x: (index + 1) * rootItem.whiteKeyWidth - width / 2
                    y: 0
                    property bool pressed: false
                    property int note: rootItem.baseNote + rootItem.whiteOffset(index) + 1

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onPressed: {
                            parent.pressed = true;
                            rootItem.noteOnRequested(parent.note);
                        }
                        onReleased: {
                            parent.pressed = false;
                            rootItem.noteOffRequested(parent.note);
                        }
                    }
                }
            }
        }
    }
}
