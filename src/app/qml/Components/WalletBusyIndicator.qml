import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick 2.12
import QtQuick.Controls 2.12

BusyIndicator {
    id: control

    property string color: ""

    contentItem: Item {
        implicitWidth: 22
        implicitHeight: 22

        Item {
            id: item
            x: parent.width / 2 - width / 2
            y: parent.height / 2 - height / 2
            width: 22
            height: 22
            opacity: control.running ? 1 : 0

            Behavior on opacity {
                OpacityAnimator {
                    duration: 250
                }
            }

            RotationAnimator {
                target: item
                running: control.visible && control.running
                from: 0
                to: 360
                loops: Animation.Infinite
                duration: 1250
            }

            Repeater {
                id: repeater
                model: 6

                Rectangle {
                    x: item.width / 2 - width / 2
                    y: item.height / 2 - height / 2
                    implicitWidth: 4
                    implicitHeight: 4
                    radius: 2
                    color: control.color
                    transform: [
                        Translate {
                            y: -Math.min(item.width, item.height) * 0.5 + 2
                        },
                        Rotation {
                            angle: index / repeater.count * 360
                            origin.x: 2
                            origin.y: 2
                        }
                    ]
                }
            }
        }
    }
}
