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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// A text field with a calendar to pick from. The field stays freely editable on purpose: a date
// tag is a string, existing projects may hold anything in it, from a bare year to a whole sentence,
// and a picker that refused those would lose them. The calendar only offers to fill it in.
RowLayout {
    id: rootItem
    spacing: 4
    property alias text: textField.text
    property string placeholderText: ""
    signal edited(string text)
    readonly property string _tag: "DateField"
    //! Dates are written ISO 8601, which is what Vorbis comments and ID3 expect.
    function _toIsoDate(date: date): string {
        return Qt.formatDate(date, "yyyy-MM-dd");
    }
    //! The month the calendar opens on: the one already in the field when it holds a date we can
    //! read, and the current month otherwise.
    function _dateOrToday(): date {
        const parsed = Date.fromLocaleDateString(Qt.locale(), textField.text, "yyyy-MM-dd");
        return isNaN(parsed.getTime()) ? new Date() : parsed;
    }
    function _pick(date: date): void {
        textField.text = rootItem._toIsoDate(date);
        rootItem.edited(textField.text);
        calendarPopup.close();
    }
    TextField {
        id: textField
        Layout.fillWidth: true
        placeholderText: rootItem.placeholderText
        onTextEdited: rootItem.edited(text)
        Keys.onReturnPressed: focus = false
    }
    AppButton {
        id: calendarButton
        text: "\u{1F4C5}"
        implicitWidth: height
        onClicked: {
            const date = rootItem._dateOrToday();
            monthGrid.month = date.getMonth();
            monthGrid.year = date.getFullYear();
            calendarPopup.open();
        }
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Pick a date")
        // Declared inside the button rather than beside it in the layout: a Popup is not an Item,
        // but sitting in a RowLayout's default property it still reads as something laid out.
        Popup {
            id: calendarPopup
            y: calendarButton.height
            x: -width + calendarButton.width
            modal: true
            focus: true
            padding: 10
            ColumnLayout {
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton {
                        text: "◀"
                        implicitWidth: height
                        onClicked: {
                            if (monthGrid.month === 0) {
                                monthGrid.month = 11;
                                monthGrid.year--;
                            } else {
                                monthGrid.month--;
                            }
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: monthGrid.title
                    }
                    AppButton {
                        text: "▶"
                        implicitWidth: height
                        onClicked: {
                            if (monthGrid.month === 11) {
                                monthGrid.month = 0;
                                monthGrid.year++;
                            } else {
                                monthGrid.month++;
                            }
                        }
                    }
                }
                DayOfWeekRow {
                    Layout.fillWidth: true
                    locale: monthGrid.locale
                    delegate: Label {
                        text: model.shortName
                        color: themeService.accentColor
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
                MonthGrid {
                    id: monthGrid
                    Layout.fillWidth: true
                    delegate: Label {
                        text: model.day
                        // The days either side of the month are drawn dimmed rather than hidden,
                        // so that the grid keeps its shape.
                        opacity: model.month === monthGrid.month ? 1 : 0.35
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: date => rootItem._pick(date)
                }
                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("Today")
                    onClicked: rootItem._pick(new Date())
                }
            }
        }
    }
}
