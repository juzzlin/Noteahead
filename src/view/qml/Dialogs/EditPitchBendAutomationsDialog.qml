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
            id: pitchBendAutomationsList
            model: pitchBendAutomationsModel
            spacing: 10
            clip: true
            delegate: EditPitchBendAutomationsDelegate {
                width: pitchBendAutomationsList.width
                height: implicitHeight
            }
        }
    }
}
