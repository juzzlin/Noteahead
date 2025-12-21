import QtQuick 2.15
import ".."

QtObject {
    function handleKeyPressed(event) {
        if (keyboardService.handleKeyPressed(event.key, event.modifiers, event.isAutoRepeat)) {
            event.accepted = true;
        }
    }
    function handleKeyReleased(event) {
        if (keyboardService.handleKeyReleased(event.key, event.modifiers, event.isAutoRepeat)) {
            event.accepted = true;
        }
    }
}