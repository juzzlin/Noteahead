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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

ColumnLayout {
    id: root

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    Label {
        text: qsTr("Algorithm")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    ComboBox {
        model: fmSynthController.algorithmNames
        currentIndex: fmSynthController.algorithm
        onActivated: i => fmSynthController.algorithm = i
        Layout.fillWidth: true
    }

    //! The routing, drawn. Eight algorithms named in words are eight things to memorise; drawn,
    //! which operator feeds which is readable at a glance -- and it is what tells the level knobs
    //! apart, the same knob being loudness on a carrier and brightness on a modulator.
    Canvas {
        id: diagram

        Layout.fillWidth: true
        Layout.preferredHeight: 190
        Layout.topMargin: 10

        readonly property int algorithm: fmSynthController.algorithm
        readonly property color boxColor: themeService.accentColor
        readonly property int boxWidth: 46
        readonly property int boxHeight: 30

        onAlgorithmChanged: requestPaint()
        onBoxColorChanged: requestPaint()

        onPaint: {
            const context = getContext("2d");
            context.reset();

            const operators = fmSynthController.operators;
            const count = operators.length;
            if (!count) {
                return;
            }

            // Who modulates whom, zero based, and who is heard.
            let modulators = [];
            let carriers = [];
            for (let i = 0; i < count; i++) {
                modulators.push(operators[i].modulatedBy.map(one => one - 1));
                carriers.push(operators[i].carrier);
            }

            // A carrier sits on the bottom row; anything modulating an operator sits one row above
            // it. Chains therefore stack upwards and the output runs along the bottom, which is how
            // every FM synth has drawn this since the DX7's front panel.
            let rows = [];
            for (let i = 0; i < count; i++) {
                rows.push(carriers[i] ? 0 : -1);
            }
            for (let pass = 0; pass < count; pass++) {
                for (let target = 0; target < count; target++) {
                    if (rows[target] < 0) {
                        continue;
                    }
                    for (const source of modulators[target]) {
                        rows[source] = Math.max(rows[source], rows[target] + 1);
                    }
                }
            }

            let rowCount = 1;
            for (const row of rows) {
                rowCount = Math.max(rowCount, row + 1);
            }

            // Centre each row's operators across the width, and leave the bottom strip for the
            // output line the carriers run into.
            const outputY = height - 22;
            const rowHeight = Math.min(52, (outputY - 10) / rowCount);
            let positions = [];
            for (let row = 0; row < rowCount; row++) {
                const members = [];
                for (let i = 0; i < count; i++) {
                    if (rows[i] === row) {
                        members.push(i);
                    }
                }
                const span = width / (members.length + 1);
                for (let m = 0; m < members.length; m++) {
                    positions[members[m]] = {
                        "x": span * (m + 1),
                        "y": outputY - 14 - (row + 0.5) * rowHeight
                    };
                }
            }

            // Modulation paths first, so the boxes are drawn over their ends.
            context.strokeStyle = "#888";
            context.lineWidth = 1.5;
            for (let target = 0; target < count; target++) {
                for (const source of modulators[target]) {
                    context.beginPath();
                    context.moveTo(positions[source].x, positions[source].y + diagram.boxHeight / 2);
                    context.lineTo(positions[target].x, positions[target].y - diagram.boxHeight / 2);
                    context.stroke();
                }
            }

            // Carriers down to the output line.
            context.beginPath();
            context.moveTo(10, outputY);
            context.lineTo(width - 10, outputY);
            context.stroke();
            for (let i = 0; i < count; i++) {
                if (carriers[i]) {
                    context.beginPath();
                    context.moveTo(positions[i].x, positions[i].y + diagram.boxHeight / 2);
                    context.lineTo(positions[i].x, outputY);
                    context.stroke();
                }
            }

            // The feedback loop, on the one operator that has it.
            const feedback = count - 1;
            context.beginPath();
            context.arc(positions[feedback].x + diagram.boxWidth / 2 + 7, positions[feedback].y, 9, -Math.PI * 0.8, Math.PI * 0.8);
            context.stroke();

            for (let i = 0; i < count; i++) {
                const x = positions[i].x - diagram.boxWidth / 2;
                const y = positions[i].y - diagram.boxHeight / 2;

                context.fillStyle = carriers[i] ? diagram.boxColor : "#2a2a2a";
                context.fillRect(x, y, diagram.boxWidth, diagram.boxHeight);
                context.strokeStyle = carriers[i] ? diagram.boxColor : "#888";
                context.strokeRect(x, y, diagram.boxWidth, diagram.boxHeight);

                context.fillStyle = carriers[i] ? "#101010" : "#dddddd";
                context.font = "bold 13px sans-serif";
                context.textAlign = "center";
                context.textBaseline = "middle";
                context.fillText(String(i + 1), positions[i].x, positions[i].y);
            }
        }
    }

    Knob {
        label: qsTr("Feedback")
        value: fmSynthController.feedback
        onMoved: v => fmSynthController.feedback = v
        Layout.fillWidth: true
    }

    Label {
        text: qsTr("Feedback is on operator") + " " + fmSynthController.operators.length
        font.pixelSize: 11
        color: "#888"
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }
}
