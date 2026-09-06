// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import Noteahead 1.0
import "../Components"

AnimatedDialog {
    id: rootItem
    modal: true
    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Copy automation...")
            implicitWidth: Constants.defaultButtonWidth
            // An action rather than an accept: the picker fills these fields in and the form stays
            // open on top of it, so nothing is written until Ok.
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: UiService.requestCopyAutomationDialog(true)
            toolTipText: qsTr("Fill these fields from an automation that already exists")
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
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
    function curve(): int {
        return model.curve();
    }
    function setCurve(curve: int): void {
        model.setCurve(curve);
    }
    function resetModulations(): void {
        model.resetModulations();
    }
    function modulationType() {
        return model.modulationType();
    }
    function setModulationType(value: int): void {
        model.setModulationType(value);
    }
    function cycles() {
        return model.cycles();
    }
    function setCycles(value: int): void {
        model.setCycles(value);
    }
    function amplitude() {
        return model.amplitude();
    }
    function setAmplitude(value: int): void {
        model.setAmplitude(value);
    }
    function offset() {
        return model.offset();
    }
    function setOffset(value: int): void {
        model.setOffset(value);
    }
    function inverted() {
        return model.inverted();
    }
    function setInverted(value: bool): void {
        model.setInverted(value);
    }
    function comment() {
        return model.comment();
    }
    function setComment(comment) {
        model.setComment(comment);
    }

    //! Fills every field from an automation that already exists. Its location is deliberately left
    //! out: the new automation belongs where the form was opened, not where the copied one lives.
    //! Pitch bend has no per-beat output settings, so there is nothing of those to carry over.
    function applyValues(values: var): void {
        setStartLine(values.line0);
        setEndLine(values.line1);
        setStartValue(values.value0);
        setEndValue(values.value1);
        setCurve(values.curve);
        setModulationType(values.modulationType);
        setCycles(values.modulationCycles);
        setAmplitude(values.modulationAmplitude);
        setOffset(values.modulationOffset);
        setInverted(values.modulationInverted);
        setComment(values.comment);
    }
    contentItem: PitchBendAutomationModel {
        id: model
    }
}
