import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts

Item {
    id: rootItem
    function controller(): int {
        return midiCcSelector.controller();
    }
    function setController(controller: int): void {
        return midiCcSelector.setController(controller);
    }
    function startValue(): int {
        return startValueSpinBox.value;
    }
    function setStartValue(value: int): void {
        startValueSpinBox.value = value;
    }
    function endValue(): int {
        return endValueSpinBox.value;
    }
    function setEndValue(value: int): void {
        endValueSpinBox.value = value;
    }
    function startLine() {
        return startLineSpinBox.value;
    }
    function setStartLine(value: int): void {
        startLineSpinBox.value = value;
    }
    function endLine() {
        return endLineSpinBox.value;
    }
    function setEndLine(value: int): void {
        endLineSpinBox.value = value;
    }

    function resetModulations(): void {
        setCycles(0);
        setAmplitude(0);
        setInverted(false);
    }
    function cycles(): int {
        return cyclesSpinBox.value;
    }
    function setCycles(cycles: int): void {
        cyclesSpinBox.value = cycles;
    }
    function amplitude(): int {
        return amplitudeSpinBox.value;
    }
    function setAmplitude(amplitude: int): void {
        amplitudeSpinBox.value = amplitude;
    }
    function inverted(): bool {
        return invertedCheckBox.checked;
    }
    function setInverted(inverted: bool): void {
        invertedCheckBox.checked = inverted;
    }

    function comment(): string {
        return commentEdit.text;
    }
    function setComment(comment: string): void {
        commentEdit.text = comment;
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        GroupBox {
            title: qsTr("Controller number")
            Layout.fillWidth: true
            MidiCcSelector {
                id: midiCcSelector
                Component.onCompleted: {
                    showEnabled = false;
                    showValue = false;
                    setEnabled(true);
                }
            }
        }
        GroupBox {
            title: qsTr("Interpolation settings")
            Layout.fillWidth: true
            GridLayout {
                rowSpacing: 10
                width: parent.width
                Label {
                    text: qsTr("Start line")
                    width: parent.width
                    Layout.row: 0
                    Layout.column: 0
                }
                SpinBox {
                    id: startLineSpinBox
                    from: 0
                    to: 999
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 0
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("End line")
                    Layout.row: 0
                    Layout.column: 1
                }
                SpinBox {
                    id: endLineSpinBox
                    from: 0
                    to: 999
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("Start value")
                    Layout.row: 0
                    Layout.column: 2
                }
                SpinBox {
                    id: startValueSpinBox
                    from: propertyService.minValue(midiCcSelector.currentController)
                    to: propertyService.maxValue(midiCcSelector.currentController)
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.fillWidth: true
                    Layout.row: 1
                    Layout.column: 2
                }
                Label {
                    text: qsTr("End value")
                    Layout.row: 0
                    Layout.column: 3
                }
                SpinBox {
                    id: endValueSpinBox
                    from: propertyService.minValue(midiCcSelector.currentController)
                    to: propertyService.maxValue(midiCcSelector.currentController)
                    value: 0
                    editable: true
                    enabled: startLine() !== endLine()
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.row: 1
                    Layout.column: 3
                    Layout.fillWidth: true
                }
            }
        }
        GroupBox {
            title: qsTr("Modulation (Sine Wave)")
            Layout.fillWidth: true
            GridLayout {
                rowSpacing: 10
                width: parent.width
                columns: 3
                Label {
                    text: qsTr("Cycles")
                }
                Label {
                    text: qsTr("Amplitude (%)")
                }
                Label {
                    text: qsTr("Inverted")
                }
                SpinBox {
                    id: cyclesSpinBox
                    from: 0
                    to: 127
                    value: 0
                    editable: true
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: amplitudeSpinBox
                    from: 0
                    to: 100
                    value: 0
                    editable: true
                    stepSize: 1
                    Keys.onReturnPressed: {
                        focus = false;
                    }
                    Layout.fillWidth: true
                }
                CheckBox {
                    id: invertedCheckBox
                }
            }
        }
        GroupBox {
            title: qsTr("Comment")
            Layout.fillWidth: true
            TextField {
                id: commentEdit
                readOnly: false
                width: parent.width
                Keys.onReturnPressed: {
                    focus = false;
                }
            }
        }
    }
}