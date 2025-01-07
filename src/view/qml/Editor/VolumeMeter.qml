import QtQuick 2.15

Item {
    id: rootItem
    property real _volume: 0.0 // Normalized volume (0.0 to 1.0)
    property real _animatedVolume: 0.0
    property real _timeStep: 0.0
    readonly property real _maxHeight: 0.8
    readonly property real _fallingScale: 0.001
    Item {
        id: levelIndicatorGradientClip
        width: parent.width * 0.2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        height: parent.height * _animatedVolume * _maxHeight
        clip: true
        Rectangle {
            id: levelIndicatorGradient
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            height: rootItem.height * _maxHeight
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: "red"
                }
                GradientStop {
                    position: 0.5
                    color: "yellow"
                }
                GradientStop {
                    position: 1.0
                    color: "green"
                }
            }
        }
    }
    Timer {
        id: fallTimer
        interval: 17 // ~60 FPS
        repeat: true
        running: false
        onTriggered: {
            if (_animatedVolume > 0) {
                _animatedVolume -= _timeStep * _timeStep;
                _timeStep += interval * _fallingScale;
            } else {
                running = false;
            }
        }
    }
    function trigger(normalizedVolume) {
        if (normalizedVolume >= 0.0 && normalizedVolume <= 1.0) {
            _volume = normalizedVolume;
            if (_volume > 0) {
                _animatedVolume = _volume;
                _timeStep = 0;
                fallTimer.restart();
            }
        }
    }
}
