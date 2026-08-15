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

AnimatedDialog {
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
