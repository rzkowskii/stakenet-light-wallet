import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    property var model: undefined

    popUpText: "Transaction address"
    width: 450
    height: 230


    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }
    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 22
        anchors.bottomMargin: 25
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 25

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            XSNLabel {
                font.family: mediumFont.name
                font.pixelSize: 20
                text: "Transaction addresses"
                color: SkinColors.mainText
            }

            Item {
                Layout.fillWidth: true
            }

            CloseButton {
                Layout.preferredHeight: 30
                Layout.preferredWidth: 30
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                onClicked: root.close()
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.model
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            delegate: Rectangle {
                height: 30
                width: parent.width
                color: mouseArea.containsMouse ? SkinColors.transactionItemSelectedBackground : SkinColors.menuBackground

                PointingCursorMouseArea {
                    id: mouseArea
                  }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 5
                    anchors.rightMargin: 5

                    XSNLabel {
                        id: address
                        font.pixelSize: 14
                        text: modelData
                    }

                    ColorOverlayImage {
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredHeight: 20
                        Layout.preferredWidth: 20
                        visible: mouseArea.containsMouse
                        imageSource: "qrc:/images/copy.png"
                        imageSize: 20
                        color: SkinColors.headerText

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                Clipboard.setText(address.text)
                                showBubblePopup("Copied");
                            }
                        }
                    }
                }
            }
        }
    }
}
