import QtQuick 2.15
import ".."
import "../.."

ToolBarButtonBase {
    id: rootItem
    enabled: !UiService.isPlaying()
    Component.onCompleted: {
        setImageSource("../Graphics/replay.png");
        setScale(0.9);
    }
    onClicked: {
        setToggled(!isToggled());
        UiService.setIsLooping(isToggled());
        focus = false;
        UiService.requestFocusOnEditorView();
    }
    toolTipText: qsTr("Loop the current pattern")
}
