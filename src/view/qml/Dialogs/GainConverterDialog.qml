import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts

Dialog {
    id: gainConverterDialog
    title: qsTr("Gain Converter (dB => linear)")
    modal: true
    clip: true
    standardButtons: Dialog.Ok
    property real dbValue: 0.0
    property real linearValue: 1.0
    property bool updating: false
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        clip: true
        RowLayout {
            Layout.fillWidth: true
            SpinBox {
                id: dbSpinBox
                from: -60
                to: 24
                value: dbValue
                stepSize: 1
                editable: true
                onValueChanged: {
                    gainConverterDialog.dbValue = value;
                    gainConverterDialog.calculateFromDb();
                }
            }
            Label {
                text: qsTr("dB")
                verticalAlignment: Label.AlignVCenter
                padding: 4
            }
            Label {
                id: linearFromDbLabel
                Layout.fillWidth: true
                text: `= ${gainConverterDialog.linearValue.toFixed(3)}`
                font.bold: true
                font.pixelSize: dbSpinBox.height
            }
        }
    }
    function calculateFromDb(): void {
        linearValue = Math.pow(10, dbValue / 20);
        linearFromDbLabel.text = `= ${linearValue.toFixed(3)}`;
    }
    Component.onCompleted: calculateFromDb()
}
