import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Dialog {
    id: rootItem
    title: qsTr("Delete unused patterns")
    modal: true
    readonly property string _tag: "DeleteUnusedPatternsDialog"

    Label {
        text: qsTr("Are you sure you want to delete all patterns that are not used in the play order?")
        width: parent.width
        wrapMode: Label.WordWrap
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Yes")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                uiLogger.info(rootItem._tag, "Deletion confirmed");
                rootItem.accept();
            }
        }
        Button {
            text: qsTr("No")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: {
                uiLogger.info(rootItem._tag, "Deletion rejected");
                rootItem.reject();
            }
        }
    }

    onAccepted: {
        UiService.confirmDeleteUnusedPatterns();
    }
}
