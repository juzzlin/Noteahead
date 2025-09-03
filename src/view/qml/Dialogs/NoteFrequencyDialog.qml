import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import ".."

Dialog {
    id: rootItem
    title: qsTr("Note Frequencies")
    modal: true
    standardButtons: Dialog.Ok

    // Tunable reference (A4)
    property real referenceFrequency: 440.0

    function buildModel(): var {
        const names = ["C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"];
        const rows = [];
        for (let midi = 12; midi <= 119; ++midi) {
            const name = names[midi % 12];
            const octave = Math.floor(midi / 12) - 1; // MIDI => octave
            const freq = referenceFrequency * Math.pow(2, (midi - 69) / 12);
            rows.push({
                    "noteLabel": name + octave,
                    "note": name,
                    "octave": octave,
                    "frequency": freq,
                    "midi": midi
                });
        }
        return rows;
    }

    property var rows: buildModel()

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: "Equal Temperament Note Frequencies (A4 = " + rootItem.referenceFrequency + " Hz) - C0...B8"
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        // Header row
        Item {
            Layout.fillWidth: true
            height: 36
            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 12
                Text {
                    text: qsTr("Note")
                    font.bold: true
                    color: Constants.mainMenuTextColor
                    Layout.preferredWidth: 140
                }
                Text {
                    text: qsTr("Octave")
                    font.bold: true
                    color: Constants.mainMenuTextColor
                    Layout.preferredWidth: 80
                }
                Text {
                    text: qsTr("Frequency (Hz)")
                    font.bold: true
                    color: Constants.mainMenuTextColor
                    Layout.preferredWidth: 200
                    horizontalAlignment: Text.AlignRight
                }
                Text {
                    text: qsTr("MIDI")
                    font.bold: true
                    color: Constants.mainMenuTextColor
                    Layout.preferredWidth: 80
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        // List (scrollable)
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: listView
                anchors.fill: parent
                model: rootItem.rows
                clip: true
                spacing: 0

                delegate: Rectangle {
                    height: 32
                    width: listView.width
                    color: index % 2 === 0 ? "#222222" : "#444444"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 12

                        Text {
                            text: modelData.noteLabel
                            color: Constants.mainMenuTextColor
                            Layout.preferredWidth: 140
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            text: String(modelData.octave)
                            color: Constants.mainMenuTextColor
                            Layout.preferredWidth: 80
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            text: Number(modelData.frequency).toFixed(2)
                            color: Constants.mainMenuTextColor
                            Layout.preferredWidth: 200
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            text: String(modelData.midi)
                            color: Constants.mainMenuTextColor
                            Layout.preferredWidth: 80
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                }
            }
        }

        Label {
            text: qsTr("Tip: Kick fundamentals often sit around E1–G1 (≈41–49 Hz). Sub bass commonly spans C1–G2 (≈33–98 Hz).")
            wrapMode: Text.WordWrap
            opacity: 0.85
            Layout.fillWidth: true
        }
    }
}
