import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import Noteahead 1.0
import "../Components"

Dialog {
    id: root
    title: applicationService.bassSynthDeviceName
    modal: true
    focus: true
    width: 900
    height: 700

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    onAboutToShow: () => {
        bassSynthController.requestSettings();
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: () => {
            bassSynthController.accept();
        }
        onRejected: () => {
            bassSynthController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        RowLayout {
            id: mainRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            // Fixed Sidebar: Global settings
            ColumnLayout {
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop
                
                BassSynthDialog_Global {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
                Item { Layout.fillHeight: true } // Spacer
            }

            // Vertical Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // Synthesis Area
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                
                ColumnLayout {
                    width: parent.width - 20
                    spacing: 20

                    BassSynthDialog_Oscillator {
                        Layout.fillWidth: true
                    }

                    BassSynthDialog_Filter {
                        Layout.fillWidth: true
                    }

                    BassSynthDialog_Distortion {
                        Layout.fillWidth: true
                    }
                }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => bassSynthController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => bassSynthController.stopNote(note)
        }
    }
}
