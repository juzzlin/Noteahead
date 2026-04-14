import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    title: qsTr("Instrument")
    Layout.fillWidth: true
    width: parent.width

    function initialize() {
        transposeSpinBox.value = columnSettingsModel.transpose;
    }

    RowLayout {
        spacing: 10
        Label {
            text: qsTr("Transpose:")
        }
        SpinBox {
            id: transposeSpinBox
            from: Constants.transposeMin
            to: Constants.transposeMax
            editable: true
            Keys.onReturnPressed: focus = false
            onValueModified: columnSettingsModel.transpose = value
        }
    }
}
