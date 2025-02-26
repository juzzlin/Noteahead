import QtQuick 2.15

Item {
    id: rootItem
    width: 800
    height: 100
    readonly property int _numKeys: 88
    property int _keyWidth: width / _numKeys
    function setPosition(position) {
        for (const trackIndex of editorService.trackIndices()) {
            const midiNotesAtPosition = editorService.midiNotesAtPosition(position.pattern, trackIndex, position.line);
            for (const note of midiNotesAtPosition) {
                triggerNoteAnimation(note);
            }
        }
    }
    function triggerNoteAnimation(midiNote) {
        const noteCircle = noteLayer.children[midiNote - 21];
        if (noteCircle) {
            noteCircle.startAnimation();
        }
    }
    function noteColor(midiNote) {
        const normalized = (midiNote - 21) / (_numKeys - 1); // Scale from 0 to 1
        let red = 1.0;
        let green = 2.0 * normalized;
        if (green > 1.0) {
            red = 2.0 - 2.0 * normalized;
            green = 1.0;
        }
        return Qt.rgba(red, green, 0, 1);
    }
    Item {
        id: noteLayer
        anchors.fill: parent
        Repeater {
            model: _numKeys
            Rectangle {
                id: noteCircle
                property int midiNote: index + 21
                property int initialX: 0
                width: 10
                height: 10
                radius: width / 2
                color: noteColor(midiNote)
                opacity: 0
                anchors.verticalCenter: noteLayer.verticalCenter
                x: initialX - width / 2
                SequentialAnimation {
                    id: shrinkAnimation
                    ParallelAnimation {
                        NumberAnimation {
                            target: noteCircle
                            property: "width"
                            to: 0
                            duration: 500
                        }
                        NumberAnimation {
                            target: noteCircle
                            property: "height"
                            to: 0
                            duration: 500
                        }
                    }
                }
                function startAnimation() {
                    width = noteLayer.height * 0.85;
                    height = width;
                    opacity = 1;
                    initialX = index * _keyWidth + (_keyWidth / 2);
                    shrinkAnimation.restart();
                }
            }
        }
    }
}
