import QtQuick 2.15
import ".."

QtObject {
    function handleKeyPressed(event) {
        if (event.isAutoRepeat) {
            return;
        }
        if (event.key === Qt.Key_Up) {
            if (!UiService.isPlaying()) {
                editorService.requestScroll(-1);
                event.accepted = true;
            }
        } else if (event.key === Qt.Key_Down) {
            if (!UiService.isPlaying()) {
                editorService.requestScroll(1);
                event.accepted = true;
            }
        } else if (event.key === Qt.Key_Left) {
            editorService.requestCursorLeft();
            event.accepted = true;
        } else if (event.key === Qt.Key_Right) {
            editorService.requestCursorRight();
            event.accepted = true;
        } else if (event.key === Qt.Key_Tab) {
            editorService.requestColumnRight();
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
            UiService.toggleEditMode();
            event.accepted = true;
        } else if (event.key === Qt.Key_Space) {
            UiService.togglePlay();
            event.accepted = true;
        } else if (event.key === Qt.Key_A) {
            if (UiService.editMode()) {
                if (editorService.requestNoteOffAtCurrentPosition()) {
                    editorService.requestScroll(UiService.activeStep());
                }
            }
            event.accepted = true;
        } else {
            _handleNoteTriggered(event);
        }
    }
    function _handleNoteTriggered(event) {
        if (UiService.editMode()) {
            if (editorService.isAtNoteColumn()) {
                _handleNoteInserted(event);
            } else if (editorService.isAtVelocityColumn()) {
                _handleVelocityInserted(event);
            }
        }
        _handleLiveNoteTriggered(event);
    }
    function _handleNoteInserted(event) {
        const effectiveNote = _effectiveNote(event.key);
        if (effectiveNote) {
            const effectiveOctave = _effectiveOctave(effectiveNote);
            if (editorService.requestNoteOnAtCurrentPosition(effectiveNote, effectiveOctave, UiService.activeVelocity())) {
                editorService.requestScroll(UiService.activeStep());
            }
            applicationService.requestLiveNoteOn(effectiveNote, effectiveOctave, UiService.activeVelocity());
        }
    }
    function _handleVelocityInserted(event) {
        const digit = _keyToDigit(event.key);
        if (digit !== undefined) {
            if (editorService.requestDigitSetAtCurrentPosition(digit)) {
                editorService.requestScroll(UiService.activeStep());
            }
        }
    }
    function _handleLiveNoteTriggered(event) {
        const effectiveNote = _effectiveNote(event.key);
        if (effectiveNote) {
            const effectiveOctave = _effectiveOctave(event.key);
            applicationService.requestLiveNoteOn(effectiveNote, effectiveOctave, UiService.activeVelocity());
        }
    }
    function handleKeyReleased(event) {
        if (event.isAutoRepeat) {
            return;
        }
        _handleLiveNoteReleased(event);
    }
    function _handleLiveNoteReleased(event) {
        const effectiveNote = _effectiveNote(event.key);
        if (effectiveNote) {
            const effectiveOctave = _effectiveOctave(event.key);
            applicationService.requestLiveNoteOff(effectiveNote, effectiveOctave);
        }
    }
    function _effectiveNote(eventKey) {
        const key = _keyToNote(eventKey);
        return key ? key.note : null;
    }
    function _effectiveOctave(eventKey) {
        return UiService.activeOctave() + _keyToNote(eventKey).octave;
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
            [Qt.Key_Z]: {
                "note": 1,
                "octave": 0
            },
            [Qt.Key_S]: {
                "note": 2,
                "octave": 0
            },
            [Qt.Key_X]: {
                "note": 3,
                "octave": 0
            },
            [Qt.Key_D]: {
                "note": 4,
                "octave": 0
            },
            [Qt.Key_C]: {
                "note": 5,
                "octave": 0
            },
            [Qt.Key_V]: {
                "note": 6,
                "octave": 0
            },
            [Qt.Key_G]: {
                "note": 7,
                "octave": 0
            },
            [Qt.Key_B]: {
                "note": 8,
                "octave": 0
            },
            [Qt.Key_H]: {
                "note": 9,
                "octave": 0
            },
            [Qt.Key_N]: {
                "note": 10,
                "octave": 0
            },
            [Qt.Key_J]: {
                "note": 11,
                "octave": 0
            },
            [Qt.Key_M]: {
                "note": 12,
                "octave": 0
            },
            [Qt.Key_Comma]: {
                "note": 1,
                "octave": 1
            },
            [Qt.Key_L]: {
                "note": 2,
                "octave": 1
            },
            [Qt.Key_Period]: {
                "note": 3,
                "octave": 1
            },
            [Qt.Key_Odiaeresis]: {
                "note": 4,
                "octave": 1
            },
            [Qt.Key_Minus]: {
                "note": 5,
                "octave": 1
            },
            // Higher octave starting with Q (C)
            [Qt.Key_Q]: {
                "note": 1,
                "octave": 1
            },
            [Qt.Key_2]: {
                "note": 2,
                "octave": 1
            },
            [Qt.Key_W]: {
                "note": 3,
                "octave": 1
            },
            [Qt.Key_3]: {
                "note": 4,
                "octave": 1
            },
            [Qt.Key_E]: {
                "note": 5,
                "octave": 1
            },
            [Qt.Key_R]: {
                "note": 6,
                "octave": 1
            },
            [Qt.Key_5]: {
                "note": 7,
                "octave": 1
            },
            [Qt.Key_T]: {
                "note": 8,
                "octave": 1
            },
            [Qt.Key_6]: {
                "note": 9,
                "octave": 1
            },
            [Qt.Key_Y]: {
                "note": 10,
                "octave": 1
            },
            [Qt.Key_7]: {
                "note": 11,
                "octave": 1
            },
            [Qt.Key_U]: {
                "note": 12,
                "octave": 1
            },
            [Qt.Key_I]: {
                "note": 1,
                "octave": 2
            },
            [Qt.Key_9]: {
                "note": 2,
                "octave": 2
            },
            [Qt.Key_O]: {
                "note": 3,
                "octave": 2
            },
            [Qt.Key_0]: {
                "note": 4,
                "octave": 2
            },
            [Qt.Key_P]: {
                "note": 5,
                "octave": 2
            }
        };
        return noteMap[eventKey];
    }
}
