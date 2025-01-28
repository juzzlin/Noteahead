import QtQuick 2.15
import ".."
import "../.."

ToolBarButtonBase {
    id: rootItem
    enabled: !UiService.isPlaying()
    Component.onCompleted: setImageSource("../Graphics/prev.svg")
    onClicked: {
        editorService.setPlayOrderSongPosition(0);
        focus = false;
    }
}
