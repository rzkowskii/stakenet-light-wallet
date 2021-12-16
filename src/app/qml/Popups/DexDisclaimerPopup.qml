import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Qt.labs.settings 1.0
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    width: 658
    height: 520
    leftPadding: 1
    topPadding: 1
    closePolicy: Popup.NoAutoClose


    Settings {
        id: settings
        category: "Wallet"
        property bool ignoreDisclaimer: false
    }

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    FontLoader {
        id: fontLight
        source: "qrc:/Rubik-Light.ttf"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 32

        Image {
            sourceSize: Qt.size(656, 224)
            source: "qrc:/images/Dex_disclaimer_image"

            Text {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.bottomMargin: 23
                anchors.rightMargin: 27
                font.family: fontLight.name
                font.pixelSize: 12
                color: SkinColors.dexDisclaimerTextColor
                font.weight: Font.Light
                text: "Copyright © 2020 XSN Core Ltd."
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 13

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 17
                font.family: fontLight.name
                color: SkinColors.dexDisclaimerTextColor
                font.weight: Font.ExtraLight
                lineHeight: 1.2
                text: "This software is experimental and is meant for testing purposes only.\nIt is currently in beta and developers hold no liability or responsibility\nfor any loss of funds or bugs that may occur during this phase."
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 17
                font.family: fontLight.name
                color: SkinColors.dexDisclaimerTextColor
                font.weight: Font.ExtraLight
                text: "Upon agreement, you accept these terms and conditions period. If you do not\nagree with these terms, please close the wallet and do not use this software."
            }

            RowLayout {
                Layout.preferredHeight: 10
                Layout.fillWidth: true
                Layout.leftMargin: 25
                spacing: 5

                CheckBox {
                    id: checkbox
                    checked: settings.ignoreDisclaimer

                    indicator: Rectangle {
                        implicitWidth: 17
                        implicitHeight: 17
                        x: checkbox.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 6
                        border.color: SkinColors.mainText
                        color: "transparent"
                        border.width: 1


                        ColorOverlayImage {
                            width: 15
                            height: 15
                            x: 2
                            y: 1
                            imageSize: 15
                            color: SkinColors.mainText
                            imageSource: "qrc:/images/checkmark-round.png"
                            visible: checkbox.checked
                        }
                    }

                    PointingCursorMouseArea {
                        onClicked:  {
                           checkbox.checked = !checkbox.checked
                           settings.ignoreDisclaimer = checkbox.checked
                        }

                        onEntered:  {
                            checkbox.scale = scale * 1.1
                        }
                        onExited: {
                            checkbox.scale = scale
                        }
                    }
                }

                XSNLabel {
                    font.weight: Font.Thin
                    text: "Ignore this message in the future"
                    font.family: regularFont.name
                    font.pixelSize: 12
                    color: SkinColors.mainText

                }
            }
        }


        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 15

            IntroButton {
                Layout.preferredHeight: 40
                Layout.preferredWidth: 120
                buttonHoveredColor: SkinColors.cancelButtonHoveredBgrColor
                buttonColor: SkinColors.cancelButtonColor

                buttonGradientHoveredColor: SkinColors.cancelButtonGradientHoveredColor
                buttonGradientColor: SkinColors.cancelButtonGradienColor
                font.pixelSize: 12
                text: "Cancel"
                onClicked: reject()
            }

            IntroButton {
                id: activateButton
                Layout.preferredHeight: 40
                Layout.preferredWidth: 120
                font.pixelSize: 12
                text: "I agree"
                onClicked: accept()
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
