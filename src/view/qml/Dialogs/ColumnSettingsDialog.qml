import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."
import "../Components"

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Column settings for track %1, column %2").arg(columnSettingsModel.trackIndex + 1).arg(columnSettingsModel.columnIndex + 1) + "</strong>"
    modal: true
    function initialize() {
        instrumentSettings.initialize();
        timingSettings.initialize();
        midiEffects.initialize();
        tabBar.currentIndex = 0;
    }
    function saveSettings() {
        columnSettingsModel.save();
    }
    function setColumn(trackIndex, columnIndex) {
        columnSettingsModel.trackIndex = trackIndex;
        columnSettingsModel.columnIndex = columnIndex;
        columnSettingsModel.requestData();
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                saveSettings();
                rootItem.accepted();
            }
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: rootItem.rejected()
        }
    }
    Column {
        anchors.fill: parent
        spacing: 10

        StackLayout {
            height: parent.height - tabBar.height - parent.spacing
            width: parent.width
            currentIndex: tabBar.currentIndex

            ScrollView {
                id: instrumentScrollView
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                rightPadding: 10
                ColumnSettingsDialog_InstrumentSettings {
                    id: instrumentSettings
                    width: instrumentScrollView.availableWidth
                }
            }

            ScrollView {
                id: timingScrollView
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                rightPadding: 10
                ColumnSettingsDialog_TimingSettings {
                    id: timingSettings
                    width: timingScrollView.availableWidth
                }
            }

            ScrollView {
                id: midiEffectsScrollView
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                rightPadding: 10
                ColumnSettingsDialog_MidiEffects {
                    id: midiEffects
                    width: midiEffectsScrollView.availableWidth
                }
            }
        }

        TabBar {
            id: tabBar
            width: parent.width
            TabButton {
                text: qsTr("Instrument")
            }
            TabButton {
                text: qsTr("Timing")
            }
            TabButton {
                text: qsTr("MIDI Effects")
            }
        }
    }

    Component.onCompleted: {
        columnSettingsModel.dataReceived.connect(initialize);
    }
}
