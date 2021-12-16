import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import com.xsn.utils 1.0

Page {
    id: root
    property alias backButton: backBtn
    property alias skipButton: skipBtn
    property alias backImage: backImage
    property string headerSource: ""
    property string headerText: ""

    header: RowLayout {
        Layout.preferredHeight: 100
        Layout.fillWidth: true

        spacing: 35

        Item {
            id: backBtnItem
            Layout.preferredWidth: 20
            Layout.preferredHeight: 45

            Layout.leftMargin: 50
            Layout.topMargin: 30

            visible: !backBtn.visible
        }

        Button {
            id: backBtn
            Layout.preferredWidth: 20
            Layout.preferredHeight: 45

            Layout.leftMargin: 50
            Layout.topMargin: 30

            background: Rectangle {
                anchors.fill: parent
                color: "transparent"
            }

            contentItem: ColorOverlayImage {
                anchors.fill: parent
                imageSize: 23
                color: SkinColors.mainText
                imageSource: "qrc:/images/ICON_BACK.png"
            }

            PointingCursorMouseArea {
                onClicked: backBtn.clicked()
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Image {
            source: "qrc:/images/logo_welcome_page.png"
            sourceSize: Qt.size(225, 56)
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 15
            visible: false

            Layout.topMargin: 30

            ColorOverlayImage {
                id: backImage
                Layout.alignment: Qt.AlignVCenter
                imageSize: 35
                width: imageSize
                height: imageSize
                color: SkinColors.mainText
                imageSource: headerSource
                visible: backButton.visible
            }

            XSNLabel {
                Layout.alignment: Qt.AlignVCenter
                text: headerText
                visible: backBtn.visible
                font.pixelSize: 24
            }
        }



        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Button {
            id: skipBtn
            Layout.preferredWidth: 20
            Layout.preferredHeight: 45

            Layout.rightMargin: 50
            Layout.topMargin: 30

            background: Rectangle {
                anchors.fill: parent
                color: "transparent"
            }

            contentItem: ColorOverlayImage {
                anchors.fill: parent
                imageSize: 23
                color: SkinColors.mainText
                imageSource: "qrc:/images/ICON_FORWARD.png"
            }

            PointingCursorMouseArea {
                onClicked: {
                    stackView.push(setupNewWalletComponent)
                    skipBtn.clicked()
                }
            }
        }

        Item {
            id: skipBtnItem
            Layout.preferredWidth: 20
            Layout.preferredHeight: 45
            visible: !skipBtn.visible

            Layout.rightMargin: 50
            Layout.topMargin: 30
        }
    }
}
