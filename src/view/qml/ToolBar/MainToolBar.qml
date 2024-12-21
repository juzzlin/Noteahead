import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Rectangle {
    id: rootItem
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: Constants.mainToolBarColor
        }
        GradientStop {
            position: 1.0
            color: "black"
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
                onValueChanged: UiService.setActiveOctave(octSpinBox.value)
            }
        }
        Rectangle {
            height: parent.height
            width: 1
            color: Constants.mainToolBarSeparatorColor
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
                value: editorService.beatsPerMinute()
                from: 30
                to: 300
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
                value: editorService.linesPerBeat()
                from: 1
                to: 16
                onValueChanged: editorService.setLinesPerBeat(value)
            }
        }
    }
}
