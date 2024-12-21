import QtQuick 2.15
import ".."

ToolBarButtonBase {
    id: rootItem
    Component.onCompleted: setImageSource("../Graphics/stop.svg")
    onClicked: UiService.requestStop()
    enabled: UiService.isPlaying()
}
