import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    title: qsTr("Timing")
    Layout.fillWidth: true
    width: parent.width

    function initialize() {
        delaySpinBox.value = columnSettingsModel.delay;
    }

    RowLayout {
        spacing: 10
        Label {
            text: qsTr("Delay (ms):")
        }
        SpinBox {
            id: delaySpinBox
            from: -10000
            to: 10000
            editable: true
            Keys.onReturnPressed: focus = false
            onValueModified: columnSettingsModel.delay = value
        }
    }
}
