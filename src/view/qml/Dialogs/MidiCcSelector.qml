import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtQuick.Controls.Universal
import ".."

RowLayout {
    Layout.fillWidth: true
    signal settingsChanged
    property int index
    function initialize() {
        enableCcCheckbox.checked = trackSettingsModel.midiCcEnabled(index);
        midiCcControllerSpinBox.value = trackSettingsModel.midiCcController(index);
        midiCcValueSpinBox.value = trackSettingsModel.midiCcValue(index);
    }
    CheckBox {
        id: enableCcCheckbox
        text: qsTr("Enable MIDI CC #") + index
        Layout.fillWidth: true
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Enable/disable MIDI Continuous Controller setting slot #") + index
        onCheckedChanged: {
            trackSettingsModel.setMidiCcEnabled(index, checked);
            settingsChanged();
        }
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
        ToolTip.text: qsTr("Set optional MIDI Continuous Controller number. See the MIDI CC implementation chart of your device.")
        onValueChanged: {
            trackSettingsModel.setMidiCcController(index, value);
            settingsChanged();
        }
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
        onValueChanged: {
            trackSettingsModel.setMidiCcValue(index, value);
            settingsChanged();
        }
        Keys.onReturnPressed: focus = false
    }
}
