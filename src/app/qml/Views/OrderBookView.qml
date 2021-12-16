import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Item {
    property WalletDexViewModel walletDexViewModel
    signal selectOrder(double amount, double price, bool isBuy)

    OrderBookWaitingView {
        id: waitingView
        anchors.centerIn: parent
        visible: walletDexViewModel.stateModel.buyOrdersListModel && walletDexViewModel.stateModel.sellOrdersListModel &&
                 (walletDexViewModel.stateModel.buyOrdersListModel.loading || walletDexViewModel.stateModel.sellOrdersListModel.loading)
    }

    RowLayout {
        anchors.fill: parent
        spacing: 8
        visible: !waitingView.visible

        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true

            DexSubheadersView {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                model: ListModel {
                    ListElement { name: "Sum"; size: 0.25}
                    ListElement { name: "Total"; size: 0.25}
                    ListElement { name: "Amount"; size: 0.25}
                    ListElement { name: "Bid Price"; size: 0.25}
                }
                hasSwapPair: walletDexViewModel.hasSwapPair
            }

            OrderBookBidListView {
                Layout.fillHeight: true
                Layout.fillWidth: true
                model: walletDexViewModel.stateModel.buyOrdersListModel
                onClicked: selectOrder(amount, price, true)
            }

            OrderBookWeightView {
                id: bidWeight
                Layout.fillWidth: true
                Layout.preferredHeight: 35
                Layout.maximumHeight: 35
                firstItem.text: walletDexViewModel.hasSwapPair ? parseFloat(walletDexViewModel.stateModel.buyOrdersListModel.totals.sumTotalStr).toFixed(6) : ""
                secondItem.text: walletDexViewModel.hasSwapPair ? parseFloat(walletDexViewModel.stateModel.buyOrdersListModel.totals.sumAmountsStr).toFixed(6) : ""
                color: "#1DB182"
                firstItemUnits.text: currentQuoteAssetSymbol
                secondtemUnits.text: currentBaseAssetSymbol
            }
        }

        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true

            DexSubheadersView {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                model: ListModel {
                    ListElement { name: "Ask Price"; size: 0.25}
                    ListElement { name: "Amount"; size: 0.25}
                    ListElement { name: "Total"; size: 0.25}
                    ListElement { name: "Sum"; size: 0.25}
                }
                hasSwapPair: walletDexViewModel.hasSwapPair
            }

            OrderBookAskListView {
                Layout.fillHeight: true
                Layout.fillWidth: true
                model: walletDexViewModel.stateModel.sellOrdersListModel
                onClicked: selectOrder(amount, price, false)
            }

            OrderBookWeightView {
                id: sellWeight
                Layout.fillWidth: true
                Layout.preferredHeight: 35
                Layout.maximumHeight: 35
                firstItem.text: walletDexViewModel.hasSwapPair ? parseFloat(walletDexViewModel.stateModel.sellOrdersListModel.totals.sumAmountsStr).toFixed(6): ""
                secondItem.text: walletDexViewModel.hasSwapPair ? parseFloat(walletDexViewModel.stateModel.sellOrdersListModel.totals.sumTotalStr).toFixed(6) : ""
                firstItemUnits.text: currentBaseAssetSymbol
                secondtemUnits.text: currentQuoteAssetSymbol
                color: "#E2344F"
            }
        }
    }
}
