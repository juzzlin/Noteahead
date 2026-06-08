import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import Noteahead 1.0
import "../Components"

Dialog {
    id: root
    title: applicationService.wavetableSynthDeviceName
    modal: true
    focus: true
    width: 900
    height: 700
    clip: true
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    onAboutToShow: () => {
        wavetableSynthController.requestSettings();
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
            wavetableSynthController.accept();
        }
        onRejected: () => {
            wavetableSynthController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        RowLayout {
            id: mainRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            readonly property real sidebarWidth: width * 0.18
            readonly property real synthAreaWidth: width - sidebarWidth - separator.width - 20
            readonly property real moduleWidth: (synthAreaWidth - (20 * 2) - 30) / 3

            ColumnLayout {
                Layout.preferredWidth: mainRow.sidebarWidth
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                WavetableSynthDialog_Global {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
                Item {
                    Layout.fillHeight: true
                }
            }

            Rectangle {
                id: separator
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
                Layout.leftMargin: 10
                Layout.rightMargin: 10
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10
                StackLayout {
                    id: synthStackLayout
                    currentIndex: synthTabBar.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.bottomMargin: 10
                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 2
                            columnSpacing: 20
                            width: parent.width - 20
                            WavetableSynthDialog_Osc1 {
                                Layout.preferredWidth: mainRow.moduleWidth * 1.5
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                            WavetableSynthDialog_Osc2 {
                                Layout.preferredWidth: mainRow.moduleWidth * 1.5
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                            WavetableSynthDialog_Noise {
                                Layout.preferredWidth: mainRow.moduleWidth * 1.5
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                Layout.columnSpan: 2
                                Layout.topMargin: 10
                            }
                        }
                    }
                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 2
                            columnSpacing: 20
                            width: parent.width - 20
                            WavetableSynthDialog_Filter {
                                Layout.preferredWidth: mainRow.moduleWidth * 1.5
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                            WavetableSynthDialog_Envelopes {
                                Layout.preferredWidth: mainRow.moduleWidth * 1.5
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                        }
                    }
                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 1
                            columnSpacing: 20
                            width: parent.width - 20
                            WavetableSynthDialog_Lfo {
                                Layout.preferredWidth: mainRow.moduleWidth * 1.5
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                        }
                    }
                }
                TabBar {
                    id: synthTabBar
                    Layout.fillWidth: true
                    TabButton {
                        text: qsTr("Oscillators")
                    }
                    TabButton {
                        text: qsTr("Filter / Envelopes")
                    }
                    TabButton {
                        text: qsTr("LFO")
                    }
                }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => wavetableSynthController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => wavetableSynthController.stopNote(note)
        }
    }
}
