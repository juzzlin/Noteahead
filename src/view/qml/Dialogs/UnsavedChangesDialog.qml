import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs

Dialog {
    id: rootItem
    title: qsTr("Save unsaved changes?")
    modal: true
    readonly property string _tag: "UnsavedChangesDialog"
    footer: DialogButtonBox {
        Button {
            text: qsTr("Yes")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                uiLogger.info(_tag, "Unsaved changes accepted");
                applicationService.acceptUnsavedChangesDialog();
                close();
            }
        }
        Button {
            text: qsTr("Close without saving")
            DialogButtonBox.buttonRole: DialogButtonBox.NoRole
            onClicked: {
                uiLogger.info(_tag, "Unsaved changes discarded");
                applicationService.discardUnsavedChangesDialog();
                close();
            }
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: {
                uiLogger.info(_tag, "Unsaved changes rejected");
                applicationService.rejectUnsavedChangesDialog();
                close();
            }
        }
    }
}
