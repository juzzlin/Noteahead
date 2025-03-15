import QtQuick 2.15
import ".."
import "../.."

ToolBarButtonBase {
    id: rootItem
    enabled: !UiService.isPlaying()
    Component.onCompleted: {
        setImageSource("../Graphics/replay.svg");
        setScale(0.9);
    }
    onClicked: {
        setToggled(!toggled());
        UiService.setIsLooping(toggled());
        focus = false;
        UiService.requestFocusOnEditorView();
    }
    toolTipText: qsTr("Loop the current pattern")
}
