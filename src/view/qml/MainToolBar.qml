import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

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
        PlayButton {}
        PrevButton {}
        StopButton {}
    }
    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: Constants.lineNumberColumnWidth
        spacing: 20
        Row {
            spacing: 5
            Text {
                text: "BPM"
                font.bold: true
                color: Constants.mainToolBarTextColor
                anchors.verticalCenter: parent.verticalCenter
            }
            SpinBox {
                id: bpmSpinBox
                value: 120
                from: 30
                to: 300
            }
        }
        Row {
            spacing: 5
            Text {
                text: "LBP"
                font.bold: true
                color: Constants.mainToolBarTextColor
                anchors.verticalCenter: parent.verticalCenter
            }
            SpinBox {
                id: lbpSpinBox
                value: 4
                from: 1
                to: 16
            }
        }
        Row {
            spacing: 5
            Text {
                text: "VEL"
                font.bold: true
                color: Constants.mainToolBarTextColor
                anchors.verticalCenter: parent.verticalCenter
            }
            SpinBox {
                id: velSpinBox
                value: 100
                from: 0
                to: 127
            }
        }
        Row {
            spacing: 5
            Text {
                text: "OCT"
                font.bold: true
                color: Constants.mainToolBarTextColor
                anchors.verticalCenter: parent.verticalCenter
            }
            SpinBox {
                id: octSpinBox
                value: 4
                from: 0
                to: 8
            }
        }
    }
}
