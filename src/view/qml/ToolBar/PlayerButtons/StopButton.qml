import QtQuick 2.15
import ".."
import "../.."

ToolBarButtonBase {
    id: rootItem
    enabled: UiService.isPlaying()
    Component.onCompleted: setImageSource("../Graphics/stop.svg")
    onClicked: {
        UiService.requestStop();
        focus = false;
    }
}
