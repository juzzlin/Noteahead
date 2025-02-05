import QtQuick 2.15
import ".."
import "../.."

ToolBarButtonBase {
    id: rootItem
    enabled: !UiService.isPlaying()
    Component.onCompleted: setImageSource("../Graphics/prev.svg")
    onClicked: {
        editorService.setPlayOrderSongPosition(0);
        editorService.requestPosition(0, 0, 0, 0, 0);
        focus = false;
        UiService.requestFocusOnEditorView();
    }
    toolTipText: qsTr("Rewind to the start of the song")
}
