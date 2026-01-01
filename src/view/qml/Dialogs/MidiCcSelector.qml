import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtQuick.Controls.Universal
import ".."

RowLayout {
    Layout.fillWidth: true
    signal settingsChanged
    signal removeRequested
    property int index
    property bool showRemoveButton: false
    property bool showEnabled: true
    property bool showValue: true
    readonly property alias currentController: midiCcControllerComboBox.currentValue
    function initialize(enabled, controller, value) {
        enableCcCheckbox.checked = enabled;
        midiCcControllerComboBox.currentIndex = midiCcControllerComboBox.indexOfValue(controller);
        midiCcValueSpinBox.value = value;
    }
    function enabled() {
        return enableCcCheckbox.checked;
    }
    function setEnabled(enabled) {
        enableCcCheckbox.checked = enabled;
    }
    function controller() {
        return midiCcControllerComboBox.currentValue;
    }
    function setController(value) {
        midiCcControllerComboBox.currentIndex = midiCcControllerComboBox.indexOfValue(value);
    }
    function value() {
        return midiCcValueSpinBox.value;
    }
    function setValue(value) {
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
    ComboBox {
        id: midiCcControllerComboBox
        Layout.fillWidth: true
        Layout.preferredWidth: 300
        model: propertyService.availableMidiControllers
        textRole: "name"
        valueRole: "number"
        editable: true
        enabled: enableCcCheckbox.checked
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Set MIDI Continuous Controller number. See the MIDI CC implementation chart of your device. Current selection: ") + midiCcControllerComboBox.currentText
        onActivated: settingsChanged()
        delegate: ItemDelegate {
            text: modelData.name
            highlighted: midiCcControllerComboBox.highlightedIndex === index
            Universal.theme: Universal.Dark
        }
        popup: Popup {
            y: midiCcControllerComboBox.height - 1
            width: midiCcControllerComboBox.width
            implicitHeight: contentItem.implicitHeight > 300 ? 300 : contentItem.implicitHeight
            padding: 1
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: midiCcControllerComboBox.popup.visible ? midiCcControllerComboBox.delegateModel : null
                currentIndex: midiCcControllerComboBox.highlightedIndex
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
