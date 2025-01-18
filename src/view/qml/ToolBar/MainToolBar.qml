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
        id: playerControls
        anchors.left: parent.left
        anchors.leftMargin: Constants.lineNumberColumnWidth
        anchors.verticalCenter: parent.verticalCenter
    }
    Item {
        id: editorControlsContainer
        anchors.left: playerControls.right
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height
        clip: true
        EditorControls {
            id: editorControls
            anchors.right: parent.right
            anchors.rightMargin: Constants.lineNumberColumnWidth
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
