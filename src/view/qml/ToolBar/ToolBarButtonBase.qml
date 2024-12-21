import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 2.15
import ".."

Button {
    id: rootItem
    width: Constants.mainToolBarButtonSize
    height: width
    function setImageSource(imageSource) {
        background.source = imageSource;
    }
    background: Image {
        sourceSize: Qt.size(parent.width, parent.height) // Makes the SVG look "good"
        width: parent.width
        height: parent.height
        fillMode: Image.PreserveAspectFit
        opacity: rootItem.enabled ? 1.0 : 0.5
    }
}
