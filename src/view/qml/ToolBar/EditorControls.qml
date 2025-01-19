import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

Row {
    id: rootItem
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
            Keys.onReturnPressed: focus = false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Add or browse patterns") + ` (${editorService.minPatternIndex()}-${editorService.maxPatternIndex()})`
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
            Keys.onReturnPressed: focus = false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Set length of the current pattern") + ` (${editorService.minLineCount()}-${editorService.maxLineCount()})`
        }
    }
    Separator {
    }
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
            Keys.onReturnPressed: focus = false
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
            Keys.onReturnPressed: focus = false
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
            Keys.onReturnPressed: focus = false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Set base octave when adding new notes")
        }
    }
    Separator {
    }
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
            Keys.onReturnPressed: focus = false
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
            Keys.onReturnPressed: focus = false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Set lines per beat")
        }
    }
}
