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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Item {
    id: rootItem
    width: 1600
    height: width * 0.1

    signal noteOnRequested(int note)
    signal noteOffRequested(int note)

    property int velocityKeyTrack: 0
    property int velocityKeyTrackOffset: 0
    property var activeNotes: []

    function getVelocityColor(note: int, baseColor: color): color {
        if (velocityKeyTrack <= 0) {
            return baseColor;
        }
        const effectiveNote = Math.max(0, note - velocityKeyTrackOffset);
        let factor = 1.0 - (velocityKeyTrack / 100.0) * (effectiveNote / 127.0);
        factor = Math.max(0.0, Math.min(1.0, factor));
        const c1 = themeService.accentColor;
        const c2 = Qt.color(baseColor);
        return Qt.rgba(c1.r + (c2.r - c1.r) * factor, c1.g + (c2.g - c1.g) * factor, c1.b + (c2.b - c1.b) * factor, 1.0);
    }

    // Real piano range: A0 (21) => C8 (108)
    readonly property int baseNote: 21
    readonly property int topNote: 108

    // 52 white keys from A0 to C8
    readonly property int whiteKeyCount: 52
    property real whiteKeyWidth: width / whiteKeyCount

    // Keys are rounded all round and the felt strip covers the top of them, so only the front edge
    // reads as rounded. Per-corner radii would say it more directly but they need Qt 6.7, and this
    // costs nothing: no clipping and no layers across 88 keys.
    readonly property real keyRadius: Math.max(1, whiteKeyWidth * 0.08)
    readonly property real feltHeight: Math.max(3, height * 0.05)

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
        // The bed the keys sit in. Without it the rounded fronts leave notches along the bottom
        // that show whatever is behind the keyboard, and the keys read as floating rather than as
        // sitting in an instrument.
        color: "#2a2a2a"

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
                    // Pressed keys sink rather than grow: the front edge stays where it is.
                    y: pressed ? 1 : 0
                    height: parent.height - (pressed ? 1 : 0)
                    radius: rootItem.keyRadius
                    property bool pressed: false
                    property int note: rootItem.baseNote + rootItem.whiteOffset(index)
                    // The key-track tint stays the key's own colour; the gradient is derived from it
                    // so a tinted key still shades from back to front.
                    property color baseColor: rootItem.getVelocityColor(note, "#ffffff")

                    gradient: Gradient {
                        GradientStop {
                            position: 0.0
                            color: Qt.darker(wkey.baseColor, wkey.pressed ? 1.22 : 1.12)
                        }
                        GradientStop {
                            position: 0.55
                            color: Qt.darker(wkey.baseColor, wkey.pressed ? 1.10 : 1.02)
                        }
                        GradientStop {
                            position: 0.93
                            color: wkey.pressed ? Qt.darker(wkey.baseColor, 1.06) : wkey.baseColor
                        }
                        GradientStop {
                            position: 1.0
                            color: Qt.darker(wkey.baseColor, wkey.pressed ? 1.18 : 1.06)
                        }
                    }

                    // The seam to the key on the left, in place of a border around every key: a
                    // shadowed edge with the neighbouring key's lit edge beside it.
                    Rectangle {
                        width: 1
                        height: parent.height - rootItem.keyRadius * 0.6
                        color: "#5f5f5f"
                        visible: index > 0
                    }
                    Rectangle {
                        x: 1
                        width: 1
                        height: parent.height - rootItem.keyRadius * 0.6
                        color: "#ffffff"
                        opacity: 0.6
                        visible: index > 0
                    }

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        radius: parent.radius
                        color: themeService.accentColor
                        opacity: 0.3
                        visible: rootItem.activeNotes.includes(parent.note)
                    }

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

                delegate: Item {
                    id: bkeySlot
                    visible: blackLayer.blackMap[index % 7] === 1
                    width: rootItem.whiteKeyWidth * 0.6
                    height: parent.height * 0.6
                    x: (index + 1) * rootItem.whiteKeyWidth - width / 2
                    y: 0
                    property bool pressed: false
                    property int note: rootItem.baseNote + rootItem.whiteOffset(index) + 1

                    // Cast onto the white keys below. A rectangle rather than an effect: 36 of these
                    // are on screen at once and a blur on each is not worth the frame.
                    Rectangle {
                        x: Math.max(1, bkeySlot.width * 0.06)
                        y: bkeySlot.pressed ? 1 : 2
                        width: parent.width
                        height: parent.height
                        radius: rootItem.keyRadius
                        color: "#000000"
                        opacity: bkeySlot.pressed ? 0.18 : 0.3
                    }

                    Rectangle {
                        id: bkey
                        // Explicit geometry rather than anchors.fill, which would override the
                        // pressed offset.
                        y: bkeySlot.pressed ? 1 : 0
                        width: parent.width
                        height: parent.height - (bkeySlot.pressed ? 1 : 0)
                        radius: rootItem.keyRadius
                        property color baseColor: rootItem.getVelocityColor(bkeySlot.note, "#0b0b0b")

                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: Qt.lighter(bkey.baseColor, bkeySlot.pressed ? 2.6 : 3.6)
                            }
                            GradientStop {
                                position: 0.45
                                color: Qt.lighter(bkey.baseColor, bkeySlot.pressed ? 1.6 : 2.0)
                            }
                            GradientStop {
                                position: 0.82
                                color: bkey.baseColor
                            }
                            GradientStop {
                                position: 1.0
                                color: Qt.lighter(bkey.baseColor, bkeySlot.pressed ? 1.8 : 2.8)
                            }
                        }

                        // Specular line along the back edge.
                        Rectangle {
                            width: parent.width - rootItem.keyRadius
                            height: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: 1
                            color: "#ffffff"
                            opacity: bkeySlot.pressed ? 0.06 : 0.14
                        }

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 1
                            radius: parent.radius
                            color: themeService.accentColor
                            opacity: 0.4
                            visible: rootItem.activeNotes.includes(bkeySlot.note)
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onPressed: {
                            bkeySlot.pressed = true;
                            rootItem.noteOnRequested(bkeySlot.note);
                        }
                        onReleased: {
                            bkeySlot.pressed = false;
                            rootItem.noteOffRequested(bkeySlot.note);
                        }
                    }
                }
            }
        }

        // Felt strip along the back, which is also what hides the top of the rounded keys.
        Rectangle {
            id: felt
            width: parent.width
            height: rootItem.feltHeight
            z: 3
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.darker(themeService.accentColor, 2.4)
                }
                GradientStop {
                    position: 1.0
                    color: Qt.darker(themeService.accentColor, 3.6)
                }
            }
        }

        // The keys are in shadow where they meet the felt.
        Rectangle {
            width: parent.width
            height: rootItem.feltHeight * 1.6
            y: felt.height
            z: 3
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.rgba(0, 0, 0, 0.35)
                }
                GradientStop {
                    position: 1.0
                    color: "transparent"
                }
            }
        }
    }
}
