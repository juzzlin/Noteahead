import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import ".."

Rectangle {
    id: rootItem
    height: editorControlsContainer.height
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
    ScrollView {
        id: editorControlsContainer
        anchors.left: parent.left
        anchors.leftMargin: Constants.lineNumberColumnWidth
        anchors.right: parent.right
        anchors.rightMargin: Constants.lineNumberColumnWidth
        anchors.top: parent.top
        height: editorControlsWrapper.height
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
        contentWidth: editorControls.width
        // Wrapper to allow vertical centering of content
        Item {
            id: editorControlsWrapper
            width: rootItem.width
            height: editorControls.height + 40
            anchors.verticalCenter: parent.verticalCenter
            EditorControls {
                id: editorControls
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
