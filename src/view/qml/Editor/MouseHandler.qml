import QtQuick 2.15
import ".."

QtObject {
    id: rootItem
    signal editorFocusRequested
    signal contextMenuRequested(int x, int y)
    property int _selectionStartLine: 0
    property int _selectionEndLine: 0
    property bool _isDragging: false
    function handleLeftClicked(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        editorService.requestPosition(track.patternIndex(), track.index(), columnIndex, lineIndex, 0);
        editorFocusRequested();
    }
    function handleRightClicked(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        editorService.requestPosition(track.patternIndex(), track.index(), columnIndex, lineIndex, 0);
        editorFocusRequested();
        UiService.requestContextMenu(x, y);
    }
    function handleLeftPressed(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        if (!UiService.isPlaying()) {
            selectionService.clear();
            _selectionStartLine = lineIndex;
            _selectionEndLine = lineIndex;
            _isDragging = true;
        }
    }
    function handleRightPressed(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
    }
    function handleLeftReleased(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        if (!UiService.isPlaying()) {
            selectionService.requestSelectionEnd(editorService.position.pattern, track.index(), columnIndex, _selectionEndLine);
            _isDragging = false;
        }
    }
    function handleRightReleased(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
    }
    function handleMouseMoved(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        if (!UiService.isPlaying()) {
            if (_isDragging) {
                if (_selectionEndLine !== lineIndex) {
                    _selectionEndLine = lineIndex;
                    selectionService.requestSelectionEnd(editorService.position.pattern, track.index(), columnIndex, _selectionEndLine);
                }
                if (_selectionStartLine !== _selectionEndLine) {
                    selectionService.requestSelectionStart(editorService.position.pattern, track.index(), columnIndex, _selectionStartLine);
                }
            }
        }
    }
}
