import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ColumnLayout {
    id: root
    property double accountBalance: 0
    property bool isApproximatelyEqual: false
    signal sendCoins(int id)
    signal receiveCoins(int id)
    signal balanceVisibilityChanged
    property alias searchWallet: searchArea

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    onBalanceVisibilityChanged: {
        walletsListModel.balanceVisibilityChanged()
    }

    Connections {
        target: ApplicationViewModel.localCurrencyViewModel
        function onCurrencyRateChanged(assetID) {
            calculateAccountBalance();
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.maximumHeight: 45
        visible: !isMobile

        XSNLabel {
            Layout.preferredWidth: 420
            text: "Wallets"
        }

        FadedRectangle {
            id: fadeBackground
            Layout.fillWidth: true
            Layout.maximumHeight: 45
            Layout.preferredHeight: 45
            radius: 2
            activeStateColor: SkinColors.secondaryBackground
            inactiveStateColor: "transparent"

            RowLayout {
                anchors.fill: parent
                spacing: 5

                ColorOverlayImage {
                    Layout.preferredWidth: 45
                    imageSize: 35
                    width: imageSize
                    height: imageSize
                    imageSource: "qrc:/images/magnifyingGlass.png"
                    color: SkinColors.magnifyingGlass
                }

                SearchTextField {
                    id: searchArea
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    placeholderText: "Search wallet"
                    validator: RegExpValidator {
                        regExp: /[a-zA-Z]{1,10}\D/g
                    }
                    onComponentActiveFocusChanged: {
                        if(componentActiveFocus)
                        {
                            fadeBackground.startFade()
                        }
                        else
                        {
                            fadeBackground.stopFade();
                        }
                    }
                }
            }
        }

        Row {
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignRight
            spacing: 10
            visible: false

            SecondaryLabel {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Sort by")
            }

            CustomizedComboBox {
                id: comboBox
                anchors.verticalCenter: parent.verticalCenter
                model: ["Currency", "Balance"]
            }
        }
    }

    ColumnLayout {
        Layout.fillHeight: true
        Layout.fillWidth: true

        WalletsListHeaderView {
            id: walletListHeaderView
            Layout.fillWidth: true
            Layout.preferredHeight: 40
        }

        WalletsListView {
            id: walletsListModel
            Layout.fillHeight: true
            Layout.fillWidth: true
            balanceVisible: mainPage.balanceVisible
            onSendCoinsRequested: {
                root.sendCoins(id)
            }
            onReceiveCoinsRequested: {
                root.receiveCoins(id)
            }

            QMLSortFilterListProxyModel {
                id: assetListProxyModel
                source:  QMLSortFilterListProxyModel {
                    source: WalletAssetsListModel {
                        id: walletAssetsListModel
                        Component.onCompleted: initialize(ApplicationViewModel)
                        onAccountBalanceChanged:
                        {
                            if(accountBalance !== undefined)
                            {
                                calculateAccountBalance();
                            }
                        }
                    }
                    filterRole: "isActive"
                    filterString: "1"
                    filterCaseSensitivity: Qt.CaseInsensitive
                }
                filterRole: "nameSymbol"
                filterString: searchArea.text
                sortRole: walletListHeaderView.currentSortRole
                filterCaseSensitivity: Qt.CaseInsensitive
                sortCaseSensitivity: Qt.CaseInsensitive
                sortAsc: walletListHeaderView.orderAsc
            }
            model: assetListProxyModel
        }
    }

    function calculateAccountBalance()
    {
        var result = 0.0;
        var isApproxEqualArray = [] //needed to consider whether coins local balances is approximaty equal
        for(var i = 0 ; i < assetListProxyModel.count; i++)
        {
            // localBalance is value which can be number or value "~0.01"
            var localBalance = parseFloat(ApplicationViewModel.localCurrencyViewModel.convert(assetListProxyModel.get(i).id,
                                                                                              Utils.formatBalance(assetListProxyModel.get(i).balance)))
            if(!isNaN(localBalance)) // if local balance is number and exact value
            {
                result += localBalance
                isApproxEqualArray.push(false);
            }
            else // if local balance is approximately equal to 0.01
            {
                result += 0.01
                isApproxEqualArray.push(true);
            }
        }

        root.accountBalance = result;
        isApproximatelyEqual = isApproxEqualArray.some(element => element)// if at least one value is approximate then the account balance value will also be approximaty equal
    }
}
