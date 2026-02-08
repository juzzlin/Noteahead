import QtQuick 2.15
import ".."
import "../.."

ToolBarButtonBase {
    id: rootItem
    enabled: UiService.isPlaying()
    Component.onCompleted: {
        setImageSource("../Graphics/stop.png");
        setScale(1.0);
    }
    onClicked: {
        UiService.requestStop();
        focus = false;
        UiService.requestFocusOnEditorView();
    }
    toolTipText: qsTr("Stop playing")
}
