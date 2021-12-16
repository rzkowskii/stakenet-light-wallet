import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import Components 1.0

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

ColumnLayout {
    id: root
    signal sendCoins()
    signal receiveCoins()

    property string mainHeader: ""
    property bool buttonsVisible: false
    property string accountBalance: ""
    property string currentCurrencySymbol: ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol
    property string activeAssetsCount: ""

    spacing: 23

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    WalletAssetsListModel {
        id: assetModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
            root.activeAssetsCount = activeAssetsCount
        }
        onActiveAssetsCountChanged: root.activeAssetsCount = activeAssetsCount
    }

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        height: 249
        width: height
        color: "transparent"

        border.width: 3.5
        border.color: SkinColors.menuBackgroundHoveredGradienRightLine
        radius: width / 2

        Column {
            anchors.centerIn: parent
            spacing: 16

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 7

                XSNLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Est. Assets Value"
                    font.pixelSize: 12
                    font.family: regularFont.name
                }

                XSNLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "%1 %2" .arg(currentCurrencySymbol) .arg(accountBalance)
                    font.pixelSize: 32
                    font.weight: Font.Light
                }
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 113
                height: 25
                radius: 12
                border.color: SkinColors.menuBackgroundGradientFirst
                border.width: 2
                color: "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "%1 Active %2" .arg(root.activeAssetsCount) .arg(root.activeAssetsCount > 1 ? "Wallets" : "Wallet")
                    font.family: regularFont.name
                    font.pixelSize: 12
                    color: SkinColors.mainText
                }
            }

        }
    }

    Item {
        Layout.fillWidth: true
        Layout.maximumHeight: 310
        Layout.fillHeight: true

        Flickable {
            anchors.fill: parent
            contentHeight: 305
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                anchors.fill: parent
                spacing: 7

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 7

                    PortfolioInfoPlateItem {
                        plateHeader: "Portfolio Change"
                        plateMainText: "%1  %2" .arg(currentCurrencySymbol) .arg("23.560,00")
                        platePercentageNumber: "4.7"
                    }

                    PortfolioInfoPlateItem {
                        plateHeader: "24H Profit/Loss"
                        plateMainText: "%1 %2" .arg(currentCurrencySymbol) .arg("23.560,00")
                        platePercentageNumber: "2.7"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 7

                    PortfolioInfoPlateItem {
                        plateHeader: "Profit/Loss"
                        plateMainText: "%1 %2" .arg(currentCurrencySymbol) .arg("17.560,00")
                        platePercentageNumber: "7.2"
                    }

                    PortfolioInfoPlateItem {
                        plateHeader: "Portfolio Age"
                        plateMainText: ApplicationViewModel.walletViewModel.walletAge()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 7

                    PortfolioInfoPlateItem {
                        plateHeader: "Best Assets"
                        plateMainText: "%1 %2" .arg(currentCurrencySymbol) .arg("17.560,00")
                        platePercentageNumber: "3.7"
                    }

                    PortfolioInfoPlateItem {
                        plateHeader: "Worst Assets"
                        plateMainText: "%1 %2" .arg(currentCurrencySymbol) .arg("6.980,00")
                        platePercentageNumber: "5.2"
                    }
                }
            }
        }

        LinearGradient {
            anchors.bottom: parent.bottom
            height: 15
            width: parent.width
            start: Qt.point(0, 0)
            end: Qt.point(0, height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: SkinColors.secondaryBackground}
            }
        }

        LinearGradient {
            anchors.top: parent.top
            height: 15
            width: parent.width
            start: Qt.point(0, 0)
            end: Qt.point(0, height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: SkinColors.secondaryBackground }
                GradientStop { position: 1.0; color: "transparent"}
            }
        }
    }
}
