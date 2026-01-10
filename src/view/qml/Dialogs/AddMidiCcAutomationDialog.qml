import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: rootItem
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    function setTitle(text: var): void {
        title = `<strong>${text}</strong>`;
    }
    function controller(): var {
        return model.controller();
    }
    function setController(controller: int): void {
        model.setController(controller);
    }
    function startValue(): int {
        return model.startValue();
    }
    function setStartValue(value): void {
        model.setStartValue(value);
    }
    function endValue(): int {
        return model.endValue();
    }
    function setEndValue(value: int): void {
        model.setEndValue(value);
    }
    function startLine(): int {
        return model.startLine();
    }
    function setStartLine(value: int): void {
        model.setStartLine(value);
    }
    function endLine(): int {
        return model.endLine();
    }
    function setEndLine(value: int): void {
        model.setEndLine(value);
    }

    function eventsPerBeat(): int {
        return model.eventsPerBeat();
    }
    function setEventsPerBeat(value: int): void {
        model.setEventsPerBeat(value);
    }
    function lineOffset(): int {
        return model.lineOffset();
    }
    function setLineOffset(value: int): void {
        model.setLineOffset(value);
    }

    function cycles(): int {
        return model.cycles();
    }
    function amplitude(): int {
        return model.amplitude();
    }
    function inverted(): bool {
        return model.inverted();
    }
    function resetModulations(): void {
        model.resetModulations();
    }
    function resetOutput(): void {
        model.resetOutput();
    }

    function comment(): string {
        return model.comment();
    }
    function setComment(comment: string): void {
        model.setComment(comment);
    }

    contentItem: MidiCcAutomationModel {
        id: model
    }
}
