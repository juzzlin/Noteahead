import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: delayCalculatorDialog
    title: qsTr("Delay Time Calculator")
    modal: true
    standardButtons: Dialog.Ok
    property real bpm: 120
    readonly property string _defaultNoteDuration: "1/4"
    property string noteDuration: _defaultNoteDuration
    property real delayMs: 0
    signal delayCalculated(real ms)
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
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
        const beatDuration = 60000 / bpm; // duration of 1 beat in ms
        const denominator = parseInt(noteDuration.split("/")[1]);
        delayMs = denominator > 0 ? 4 * beatDuration / denominator : 0;
        delayCalculated(delayMs);
    }
    Component.onCompleted: {
        noteDurationCombo.currentIndex = noteDurationCombo.find(_defaultNoteDuration);
    }
}
