import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: rootItem
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    function setTitle(text) {
        title = "<strong>" + text + "</strong>";
    }
    function controller() {
        return model.controller();
    }
    function comment() {
        return model.comment();
    }
    function setComment(comment) {
        model.setComment(comment);
    }
    contentItem: InstrumentLayerModel {
        id: model
    }
}
