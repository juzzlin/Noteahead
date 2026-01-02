import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtQuick.Controls.Universal
import ".."
import "../Components"

RowLayout {
    Layout.fillWidth: true
    signal settingsChanged
    signal removeRequested
    property int index
    property bool showRemoveButton: false
    property bool showEnabled: true
    property bool showValue: true
    readonly property alias currentController: midiCcControllerComboBox.currentValue
    function initialize(enabled: bool, controller: int, value: int): void {
        enableCcCheckbox.checked = enabled;
        midiCcControllerComboBox.setController(controller);
        midiCcValueSpinBox.value = value;
    }
    function enabled(): bool {
        return enableCcCheckbox.checked;
    }
    function setEnabled(enabled: bool): void {
        enableCcCheckbox.checked = enabled;
    }
    function controller(): int {
        return midiCcControllerComboBox.currentValue;
    }
    function setController(value: int): void {
        midiCcControllerComboBox.setController(value);
    }
    function value(): int {
        return midiCcValueSpinBox.value;
    }
    function setValue(value: int): void {
        midiCcValueSpinBox.value = value;
    }
    CheckBox {
        id: enableCcCheckbox
        text: qsTr("Enable")
        visible: showEnabled
        Layout.fillWidth: true
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Enable/disable MIDI Continuous Controller setting slot #") + index
        onToggled: settingsChanged()
    }
    Label {
        text: showValue ? qsTr("Controller and value:") : qsTr("Controller:")
        Layout.fillWidth: true
        elide: Text.ElideRight
    }
    MidiCcComboBox {
        id: midiCcControllerComboBox
        Layout.fillWidth: true
        Layout.preferredWidth: 300
        onActivated: settingsChanged()
    }
    SpinBox {
        id: midiCcValueSpinBox
        from: 0
        to: 127
        enabled: enableCcCheckbox.checked
        Layout.fillWidth: true
        editable: true
        visible: showValue
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Set optional MIDI Continuous Controller value")
        onValueModified: settingsChanged()
        Keys.onReturnPressed: focus = false
    }
    Button {
        text: "✕"
        onClicked: removeRequested()
        visible: showRemoveButton
        Layout.preferredWidth: 40
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Remove this MIDI CC setting")
    }
}
