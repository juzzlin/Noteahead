import QtQuick 2.15
import ".."
import "../.."

ToolBarButtonBase {
    id: rootItem
    enabled: !UiService.isPlaying()
    Component.onCompleted: {
        setImageSource("../Graphics/prev.svg");
        setScale(1.0);
    }
    onClicked: {
        UiService.rewindSong();
        focus = false;
        UiService.requestFocusOnEditorView();
    }
    toolTipText: qsTr("Rewind to the start of the song and reset track settings")
}
