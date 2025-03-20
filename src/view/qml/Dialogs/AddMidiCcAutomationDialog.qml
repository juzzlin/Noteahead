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
        return midiCcAutomationModel.controller();
    }
    function startValue() {
        return midiCcAutomationModel.startValue();
    }
    function setStartValue(value) {
        midiCcAutomationModel.setStartValue(value);
    }
    function endValue(value) {
        return midiCcAutomationModel.endValue();
    }
    function setEndValue(value) {
        midiCcAutomationModel.setEndValue(value);
    }
    function startLine() {
        return midiCcAutomationModel.startLine();
    }
    function setStartLine(value) {
        midiCcAutomationModel.setStartLine(value);
    }
    function endLine() {
        return midiCcAutomationModel.endLine();
    }
    function setEndLine(value) {
        midiCcAutomationModel.setEndLine(value);
    }
    function comment() {
        return midiCcAutomationModel.comment();
    }
    function setComment(comment) {
        midiCcAutomationModel.setComment(comment);
    }
    contentItem: MidiCcAutomationModel {
        id: midiCcAutomationModel
    }
}
