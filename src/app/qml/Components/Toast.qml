import QtQuick 2.10
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.12
//import QtMultimedia 5.9

Popup {
    id: popup
    property alias text: toastText.text
    property bool error: true
    property bool autoClose: true
    property int autoCloseInterval: 3000
    property string soundSource: error ? "qrc:/Sounds/error.wav" : "qrc:/Sounds/notification.wav"
    default property alias controlsContent: controls.data

    parent: Overlay.overlay
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    y: 20 + WindowCaptionHeight
    x: (parent.width - width) / 2

    onVisibleChanged: {
        if (visible) {
            fadeInAnimation.start()
//            sound.play()
            if (autoClose) {
                autoCloseTimer.start()
            }
        }
    }

    background: Item {
        DropShadow {
            z: -1
            anchors.fill: parent
            verticalOffset: 4
            radius: 8
            samples: 17
            color: Qt.rgba(0,0,0,0.24)
            source: back
            transparentBorder: true
        }

        Rectangle {
            id: back
            anchors.fill: parent
            color: error ? Style.dangerColor : Style.successColor
            radius: 4
        }
    }

    contentItem: RowLayout {
        id: controls
        spacing: 8
        Label {
            id: toastText
            padding: 16
            topPadding: 8
            bottomPadding: 8
            font.bold: true
            Layout.alignment: Qt.AlignCenter

            MouseArea {
                anchors.fill: parent
                onClicked: popup.close()
            }            
        }
    }

    SequentialAnimation on opacity {
        id: fadeInAnimation

        NumberAnimation {
            from: 0; to: 1
            easing.type: Easing.OutCubic; duration: 500
        }
    }

    Timer {
        id: autoCloseTimer
        interval: popup.autoCloseInterval
        onTriggered: {
            stop()
            popup.close()
        }
    }

//    SoundEffect {
//        id: sound
//        source: popup.soundSource
//    }
}
