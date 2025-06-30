import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

Dialog {
    id: rootItem
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    function setTitle(text) {
        title = "<strong>" + text + "</strong>";
    }
    contentItem: ScrollView {
        ListView {
            id: instrumentLayerList
            model: instrumentLayersModel
            spacing: 10
            clip: true
            delegate: EditInstrumentLayersDelegate {
                width: instrumentLayerList.width
                height: implicitHeight
            }
        }
    }
    Text {
        text: qsTr("No instrument layers added")
        anchors.centerIn: parent
        color: "white"
        visible: !instrumentLayerList.count
    }
}
