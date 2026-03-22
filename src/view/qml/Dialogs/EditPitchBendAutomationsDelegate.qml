import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../ToolBar"

GroupBox {
    id: delegateRoot
    title: `Pattern: ${model.pattern}, Track: ${model.track}, Column: ${model.column}`

    function initialize(): void {
        if (model) {
            startLineSpinBox.value = model.line0;
            endLineSpinBox.value = model.line1;
            startValueSpinBox.value = model.value0;
            endValueSpinBox.value = model.value1;
            modulationTypeComboBox.currentIndex = model.modulationType;
            modulationCyclesSpinBox.value = model.modulationCycles;
            modulationAmplitudeSpinBox.value = model.modulationAmplitude;
            modulationOffsetSpinBox.value = model.modulationOffset;
            modulationInvertedCheckBox.checked = model.modulationInverted;
            commentEdit.text = model.comment;
            enableCheckbox.checked = model.enabled;
        }
    }

    GridLayout {
        anchors.fill: parent
        columns: 10
        rowSpacing: 10
        CheckBox {
            id: enableCheckbox
            text: qsTr("Enabled")
            Layout.row: 0
            Layout.column: 0
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Enable/disable the automation")
            onCheckedChanged: model.enabled = checked
            Component.onCompleted: checked = model.enabled
        }
        GroupBox {
            title: qsTr("Interpolation")
            Layout.row: 0
            Layout.column: 1
            Layout.columnSpan: 8
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 4
                Label {
                    text: qsTr("Start line")
                    Layout.row: 0
                    Layout.column: 0
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: startLineSpinBox
                    from: 0
                    to: 999
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 0
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The line where the interpolation starts")
                    onValueModified: model.line0 = value
                }
                Label {
                    text: qsTr("End line")
                    Layout.row: 0
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: endLineSpinBox
                    from: 0
                    to: 999
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The line where the interpolation ends")
                    onValueModified: model.line1 = value
                }
                Label {
                    text: qsTr("Start value (%)")
                    Layout.row: 0
                    Layout.column: 2
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: startValueSpinBox
                    from: -100
                    to: 100
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 2
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The pitch bend value at the start line (-100% to 100%)")
                    onValueModified: model.value0 = value
                }
                Label {
                    text: qsTr("End value (%)")
                    Layout.row: 0
                    Layout.column: 3
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: endValueSpinBox
                    from: -100
                    to: 100
                    editable: true
                    enabled: model.line0 !== model.line1
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 3
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The pitch bend value at the end line (-100% to 100%)")
                    onValueModified: model.value1 = value
                }
            }
        }
        GroupBox {
            title: qsTr("Modulation")
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 8
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 5
                Label {
                    text: qsTr("Type")
                    Layout.row: 0
                    Layout.column: 0
                    Layout.fillWidth: true
                }
                ComboBox {
                    id: modulationTypeComboBox
                    model: [qsTr("Sine Wave"), qsTr("Random")]
                    Layout.row: 1
                    Layout.column: 0
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The type of modulation to apply (Sine Wave or Random)")
                    onActivated: function (comboBoxIndex) {
                        pitchBendAutomationsModel.changeModulationType(index, comboBoxIndex);
                    }
                }
                Label {
                    text: qsTr("Cycles")
                    Layout.row: 0
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: modulationCyclesSpinBox
                    from: 0
                    to: 127
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The number of modulation cycles over the automation range")
                    onValueModified: model.modulationCycles = value
                }
                Label {
                    text: qsTr("Amplitude (%)")
                    Layout.row: 0
                    Layout.column: 2
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: modulationAmplitudeSpinBox
                    from: 0
                    to: 200
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 2
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The strength of the modulation as a percentage of the pitch bend half-range")
                    onValueModified: model.modulationAmplitude = value
                }
                Label {
                    text: qsTr("Offset (%)")
                    Layout.row: 0
                    Layout.column: 3
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: modulationOffsetSpinBox
                    from: -100
                    to: 100
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 3
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("An additive constant value applied to the modulation")
                    onValueModified: model.modulationOffset = value
                }
                CheckBox {
                    id: modulationInvertedCheckBox
                    text: qsTr("Inverted")
                    Layout.row: 1
                    Layout.column: 4
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Invert the phase of the modulation")
                    onCheckedChanged: model.modulationInverted = checked
                    Component.onCompleted: checked = model.modulationInverted
                }
            }
        }
        Button {
            id: deleteButton
            Layout.row: 0
            Layout.rowSpan: 2
            Layout.column: 9
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Delete this automation")
            Image {
                id: backgroundImage
                source: "../Graphics/delete.png"
                sourceSize: Qt.size(parent.width, parent.height)
                width: parent.width
                height: parent.height
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
            }
            onClicked: pitchBendAutomationsModel.removeAt(index)
        }
        TextField {
            id: commentEdit
            readOnly: false
            placeholderText: qsTr("Comment")
            Layout.row: 2
            Layout.column: 1
            Layout.columnSpan: 8
            Layout.fillWidth: true
            Keys.onReturnPressed: focus = false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Optional comment for this automation")
            onTextChanged: model.comment = text
            Component.onCompleted: text = model.comment
        }
    }
    Component.onCompleted: initialize()
}
