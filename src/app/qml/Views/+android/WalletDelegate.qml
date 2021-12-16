import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0
import Views 1.0

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0


Rectangle {
    id: root
    height: 96
    color: SkinColors.secondaryBackground
    radius: 8

    property string assetColor: ""
    property int assetID: -1
    property string name: ""
    property string symbol: ""
    property var balance: ""
    property var transactionListModel: undefined
    property bool balanceVisible

    signal assetClicked()

    Component {
        id: mobileWalletViewComponent
        MobileWalletView {

        }
    }

    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }
    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 10
        anchors.bottomMargin: 15
        anchors.leftMargin: 15
        anchors.rightMargin: 15

        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 33

            RowLayout {
                spacing: 10

                Image {
                    Layout.alignment: Qt.AlignVCenter
                    source: "qrc:/images/ICON_%1.svg" .arg(name)
                    sourceSize: Qt.size(30, 35)
                }

                Row {
                    Layout.fillWidth: true

                    Text {
                        id: assetsName
                        text: name
                        font.family: mediumFont.name
                        font.pixelSize: 14
                        color: SkinColors.mainText
                    }

                    Text {
                        text: qsTr(" - " + symbol)
                        font.family: mediumFont.name
                        font.pixelSize: 14
                        font.capitalization: Font.AllUppercase
                        color: SkinColors.mainText
                    }
                }
            }

            ColumnLayout {
                //visible: false //temporary
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                Row {
                    spacing: 5

                    Text {
                        font.pixelSize: 12
                        font.family: regularFont.name
                        text: "%1 %2".arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol) .arg(ApplicationViewModel.localCurrencyViewModel.convert(assetID, 1))
                        color: SkinColors.menuItemText
                    }

                    Text {
                        font.pixelSize: 12
                        font.family: regularFont.name
                        text: ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode
                        font.capitalization: Font.AllUppercase
                        color: SkinColors.menuItemText
                    }
                }

                Row {
                    spacing: 0
                    Layout.alignment: Qt.AlignRight

                    Text {
                        font.pixelSize: 12
                        font.family: regularFont.name
                        text: "+"
                        color: SkinColors.mobileWalletReceive
                    }

                    Text {
                        Layout.alignment: Qt.AlignRight
                        font.pixelSize: 12
                        font.family: regularFont.name
                        text: "%1%2".arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol) .arg("120")
                        color: SkinColors.mobileWalletReceive
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 0

                Row {
                    spacing: 5

                    SecondaryLabel {
                        font.family: regularFont.name
                        text: Utils.formatBalance(balance)

                    }

                    SecondaryLabel {
                        text: symbol
                        font.family: regularFont.name
                        font.capitalization: Font.AllUppercase
                    }
                }

                SecondaryLabel {
                    font.family: mediumFont.name
                    font.pixelSize: 12
                    text: "= %1 %2" .arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(ApplicationViewModel.localCurrencyViewModel.convert(assetID, Utils.formatBalance(balance)))
                }
            }
        }
    }

    PointingCursorMouseArea {
        onClicked:
        {
            assetClicked();
            navigateToItem(mobileWalletViewComponent, {assetID : assetID,
                               assetColor : assetColor,
                               name : name,
                               symbol : symbol,
                               balance : balance,
                               transactionListModel : root.transactionListModel})
        }
    }
}

