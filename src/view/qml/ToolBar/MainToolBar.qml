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
    PlayerControls {
        anchors.left: parent.left
        anchors.leftMargin: Constants.lineNumberColumnWidth
        anchors.verticalCenter: parent.verticalCenter
    }
    EditorControls {
        anchors.right: parent.right
        anchors.rightMargin: Constants.lineNumberColumnWidth
        anchors.verticalCenter: parent.verticalCenter
    }
}
