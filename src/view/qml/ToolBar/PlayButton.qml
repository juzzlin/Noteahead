import QtQuick 2.15
import ".."

ToolBarButtonBase {
    id: rootItem
    Component.onCompleted: setImageSource("../Graphics/play.svg")
    onClicked: UiService.requestPlay()
    enabled: !UiService.isPlaying()
}
