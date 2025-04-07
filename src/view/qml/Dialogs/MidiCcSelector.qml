import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtQuick.Controls.Universal
import ".."

RowLayout {
    Layout.fillWidth: true
    signal settingsChanged
    property int index
    property bool showEnabled: true
    property bool showValue: true
    property bool _initializing: false
    function initialize(enabled, controller, value) {
        _initializing = true;
        enableCcCheckbox.checked = enabled;
        midiCcControllerSpinBox.value = controller;
        midiCcValueSpinBox.value = value;
        _initializing = false;
    }
    function enabled() {
        return enableCcCheckbox.checked;
    }
    function setEnabled(enabled) {
        enableCcCheckbox.checked = enabled;
    }
    function controller() {
        return midiCcControllerSpinBox.value;
    }
    function setController(value) {
        midiCcControllerSpinBox.value = value;
    }
    function value() {
        return midiCcValueSpinBox.value;
    }
    function setValue(value) {
        midiCcValueSpinBox.value = value;
    }
    function _notify() {
        if (!_initializing) {
            settingsChanged();
        }
    }
    CheckBox {
        id: enableCcCheckbox
        text: qsTr("Enable MIDI CC #") + index
        Layout.fillWidth: true
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Enable/disable MIDI Continuous Controller setting slot #") + index
        onCheckedChanged: _notify()
    }
    Label {
        text: qsTr("Controller / Value:")
        Layout.fillWidth: true
        elide: Text.ElideRight
    }
    SpinBox {
        id: midiCcControllerSpinBox
        from: 0
        to: 127
        enabled: enableCcCheckbox.checked
        Layout.fillWidth: true
        editable: true
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Set MIDI Continuous Controller number. See the MIDI CC implementation chart of your device.")
        onValueChanged: _notify()
        Keys.onReturnPressed: focus = false
    }
    Label {
        text: `( ${trackSettingsModel.midiCcToString(midiCcControllerSpinBox.value)} )`
        horizontalAlignment: Text.AlignHCenter
        Layout.preferredWidth: 200
        elide: Text.ElideRight
        enabled: enableCcCheckbox.enabled
    }
    SpinBox {
        id: midiCcValueSpinBox
        from: 0
        to: 127
        enabled: enableCcCheckbox.checked
        Layout.fillWidth: true
        editable: true
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Set optional MIDI Continuous Controller value")
        onValueChanged: _notify()
        Keys.onReturnPressed: focus = false
    }
}
