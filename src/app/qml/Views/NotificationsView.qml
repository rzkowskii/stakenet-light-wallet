import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

StackLayout {
    id: stackLayout
    currentIndex: 1

    property bool showNotifications: currentIndex === 0

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }
    FontLoader {
        id: fontMedium
        source: "qrc:/Rubik-Medium.ttf"
    }

    NotificationsModel {
        id: notificationsModel

        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true

        Rectangle {
            anchors.fill: parent
            color: SkinColors.notificationViewBackground
            border.width: 0.5
            border.color: SkinColors.popupFieldBorder
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                color: SkinColors.secondaryBackground

                RowLayout {
                    anchors.topMargin: 20
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.left: parent.left
                    spacing: 5

                    Image {
                        source: "qrc:/images/bell.png"
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Notifications"
                        font.family: fontMedium.name
                        font.pixelSize: 14
                        color: SkinColors.mainText
                        font.weight: Font.DemiBold
                    }
                }

                PointingCursorMouseArea {
                    anchors.fill: parent
                    onClicked: stackLayout.currentIndex = 1
                }
            }

            ListView {
                id: notificationsListView
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.topMargin: 15
                Layout.leftMargin: 7
                boundsBehavior: Flickable.StopAtBounds
                spacing: 4
                clip: true
                model: notificationsModel

                ScrollBar.vertical: ScrollBar {
                    id: scrollBar
                    size: parent.height / parent.contentHeight - 5
                }

                delegate: Item {
                    id: delegateItem
                    width: parent.width - scrollBar.width
                    height: 64

                    Rectangle {
                        anchors.fill: parent
                        color: mouseArea.containsMouse ? SkinColors.headerBackground : "transparent"
                        border.width: 1
                        border.color: SkinColors.popupFieldBorder
                        radius: 3
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.leftMargin: 1
                            Layout.fillHeight: true
                            Layout.preferredWidth: 4
                            color: mouseArea.containsMouse ? SkinColors.menuBackgroundGradienRightLine : "transparent"
                        }

                        Item {
                            Layout.preferredWidth: 50
                            Layout.fillHeight: true

                            Image {
                                anchors.centerIn: parent
                                source: {
                                    switch (model.type) {
                                    case Enums.NotificationType.Withdraw:
                                        return "qrc:/images/IC_WITHDRAW.png"
                                    case Enums.NotificationType.Swap:
                                        return "qrc:/images/ic_ln_notif.png"
                                    case Enums.NotificationType.Deposit:
                                        return "qrc:/images/IC_DEPOSIT.png"
                                    case Enums.NotificationType.BuyOrderPlaced:
                                        return "qrc:/images/IC_BUY_24.png"
                                    case Enums.NotificationType.SellOrderPlaced:
                                        return "qrc:/images/IC_SELL_24.png"
                                    }
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: model.message
                            color: SkinColors.secondaryText
                            font.pixelSize: 12
                            font.family: fontRegular.name
                            textFormat: Text.RichText
                            wrapMode: Text.WordWrap
                        }

                        ColumnLayout {
                            Layout.preferredWidth: 50
                            Layout.fillHeight: true
                            Layout.rightMargin: 15
                            Layout.topMargin: 10
                            Layout.bottomMargin: 10
                            spacing: 10

                            Image {

                                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                                source: "qrc:/images/ic_close.svg"
                                sourceSize: Qt.size(20, 20)

                                PointingCursorMouseArea {
                                    onClicked: removeNotification(
                                                   delegateItem.index)
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                                Layout.fillHeight: true

                                Text {
                                    anchors.right: parent.right
                                    text: model.existenceTime
                                    font.pixelSize: 10
                                    font.family: fontRegular.name
                                    color: SkinColors.secondaryText
                                }
                            }
                        }
                    }

                    PointingCursorMouseArea {
                        id: mouseArea
                        anchors.fill: parent
                    }
                }
            }
        }
    }

    Rectangle {
        Layout.fillHeight: true
        Layout.fillWidth: true
        color: SkinColors.secondaryBackground
        border.width: 1
        border.color: SkinColors.popupFieldBorder
        radius: 5

        Item {
            anchors.centerIn: parent

            Image {
                id: bellImage
                anchors.centerIn: parent
                source: "qrc:/images/bell.png"
            }

            Rectangle {
                anchors.bottom: bellImage.bottom
                height: 14
                width: 14
                radius: 7
                color: "#FF4A68"

                Text {
                    anchors.centerIn: parent
                    text: notificationsListView.count
                    font.family: fontRegular.name
                    font.pixelSize: 11
                    color: SkinColors.mainText
                }
            }
        }

        PointingCursorMouseArea {
            anchors.fill: parent
            onClicked: stackLayout.currentIndex = 0
        }
    }
}
