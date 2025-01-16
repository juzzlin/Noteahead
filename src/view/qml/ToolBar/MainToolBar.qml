import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

Rectangle {
    id: rootItem
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: Constants.mainToolBarGradientStartColor
        }
        GradientStop {
            position: 1.0
            color: Constants.mainToolBarGradientStopColor
        }
    }
    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: Constants.lineNumberColumnWidth
        spacing: 5
        PlayButton {
        }
        PrevButton {
        }
        StopButton {
        }
    }
    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: Constants.lineNumberColumnWidth
        spacing: 20
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
                value: editorService.currentLineCount()
                from: editorService.minLineCount()
                to: editorService.maxLineCount()
                editable: true
                onValueChanged: editorService.setCurrentLineCount(patternLengthSpinBox.value)
                Keys.onReturnPressed: {
                    focus = false;
                }
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
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
                onValueChanged: UiService.setActiveStep(stepSpinBox.value)
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
                onValueChanged: UiService.setActiveVelocity(velocitySpinBox.value)
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
                onValueChanged: UiService.setActiveOctave(octSpinBox.value)
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
            }
        }
    }
}
