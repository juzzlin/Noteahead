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
    Item {
        id: editorControlsContainer
        anchors.left: parent.left
        anchors.leftMargin: Constants.lineNumberColumnWidth
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height
        clip: true
        EditorControls {
            id: editorControls
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
