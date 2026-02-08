import QtQuick 2.15
import ".."
import "../.."

ToolBarButtonBase {
    id: rootItem
    enabled: !UiService.isPlaying()
    Component.onCompleted: {
        setImageSource("../Graphics/play.png");
        setScale(1.0);
    }
    onClicked: {
        UiService.requestPlay();
        focus = false;
        UiService.requestFocusOnEditorView();
    }
    toolTipText: qsTr("Start playing from the current position")
}
