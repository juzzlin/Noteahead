import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import ".."

Dialog {
    id: rootItem
    title: qsTr("Open a recent project")
    modal: true
    signal accepted(string filePath)
    property string selectedFile
    GroupBox {
        title: qsTr("Recent files")
        anchors.fill: parent
        clip: true
        ScrollView {
            anchors.fill: parent
            clip: true
            ListView {
                id: recentFilesList
                model: recentFilesModel
                anchors.fill: parent
                delegate: Item {
                    id: recentFileItem
                    width: recentFilesList.width
                    height: recentFileText.height + 20
                    Label {
                        id: recentFileText
                        text: model.display
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        elide: Text.ElideRight
                        color: hoverHandler.hovered ? Constants.recentFileItemHoveredTextColor : Constants.recentFileItemTextColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            recentFilesDialog.close();
                            recentFilesDialog.selectedFile = model.display;
                            recentFilesDialog.accepted(model.display);
                        }
                        HoverHandler {
                            id: hoverHandler
                            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                }
                focus: true
            }
        }
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Don't open any recent projects")
        }
    }
}
