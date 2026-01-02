import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtQuick.Controls.Universal
import ".."
import "../.."

ComboBox {
    id: root
    signal controllerChanged(int controller)
    model: propertyService.availableMidiControllers
    textRole: "name"
    valueRole: "number"
    editable: true
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.timeout: Constants.toolTipTimeout
    ToolTip.visible: hovered
    ToolTip.text: qsTr("Controller. Current selection: ") + root.currentText
    delegate: ItemDelegate {
        width: root.width
        text: modelData.name
        highlighted: root.highlightedIndex === index
        Universal.theme: Universal.Dark
    }
    popup: Popup {
        y: root.height - 1
        width: root.width
        implicitHeight: contentItem.implicitHeight > 300 ? 300 : contentItem.implicitHeight
        padding: 1
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
            }
        }
        background: Rectangle {
            color: "#303030"
            border.color: "#606060"
        }
    }
    function setController(value: int): void {
        currentIndex = indexOfValue(value);
    }
    onActivated: controllerChanged(currentValue)
}
