import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import "PlayerButtons"
import ".."

Row {
    id: rootItem
    anchors.verticalCenter: parent.verticalCenter
    spacing: 5
    PlayButton {}
    PrevButton {}
    StopButton {}
    LoopButton {}
}
