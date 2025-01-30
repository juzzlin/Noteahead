import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

Row {
    id: rootItem
    spacing: 20
    anchors.verticalCenter: parent.verticalCenter
    GroupBox {
        id: songGroupBox
        label: Row {
            spacing: 5
            TextField {
                text: qsTr("Song") + ` | ${editorService.currentTime} / ${editorService.duration}`
                padding: 0
                background: null
                readOnly: true
            }
        }
        Row {
            spacing: 20
            anchors.verticalCenter: parent.verticalCenter
            PlayerControls {
                id: playerControls
                anchors.verticalCenter: parent.verticalCenter
            }
            Separator {
            }
            Row {
                spacing: 5
                Text {
                    text: qsTr("POS")
                    font.bold: true
                    color: Constants.mainToolBarTextColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: songPositionSpinBox
                    value: editorService.playOrderSongPosition
                    from: editorService.minSongPosition()
                    to: editorService.maxSongPosition()
                    editable: false
                    enabled: !UiService.isPlaying()
                    onValueChanged: editorService.setPlayOrderSongPosition(value)
                    Keys.onReturnPressed: {
                        focus = false;
                        UiService.requestFocusOnEditorView();
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set the song position where you want to assign a pattern.")
                }
            }
            Row {
                spacing: 5
                Text {
                    text: qsTr("PAT")
                    font.bold: true
                    color: Constants.mainToolBarTextColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: songPatternIndexSpinBox
                    value: editorService.patternAtCurrentPlayOrderSongPosition
                    editable: false
                    enabled: !UiService.isPlaying()
                    onValueChanged: editorService.setPatternAtPlayOrderSongPosition(songPositionSpinBox.value, value)
                    Keys.onReturnPressed: {
                        focus = false;
                        UiService.requestFocusOnEditorView();
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Assign a pattern to the selected position. If the selected index doesn’t exist, a new pattern will be created.")
                }
            }
        }
    }
    GroupBox {
        label: Row {
            spacing: 5
            TextField {
                id: patternLabel
                text: qsTr("Pattern: ")
                padding: 0
                background: null
                readOnly: true
            }
            TextField {
                placeholderText: qsTr("Pattern name")
                text: editorService.currentPatternName
                height: patternLabel.height
                padding: 0
                background: null
                color: Constants.trackHeaderTextColor
                font.bold: true
                verticalAlignment: TextInput.AlignVCenter // Align text inside the field
                Keys.onReturnPressed: {
                    focus = false;
                    UiService.requestFocusOnEditorView();
                }
                onTextChanged: editorService.setCurrentPatternName(text)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set pattern name")
            }
        }
        anchors.top: songGroupBox.top
        anchors.bottom: songGroupBox.bottom
        Row {
            spacing: 20
            anchors.verticalCenter: parent.verticalCenter
            Row {
                spacing: 5
                Text {
                    text: qsTr("PAT")
                    font.bold: true
                    color: Constants.mainToolBarTextColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: patternIndexSpinBox
                    value: editorService.currentPattern
                    from: editorService.minPatternIndex()
                    to: editorService.maxPatternIndex()
                    editable: false
                    enabled: !UiService.isPlaying()
                    onValueChanged: editorService.setCurrentPattern(patternIndexSpinBox.value)
                    Keys.onReturnPressed: {
                        focus = false;
                        UiService.requestFocusOnEditorView();
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set pattern to edit and/or create a new pattern ") + ` (${editorService.minPatternIndex()}-${editorService.maxPatternIndex()})`
                }
            }
            Row {
                spacing: 5
                Text {
                    text: qsTr("LEN")
                    font.bold: true
                    color: Constants.mainToolBarTextColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: patternLengthSpinBox
                    value: editorService.currentLineCount
                    from: editorService.minLineCount()
                    to: editorService.maxLineCount()
                    editable: true
                    enabled: !UiService.isPlaying()
                    onValueChanged: editorService.setCurrentLineCount(patternLengthSpinBox.value)
                    Keys.onReturnPressed: {
                        focus = false;
                        UiService.requestFocusOnEditorView();
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set length of the current pattern") + ` (${editorService.minLineCount()}-${editorService.maxLineCount()})`
                }
            }
        }
    }
    GroupBox {
        label: Row {
            spacing: 5
            TextField {
                text: qsTr("Edit")
                padding: 0
                background: null
                readOnly: true
            }
        }
        anchors.top: songGroupBox.top
        anchors.bottom: songGroupBox.bottom
        Row {
            spacing: 20
            anchors.verticalCenter: parent.verticalCenter
            Row {
                spacing: 5
                Text {
                    text: qsTr("STEP")
                    font.bold: true
                    color: Constants.mainToolBarTextColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: stepSpinBox
                    value: UiService.activeStep()
                    from: 0
                    to: 64
                    editable: true
                    enabled: !UiService.isPlaying()
                    onValueChanged: UiService.setActiveStep(stepSpinBox.value)
                    Keys.onReturnPressed: {
                        focus = false;
                        UiService.requestFocusOnEditorView();
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set editing step")
                }
            }
            Row {
                spacing: 5
                Text {
                    text: qsTr("VEL")
                    font.bold: true
                    color: Constants.mainToolBarTextColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: velocitySpinBox
                    value: UiService.activeVelocity()
                    from: 0
                    to: 127
                    editable: true
                    enabled: !UiService.isPlaying()
                    onValueChanged: UiService.setActiveVelocity(velocitySpinBox.value)
                    Keys.onReturnPressed: {
                        focus = false;
                        UiService.requestFocusOnEditorView();
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set default note velocity (0-127)")
                }
            }
            Row {
                spacing: 5
                Text {
                    text: qsTr("OCT")
                    font.bold: true
                    color: Constants.mainToolBarTextColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: octSpinBox
                    value: UiService.activeOctave()
                    from: 0
                    to: 8
                    editable: true
                    enabled: !UiService.isPlaying()
                    onValueChanged: UiService.setActiveOctave(octSpinBox.value)
                    Keys.onReturnPressed: {
                        focus = false;
                        UiService.requestFocusOnEditorView();
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set base octave when adding new notes")
                }
            }
        }
    }
    GroupBox {
        label: Row {
            spacing: 5
            TextField {
                text: qsTr("Tempo")
                padding: 0
                background: null
                readOnly: true
            }
        }
        anchors.top: songGroupBox.top
        anchors.bottom: songGroupBox.bottom
        Row {
            spacing: 20
            anchors.verticalCenter: parent.verticalCenter
            Row {
                spacing: 5
                Text {
                    text: qsTr("BPM")
                    font.bold: true
                    color: Constants.mainToolBarTextColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: bpmSpinBox
                    value: editorService.beatsPerMinute
                    from: 30
                    to: 300
                    editable: true
                    enabled: !UiService.isPlaying()
                    onValueChanged: editorService.setBeatsPerMinute(value)
                    Keys.onReturnPressed: {
                        focus = false;
                        UiService.requestFocusOnEditorView();
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set beats per minute")
                }
            }
            Row {
                spacing: 5
                Text {
                    text: qsTr("LBP")
                    font.bold: true
                    color: Constants.mainToolBarTextColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                SpinBox {
                    id: lbpSpinBox
                    value: editorService.linesPerBeat
                    from: 1
                    to: 16
                    editable: true
                    enabled: !UiService.isPlaying()
                    onValueChanged: editorService.setLinesPerBeat(value)
                    Keys.onReturnPressed: {
                        focus = false;
                        UiService.requestFocusOnEditorView();
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set lines per beat")
                }
            }
        }
    }
}
