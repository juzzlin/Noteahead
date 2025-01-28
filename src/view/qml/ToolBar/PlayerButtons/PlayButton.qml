import QtQuick 2.15
import ".."
import "../.."

ToolBarButtonBase {
    id: rootItem
    enabled: !UiService.isPlaying()
    Component.onCompleted: setImageSource("../Graphics/play.svg")
    onClicked: {
        UiService.requestPlay();
        focus = false;
        UiService.requestFocusOnEditorView();
    }
    toolTipText: qsTr("Start playing from the current position")
}
