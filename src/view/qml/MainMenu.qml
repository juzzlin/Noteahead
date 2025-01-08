import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

MenuBar {
    Menu {
        title: qsTr("&File")
        Action {
            text: qsTr("&New...")
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("&Quit")
            onTriggered: close()
        }
    }
    Menu {
        title: qsTr("&Help")
        Action {
            text: qsTr("&About")
        }
    }
}
