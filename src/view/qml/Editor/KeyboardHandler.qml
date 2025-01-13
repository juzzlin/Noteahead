import QtQuick 2.15
import ".."

QtObject {
    function handleEvent(event) {
        if (event.key === Qt.Key_Up) {
            editorService.requestScroll(-1);
            event.accepted = true;
        } else if (event.key === Qt.Key_Down) {
            editorService.requestScroll(1);
            event.accepted = true;
        } else if (event.key === Qt.Key_Left) {
            editorService.requestCursorLeft();
            event.accepted = true;
        } else if (event.key === Qt.Key_Right) {
            editorService.requestCursorRight();
            event.accepted = true;
        } else if (event.key === Qt.Key_Tab) {
            editorService.requestTrackRight();
            event.accepted = true;
        } else if (event.key === Qt.Key_Delete) {
            if (UiService.editMode()) {
                if (editorService.isAtNoteColumn()) {
                    editorService.requestNoteDeletionAtCurrentPosition();
                    editorService.requestScroll(UiService.activeStep());
                } else if (editorService.isAtVelocityColumn) {
                    if (editorService.requestDigitSetAtCurrentPosition(0)) {
                        editorService.requestScroll(UiService.activeStep());
                    }
                }
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape) {
            UiService.setEditMode(!UiService.editMode());
            event.accepted = true;
        } else if (event.key === Qt.Key_Space) {
            UiService.togglePlay();
            event.accepted = true;
        } else if (event.key === Qt.Key_A) {
            if (editorService.requestNoteOffAtCurrentPosition()) {
                editorService.requestScroll(UiService.activeStep());
            }
            event.accepted = true;
        } else {
            if (UiService.editMode()) {
                if (editorService.isAtNoteColumn()) {
                    const key = _keyToNote(event.key);
                    if (key) {
                        const keysPerOctave = 12;
                        const effectiveOctave = UiService.activeOctave() + Math.floor(key / keysPerOctave);
                        const effectiveKey = key % keysPerOctave;
                        if (editorService.requestNoteOnAtCurrentPosition(effectiveKey, effectiveOctave, UiService.activeVelocity())) {
                            editorService.requestScroll(UiService.activeStep());
                        }
                    }
                } else if (editorService.isAtVelocityColumn()) {
                    const digit = _keyToDigit(event.key);
                    if (digit !== undefined) {
                        if (editorService.requestDigitSetAtCurrentPosition(digit)) {
                            editorService.requestScroll(UiService.activeStep());
                        }
                    }
                }
            }
        }
    }
    function _keyToDigit(eventKey) {
        const digitMap = {
            [Qt.Key_0]: 0,
            [Qt.Key_1]: 1,
            [Qt.Key_2]: 2,
            [Qt.Key_3]: 3,
            [Qt.Key_4]: 4,
            [Qt.Key_5]: 5,
            [Qt.Key_6]: 6,
            [Qt.Key_7]: 7,
            [Qt.Key_8]: 8,
            [Qt.Key_9]: 9
        };
        return digitMap[eventKey];
    }
    function _keyToNote(eventKey) {
        const noteMap = {
            // Lower octave starting with Z (C)
            [Qt.Key_Z]: 1,
            [Qt.Key_S]: 2,
            [Qt.Key_X]: 3,
            [Qt.Key_D]: 4,
            [Qt.Key_C]: 5,
            [Qt.Key_V]: 6,
            [Qt.Key_G]: 7,
            [Qt.Key_B]: 8,
            [Qt.Key_H]: 9,
            [Qt.Key_N]: 10,
            [Qt.Key_J]: 11,
            [Qt.Key_M]: 12,

            // Higher octave starting with Q (C)
            [Qt.Key_Q]: 13,
            [Qt.Key_2]: 14,
            [Qt.Key_W]: 15,
            [Qt.Key_3]: 16,
            [Qt.Key_E]: 17,
            [Qt.Key_R]: 18,
            [Qt.Key_5]: 19,
            [Qt.Key_T]: 20,
            [Qt.Key_6]: 21,
            [Qt.Key_Y]: 22,
            [Qt.Key_7]: 23,
            [Qt.Key_U]: 24
        };
        return noteMap[eventKey];
    }
}
