import QtQuick 2.4
import QtQuick.Controls 2.2

import "../Components"
import com.xsn.utils 1.0

TextField {
    id: root
    property int validated: Enums.ValidationState.None
    property alias beckgroundItem: backGrd
    selectByMouse: true
    color: "#fff"
    opacity: enabled ? 1 : .38
    leftPadding: 15
    rightPadding: 40
    echoMode: TextInput.Password

    background: Rectangle {
        id: backGrd
        color: "#090E2B"
        border.width: 2
        border.color: "#7377A5"

        IconButton {
            id: showPswBtn
            anchors {
                right: parent.right
                rightMargin: 15
                verticalCenter: parent.verticalCenter
            }
            iconColor: SkinColors.mainText
            sourceSize: Qt.size(19, checked ? 19 : 17)
            checkable: true
            source: checked ? "qrc:/images/ic_eye_shut.png" : "qrc:/images/ic_eyeopen.png"

            onCheckedChanged: {
                if (checked) {
                    root.echoMode = TextInput.Normal
                } else {
                    root.echoMode = TextInput.Password
                }
            }

            PointingCursorMouseArea {
                anchors.fill: parent

                onClicked: {
                    showPswBtn.toggle()
                }
            }
        }
    }

    Rectangle {
        id: statusIndicator
        state: root.validated
        states: [
            State {
                name: Enums.ValidationState.None
                PropertyChanges {
                    target: statusIndicator
                    color: "transparent" // root.text.length > 0 ? SkinColors.mainText :  "transparent"
                }
            },
            State {
                name: Enums.ValidationState.Validated
                PropertyChanges {
                    target: statusIndicator
                    color: SkinColors.mainText //"green"
                }
            },
            State {
                name: Enums.ValidationState.Failed
                PropertyChanges {
                    target: statusIndicator
                    color: "red"
                }
            }
        ]

        height: 2
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }

    function validatePassword(password) {
        if (password.length < 8) {
            return false
        } else {
            var hasNumber = /\d/
            var hasSmallLetter = /[a-z]/
            var hasUpperLetter = /[A-Z]/

            if (!hasNumber.test(password) || !hasSmallLetter.test(password)
                    || !hasUpperLetter.test(password)) {
                return false
            } else {
                return true
            }
        }
    }
}
