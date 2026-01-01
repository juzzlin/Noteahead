import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../ToolBar"

GroupBox {
    title: `Pattern: ${model.pattern}, Track: ${model.track}, Column: ${model.column}`
    function initialize(): void {
        if (model && model.controller !== undefined) {
            controllerComboBox.currentIndex = controllerComboBox.indexOfValue(model.controller);
        } else {
            // Fallback if model.controller is undefined (should not happen in practice if model is valid)
            controllerComboBox.currentIndex = 0;
        }
    }
    GridLayout {
        anchors.fill: parent
        CheckBox {
            id: enableCheckbox
            text: qsTr("Enabled")
            Layout.row: 1
            Layout.column: 0
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Enable/disable the automation")
            onCheckedChanged: model.enabled = checked
            Component.onCompleted: checked = model.enabled
        }
        GroupBox {
            title: qsTr("Interpolation")
            GridLayout {
                anchors.fill: parent
                Label {
                    text: qsTr("Controller")
                    Layout.row: 0
                    Layout.column: 1
                    Layout.fillWidth: true
                }
                ComboBox {
                    id: controllerComboBox
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                    model: propertyService.availableMidiControllers
                    textRole: "name"
                    valueRole: "number"
                    editable: true
                    // function() needed due to index binding
                    onActivated: function () {
                        midiCcAutomationsModel.changeController(index, currentValue);
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Controller. Current selection: ") + controllerComboBox.currentText
                    delegate: ItemDelegate {
                        text: modelData.name
                        highlighted: controllerComboBox.highlightedIndex === index
                        Universal.theme: Universal.Dark
                    }
                    popup: Popup {
                        y: controllerComboBox.height - 1
                        width: controllerComboBox.width
                        implicitHeight: contentItem.implicitHeight > 300 ? 300 : contentItem.implicitHeight
                        padding: 1

                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: controllerComboBox.popup.visible ? controllerComboBox.delegateModel : null
                            currentIndex: controllerComboBox.highlightedIndex

                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AlwaysOn
                            }
                        }

                        background: Rectangle {
                            color: "#303030"
                            border.color: "#606060"
                        }
                    }
                }
                Label {
                    text: qsTr("Start line")
                    Layout.row: 0
                    Layout.column: 2
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: startLineSpinBox
                    from: 0
                    to: 999
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 2
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Start line")
                    onValueChanged: model.line0 = value
                    Component.onCompleted: value = model.line0
                }
                Label {
                    text: qsTr("End line")
                    Layout.row: 0
                    Layout.column: 3
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: endLineSpinBox
                    from: 0
                    to: 999
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 3
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("End line")
                    onValueChanged: model.line1 = value
                    Component.onCompleted: value = model.line1
                }
                Label {
                    text: qsTr("Start value")
                    Layout.row: 0
                    Layout.column: 4
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: startValueSpinBox
                    from: propertyService.minValue(model.controller)
                    to: propertyService.maxValue(model.controller)
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 4
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Start value")
                    onValueChanged: model.value0 = value
                    Component.onCompleted: value = model.value0
                }
                Label {
                    text: qsTr("End value")
                    Layout.row: 0
                    Layout.column: 5
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: endValueSpinBox
                    from: propertyService.minValue(model.controller)
                    to: propertyService.maxValue(model.controller)
                    editable: true
                    enabled: model.line0 !== model.line1
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 5
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("End value")
                    onValueChanged: model.value1 = value
                    Component.onCompleted: value = model.value1
                }
            }
        }
        GroupBox {
            title: qsTr("Modulation (Sine Wave)")
            GridLayout {
                anchors.fill: parent
                Label {
                    text: qsTr("Cycles")
                    Layout.row: 0
                    Layout.column: 6
                    Layout.fillWidth: true
                }
                SpinBox {
                    from: 0
                    to: 127
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 6
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Cycles")
                    onValueChanged: model.modulationSineCycles = value
                    Component.onCompleted: value = model.modulationSineCycles
                }
                Label {
                    text: qsTr("Amplitude (%)")
                    Layout.row: 0
                    Layout.column: 7
                    Layout.fillWidth: true
                }
                SpinBox {
                    from: 0
                    to: 100
                    editable: true
                    Keys.onReturnPressed: focus = false
                    Layout.row: 1
                    Layout.column: 7
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Amplitude")
                    onValueChanged: model.modulationSineAmplitude = value
                    Component.onCompleted: value = model.modulationSineAmplitude
                }
                CheckBox {
                    text: qsTr("Inverted")
                    Layout.row: 1
                    Layout.column: 8
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Invert the phase of the sine wave")
                    onCheckedChanged: model.modulationSineInverted = checked
                    Component.onCompleted: checked = model.modulationSineInverted
                }
            }
        }
        Button {
            id: rootItem
            Layout.row: 1
            Layout.rowSpan: 2
            Layout.column: 9
            Layout.columnSpan: 2
            Layout.fillWidth: true
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Delete")
            Image {
                id: backgroundImage
                source: "../Graphics/delete.svg"
                sourceSize: Qt.size(parent.width, parent.height)
                width: parent.width
                height: parent.height
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
            }
            onClicked: midiCcAutomationsModel.removeAt(index)
        }
        TextField {
            id: commentEdit
            readOnly: false
            placeholderText: qsTr("Comment")
            Layout.row: 2
            Layout.columnSpan: 9
            Layout.fillWidth: true
            Keys.onReturnPressed: focus = false
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Comment")
            onTextChanged: model.comment = text
            Component.onCompleted: text = model.comment
        }
    }
    // Ensure initial value is set on component creation
    Component.onCompleted: initialize()
}
