import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Controls.Universal 2.3
import "PlayerButtons"
import ".."

Row {
    id: rootItem
    anchors.verticalCenter: parent.verticalCenter
    spacing: 5
    PlayButton {
    }
    PrevButton {
    }
    StopButton {
    }
}
