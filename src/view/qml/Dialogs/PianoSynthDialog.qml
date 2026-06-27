import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import Noteahead 1.0
import "../Components"

Dialog {
    id: root
    title: applicationService.pianoSynthDeviceName
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
        pianoSynthController.requestSettings();
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
            pianoSynthController.accept();
        }
        onRejected: () => {
            pianoSynthController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            // Fixed Sidebar: Global settings
            ColumnLayout {
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                PianoSynthDialog_Global {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
                Item { Layout.fillHeight: true }
            }

            // Vertical Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // String section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                PianoSynthDialog_String {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
                Item { Layout.fillHeight: true }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => pianoSynthController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => pianoSynthController.stopNote(note)
        }
    }
}
