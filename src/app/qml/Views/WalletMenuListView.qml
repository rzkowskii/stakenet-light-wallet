import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5
import Qt.labs.settings 1.0

import "../Components"

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0

Rectangle {
    id: root
    height: parent.height
    width: menuWidthSmallMode
    color: SkinColors.menuBackground
    opacity: 0.81

    property double walletBalance: 0
    property alias currentIndex: settings.pageIndex
    property bool balanceVisible
    signal balanceVisibileChanged
    state: "opened"

    property LockingViewModel lockingViewModel: ApplicationViewModel.lockingViewModel

    states: [
        State {
            name: "opened"
            PropertyChanges {
                target: menuColumn
                visible: true
            }
            PropertyChanges {
                target: root
                width: menuWidthSmallMode
            }
            PropertyChanges {
                target: closeButton
                visible: false
            }
        },
        State {
            name: "closed"
            PropertyChanges {
                target: menuColumn
                visible: false
            }
            PropertyChanges {
                target: root
                width: 10
            }
            PropertyChanges {
                target: closeButton
                visible: true
            }
        }
    ]

    function close() {
        menuColumn.visible = false
        root.width = 10
        closeButton.visible = true
    }

    function open() {
        root.width = menuWidthSmallMode
        menuColumn.visible = true
        closeButton.visible = false
    }

    Settings {
        id: settings
        category: "Wallet pages"
        property int pageIndex: 4
    }

    ColumnLayout {
        id: menuColumn
        anchors {
            fill: parent
            leftMargin: 7
            bottomMargin: 20
        }
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 80

            Image {
                anchors.centerIn: parent
                source: SkinColors.walletSideMenuLogo
                sourceSize: Qt.size(70, 60)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            opacity: 0.3
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: "transparent"
                }
                GradientStop {
                    position: 1.0
                    color: SkinColors.mainText
                }
            }
        }

        ColumnLayout {
            Layout.topMargin: 32
            Layout.leftMargin: 21
            Layout.fillWidth: true

            XSNLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTr("My Balance")
                color: SkinColors.mainText
                font.pixelSize: 12
            }

            Text {
                id: balanceText
                Layout.fillWidth: true
                text: "%1 %2".arg(
                          ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                          balanceVisible ? walletBalance.toFixed(
                                               2) : hideBalance(
                                               walletBalance.toFixed(2)))
                horizontalAlignment: Text.AlignLeft
                fontSizeMode: Text.HorizontalFit
                color: SkinColors.mainText
                FontLoader {
                    id: localFont
                    source: "qrc:/Rubik-Light.ttf"
                }
                font.family: localFont.name
                minimumPixelSize: 9
                font.pixelSize: 20

                PointingCursorMouseArea {
                    id: balanceMouseArea
                    anchors.fill: parent
                    onClicked: balanceVisibileChanged()
                }

                CustomToolTip {
                    height: 40
                    width: 70
                    x: balanceText.x
                    visible: balanceMouseArea.containsMouse
                    tailPosition: 35
                    text: root.balanceVisible ? "Hide" : "Show"
                }
            }
        }

        Item {
            Layout.preferredHeight: 20
            Layout.fillWidth: true
        }

        Repeater {
            id: menuListView
            Layout.fillWidth: true
            Layout.preferredHeight: count * menuItemHeightSmallMode
            model: [
                /*{
                    "name": "Dashboard",
                    "activeImage": "qrc:/images/dashboard_active.svg",
                    "inactiveImage": "qrc:/images/dashboard_inactive.svg"
                },*/ {
                    "name": "Portfolio",
                    "activeImage": "qrc:/images/portfolio_active.svg",
                    "inactiveImage": "qrc:/images/portfolio_inactive.svg"
                }, {
                    "name": "Wallets",
                    "activeImage": "qrc:/images/wallet_active.svg",
                    "inactiveImage": "qrc:/images/wallet_inactive.svg"
                }, {
                    "name": "DEX",
                    "activeImage": "qrc:/images/ic_dex_active.svg",
                    "inactiveImage": "qrc:/images/ic_dex_inactive.svg"
                }

                /*{
                    "name": "Swap",
                    "activeImage": "qrc:/images/lightning_active.svg",
                    "inactiveImage": "qrc:/images/lightning_inactive.svg"
                }, {
                    "name": "XSN Cloud",
                    "activeImage": "qrc:/images/xsncloud_active.svg",
                    "inactiveImage": "qrc:/images/xsncloud_inactive.svg"
                }, {
                    "name": "Vortex",
                    "activeImage": "qrc:/images/BOT_Active.svg",
                    "inactiveImage": "qrc:/images/BOT_Inactive.svg"
                }*/]

            delegate: MainMenuItem {
                Layout.fillWidth: true
                Layout.preferredHeight: menuItemHeightSmallMode
                isCurrentItem: settings.pageIndex === index
                name: modelData.name
                imageSource: isCurrentItem
                             || itemMouseArea.containsMouse ? modelData.activeImage : modelData.inactiveImage
                onMenuItemClicked: {
                    settings.pageIndex = index
                }
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        MainMenuItem {
            id: updateBtn
            Layout.fillWidth: true
            Layout.preferredHeight: menuItemHeightSmallMode
            name: "Update"
            imageSource: "qrc:/images/update.svg"
            visible: AppUpdater.updaterState === AppUpdater.UpdaterState.Ready
            onMenuItemClicked: {
                var popup = openUpdateNotificationDialog({
                                                             "confirmButtonText": "Update"
                                                         })
                popup.accepted.connect(function () {
                    AppUpdater.startUpdating()
                })
            }
        }

        MainMenuItem {
            property int index: 3
            Layout.fillWidth: true
            Layout.preferredHeight: menuItemHeightSmallMode
            Layout.alignment: Qt.AlignBottom
            isCurrentItem: settings.pageIndex === index
            name: "Settings"
            imageSource: isCurrentItem
                         || itemMouseArea.containsMouse ? "qrc:/images/settings_active.svg" : "qrc:/images/settings_inactive.svg"
            onMenuItemClicked: {
                settings.pageIndex = index
            }
        }

        Item {
            Layout.preferredHeight: 38
            Layout.fillWidth: true
//            Layout.leftMargin: 10
            visible: lockingViewModel && lockingViewModel.isEncrypted


            RowLayout {
                anchors {
                    fill: parent
                    leftMargin: 24
                }
                spacing: 18

                Image {
                    source: "qrc:/images/iclock.svg"
                    sourceSize: Qt.size(15, 15)
                }

                XSNLabel {
                    FontLoader {
                        id: fontRegular
                        source: "qrc:/Rubik-Regular.ttf"
                    }
                    Layout.fillWidth: true
                    Layout.alignment: Text.AlignLeft
                    text: "Lock wallet"
                    font.pixelSize: 14
                    color: mouseArea.containsMouse ? SkinColors.mainText : SkinColors.menuItemText
                    font.family: fontRegular.name
                }
            }



//            XSNLabel {
//                FontLoader {
//                    id: fontRegular
//                    source: "qrc:/Rubik-Regular.ttf"
//                }
//                anchors.centerIn: parent
//                text: "Lock wallet"
//                font.pixelSize: 14
//                color: mouseArea.containsMouse ? SkinColors.mainText : SkinColors.menuItemText
//                font.family: fontRegular.name
//            }

            PointingCursorMouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: lockingViewModel.lock()
            }
        }

        Item {
            Layout.preferredHeight: 30
            Layout.fillWidth: true
        }

        MainMenuItem {
            Layout.fillWidth: true
            Layout.preferredHeight: menuItemHeightSmallMode
            Layout.alignment: Qt.AlignBottom
            name: "Hide Menu"
            imageSource: "qrc:/images/ic_up.svg"
            onMenuItemClicked: root.state = "closed"
        }
    }

    RoundedRectangle {
        id: closeButton
        anchors {
            left: parent.right
            bottom: parent.bottom
            bottomMargin: 20
        }
        corners.bottomRightRadius: 10
        corners.topRightRadius: 10

        height: 30
        width: 30
        color: SkinColors.menuBackground
        visible: false

        Image {
            anchors.centerIn: parent
            sourceSize: Qt.size(10, 10)
            source: "qrc:/images/ic_rightarrow1x.svg"
        }

        PointingCursorMouseArea {
            onClicked: root.state = "opened"
        }
    }
}
