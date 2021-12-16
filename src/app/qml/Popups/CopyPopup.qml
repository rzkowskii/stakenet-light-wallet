import QtQuick.Controls 2.2
import QtQuick 2.12

import com.xsn.utils 1.0

Rectangle {
    id: root
    property string message: ""
    width: message.length * 9
    height: 40
    radius: 10
    color: SkinColors.popupFieldBorder
    z: 1000
    opacity: 0
    property bool shown: false

    onShownChanged: {
        if(shown) {
            state = "visible";
        } else {
            state = "hidden";
        }
    }

    states: [
        State {
            name: "hidden"
            PropertyChanges {
                target: root
            }
        },
        State {
            name: "visible"
            PropertyChanges {
                target: root
                opacity: 1.0
            }
        }
    ]

    Text {
        text: message
        anchors.centerIn: parent
        color: SkinColors.mainText
    }


    transitions: [
        Transition {
            from: "hidden"
            to: "visible"
            NumberAnimation {
                target: root
                property: "scale"
                from: 0.4
                to: 1
                duration: 400
                easing.type: Easing.OutCubic
            }
        },
        Transition {
            from: "visible"
            to: "hidden"
            NumberAnimation {
                target: root
                property: "opacity"
                to: 0
                duration: 1000
                easing.type: Easing.OutCubic
            }
        }
    ]

    function startTimer(){
        hideMessageTimer.start();
    }

    Timer {
        id: hideMessageTimer
        interval: 7000;
        onTriggered: {
            shown = false;
        }
    }
}




