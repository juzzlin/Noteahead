import QtQuick 2.15
import ".."

QtObject {
    function handleKeyPressed(event) {
        if (event.key === Qt.Key_Up) {
            _handleUp(event);
            event.accepted = true;
        } else if (event.key === Qt.Key_Down) {
            _handleDown(event);
            event.accepted = true;
        } else if (event.key === Qt.Key_Left) {
            _handleLeft();
            event.accepted = true;
        } else if (event.key === Qt.Key_Right) {
            _handleRight();
            event.accepted = true;
        } else if (event.key === Qt.Key_PageUp) {
            _handlePageUp(event);
            event.accepted = true;
        } else if (event.key === Qt.Key_PageDown) {
            _handlePageDown(event);
            event.accepted = true;
        } else if (event.key === Qt.Key_Tab) {
            _handleTab();
            event.accepted = true;
        } else if (event.key === Qt.Key_Delete) {
            _handleDelete();
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape) {
            _handleEscape();
            event.accepted = true;
        } else if (event.key === Qt.Key_Home) {
            _handleHome();
            event.accepted = true;
        } else if (event.key === Qt.Key_Insert) {
            _handleInsert();
            event.accepted = true;
        } else if (event.key === Qt.Key_Backspace) {
            _handleBackspace();
            event.accepted = true;
        } else if (event.key === Qt.Key_Space) {
            _handleSpace();
            event.accepted = true;
        } else if (event.key === Qt.Key_F3) {
            UiService.setActiveOctave(UiService.activeOctave() - 1);
        } else if (event.key === Qt.Key_F4) {
            UiService.setActiveOctave(UiService.activeOctave() + 1);
        } else if (event.key === Qt.Key_A) {
            _handleNoteOff();
            event.accepted = true;
        } else {
            _handleNoteTriggered(event);
        }
    }
    function _handleScrollCommon(event, scrollAmount) {
        if (!UiService.isPlaying()) {
            if (event.modifiers & Qt.ShiftModifier) {
                let position = editorService.position;
                selectionService.requestSelectionStart(position.pattern, position.track, position.column, position.line);
                editorService.requestScroll(scrollAmount);
                position = editorService.position;
                selectionService.requestSelectionEnd(position.pattern, position.track, position.column, position.line);
            } else {
                selectionService.clear();
                editorService.requestScroll(scrollAmount);
            }
        }
    }
    function _handleUp(event) {
        _handleScrollCommon(event, -1);
    }
    function _handleDown(event) {
        _handleScrollCommon(event, 1);
    }
    function _handlePageUp(event) {
        _handleScrollCommon(event, -editorService.linesPerBeat);
    }
    function _handlePageDown(event) {
        _handleScrollCommon(event, editorService.linesPerBeat);
    }
    function _handleLeft() {
        selectionService.clear();
        editorService.requestCursorLeft();
    }
    function _handleRight() {
        selectionService.clear();
        editorService.requestCursorRight();
    }
    function _handleTab() {
        selectionService.clear();
        editorService.requestColumnRight();
    }
    function _handleDelete() {
        selectionService.clear();
        if (UiService.editMode()) {
            if (editorService.isAtNoteColumn()) {
                editorService.requestNoteDeletionAtCurrentPosition(false);
                editorService.requestScroll(UiService.activeStep());
            } else if (editorService.isAtVelocityColumn) {
                if (editorService.requestDigitSetAtCurrentPosition(0)) {
                    editorService.requestScroll(UiService.activeStep());
                }
            }
        }
    }
    function _handleEscape() {
        selectionService.clear();
        UiService.toggleEditMode();
    }
    function _handleHome() {
        selectionService.clear();
        if (!UiService.isPlaying()) {
            const position = editorService.position;
            editorService.requestPosition(position.pattern, 0, 0, 0, 0);
        }
    }
    function _handleInsert() {
        selectionService.clear();
        if (UiService.editMode()) {
            editorService.requestNoteInsertionAtCurrentPosition();
        }
    }
    function _handleBackspace() {
        selectionService.clear();
        if (UiService.editMode()) {
            editorService.requestNoteDeletionAtCurrentPosition(true);
        }
    }
    function _handleSpace() {
        selectionService.clear();
        UiService.togglePlay();
    }
    function _handleNoteOff() {
        if (UiService.editMode()) {
            if (editorService.requestNoteOffAtCurrentPosition()) {
                editorService.requestScroll(UiService.activeStep());
            }
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
        if (!event.isAutoRepeat) {
            _handleLiveNoteTriggered(event);
        }
    }
    function _handleNoteInserted(event) {
        const effectiveNote = _effectiveNote(event.key);
        if (effectiveNote) {
            const effectiveOctave = _effectiveOctave(event.key);
            if (editorService.requestNoteOnAtCurrentPosition(effectiveNote, effectiveOctave, UiService.activeVelocity())) {
                editorService.requestScroll(UiService.activeStep());
            }
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
        if (!event.isAutoRepeat) {
            _handleLiveNoteReleased(event);
        }
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
