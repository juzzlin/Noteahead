import QtQuick 2.15

Item {
    id: rootItem
    property int _index: 0
    property var _tracks: []
    function clearTracks() {
        _tracks = [];
    }
    function index() {
        return _index;
    }
    function setIndex(index) {
        _index = index;
    }
}
