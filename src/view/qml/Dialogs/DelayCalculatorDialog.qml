import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: delayCalculatorDialog
    title: qsTr("Delay Time Calculator")
    modal: true
    clip: true
    standardButtons: Dialog.Ok
    property real bpm: 120
    readonly property string _defaultNoteDuration: "1/4"
    property string noteDuration: _defaultNoteDuration
    property real delayMs: 0
    signal delayCalculated(real ms)
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        clip: true
        RowLayout {
            Layout.fillWidth: true
            ComboBox {
                id: noteDurationCombo
                model: ["1/1", "3/4"  // dotted 1/2
                    , "1/2", "3/8"  // dotted 1/4
                    , "1/3"  // triplet half note
                    , "1/4", "3/16" // dotted 1/8
                    , "1/6"  // triplet quarter
                    , "1/8", "3/32" // dotted 1/16
                    , "1/12" // triplet 1/8
                    , "1/16", "3/64" // dotted 1/32
                    , "1/24" // triplet 1/16
                    , "1/32", "1/64"]
                currentIndex: 2
                onCurrentTextChanged: {
                    noteDuration = currentText;
                    calculateDelay();
                }
            }
            SpinBox {
                id: bpmSpinBox
                from: 30
                to: 300
                value: bpm
                stepSize: 1
                editable: true
                onValueChanged: {
                    bpm = value;
                    calculateDelay();
                }
            }
            Label {
                text: qsTr("BPM")
                verticalAlignment: Label.AlignVCenter
                padding: 4
            }
            Label {
                id: resultLabel
                Layout.fillWidth: true
                text: {
                    const hz = delayMs > 0 ? (1000 / delayMs).toFixed(2) : "--";
                    return `= ${delayMs.toFixed(2)} ms (${hz} Hz)`;
                }
                font.bold: true
                font.pixelSize: bpmSpinBox.height
            }
        }
    }
    function calculateDelay(): void {
        const beatDuration = 60000 / bpm; // duration of a 1/4 note beat in ms

        // 1. Split the string (e.g., "3/4") into numerator and denominator
        const parts = noteDuration.split("/");
        const numerator = parseFloat(parts[0]);
        const denominator = parseFloat(parts[1]);

        let fraction = 0;
        if (denominator > 0) {
            // 2. Calculate the fractional value (e.g., 3/4 = 0.75, 1/8 = 0.125)
            fraction = numerator / denominator;
        }

        // 3. The delay is the duration of a quarter note (beatDuration) multiplied by the fraction,
        //    scaled by 4 to get the duration relative to a whole note,
        //    OR, simply: delay = fraction * duration_of_whole_note
        //    Since T_whole = 4 * T_1/4 = 4 * beatDuration:
        delayMs = fraction * 4 * beatDuration;
        delayCalculated(delayMs);
    }
    Component.onCompleted: {
        noteDurationCombo.currentIndex = noteDurationCombo.find(_defaultNoteDuration);
    }
}
