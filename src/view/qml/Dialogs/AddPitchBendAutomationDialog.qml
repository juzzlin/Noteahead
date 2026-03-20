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
    function startValue() {
        return model.startValue();
    }
    function setStartValue(value) {
        model.setStartValue(value);
    }
    function endValue() {
        return model.endValue();
    }
    function setEndValue(value) {
        model.setEndValue(value);
    }
    function startLine() {
        return model.startLine();
    }
    function setStartLine(value) {
        model.setStartLine(value);
    }
    function endLine() {
        return model.endLine();
    }
    function setEndLine(value) {
        model.setEndLine(value);
    }
    function modulationType() {
        return model.modulationType();
    }
    function cycles() {
        return model.cycles();
    }
    function amplitude() {
        return model.amplitude();
    }
    function offset() {
        return model.offset();
    }
    function inverted() {
        return model.inverted();
    }
    function comment() {
        return model.comment();
    }
    function setComment(comment) {
        model.setComment(comment);
    }
    contentItem: PitchBendAutomationModel {
        id: model
    }
}
