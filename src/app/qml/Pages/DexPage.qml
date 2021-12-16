import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0
import QtGraphicalEffects 1.15

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

Page {
    id: root

    background: Rectangle {
        anchors.fill: parent
        color: walletDexViewModel.inMaintenance ? SkinColors.mainBackground : "transparent"
    }

    FontLoader { id: fontRegular; source: "qrc:/Rubik-Regular.ttf" }
    FontLoader { id: fontMedium; source: "qrc:/Rubik-Medium.ttf" }

    property string currentBaseAssetSymbol: walletDexViewModel.hasSwapPair ? assetModel.get(assetModel.getInitial(walletDexViewModel.baseAssetID)).symbol : ""
    property string currentQuoteAssetSymbol: walletDexViewModel.hasSwapPair ? assetModel.get(assetModel.getInitial(walletDexViewModel.quoteAssetID)).symbol : ""

    property string currentBaseAssetName: walletDexViewModel.hasSwapPair ? assetModel.get(assetModel.getInitial(walletDexViewModel.baseAssetID)).name : ""
    property string currentQuoteAssetName: walletDexViewModel.hasSwapPair ? assetModel.get(assetModel.getInitial(walletDexViewModel.quoteAssetID)).name : ""

    property bool closeDialogOpened: false
    property bool canClose: false

    property bool isLargeScreenLayout: mainWindow.width > 1600

    property alias swapInProcess: swapViewModel.marketSwapInProcess

    enabled: !swapInProcess

    property WalletDexViewModel walletDexViewModel

    FastBlur {
        anchors.fill: parent
        z: 1000
        source: dexStackLayout
        radius: 32
        visible: swapInProcess
    }

    Connections {
        target: walletDexViewModel
        function onSwapSuccess(amount, receiveSymbol) {
            showBubblePopup("Swap successful for %1 %2".arg(
                                Utils.formatBalance(amount)).arg(receiveSymbol))
        }

        function onSwapFailed(errorText) {
            showBubblePopup("Swap failed: %1".arg(errorText))
        }
    }

    Connections {
        target: mainWindow
        function onClosing() {
            if (walletDexViewModel.stateModel.ownOrderBookListModel
                    && walletDexViewModel.stateModel.ownOrderBookListModel.rowCount(
                        ) > 0) {
                if (canClose) {
                    close.accepted = true
                    return
                }

                if (!closeDialogOpened) {
                    openCloseNotificationDialog({
                                                    "message": "Are you sure you want to close the wallet?\n All active orders will be removed from the DEX."
                                                })
                }
                close.accepted = false
            }
        }
        function onHeightChanged() {
            chart.Layout.preferredHeight = Qt.binding(function () {
                return mainWindow.height > 840 ? mainWindow.height * 0.5 : 420
            })
        }
    }

    Component {
        id: closeNotification

        CloseNotificationPopup {
            onClosed: {
                closeDialogOpened = false
                canClose = false
            }

            onConfirmClosing: {
                canClose = true
                mainWindow.close()
            }
        }
    }

    WalletAssetsListModel {
        id: assetModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    WalletMarketSwapViewModel {
        id: swapViewModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    StackLayout {
        id: dexStackLayout
        anchors.fill: parent
        currentIndex: walletDexViewModel.inMaintenance ? 0 : 1

        Item {
            Layout.preferredHeight: 300
            Layout.preferredWidth: 300

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 35

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: "qrc:/images/maintenance.svg"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 30

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Under Maintenance"
                        font.family: fontRegular.name
                        font.pixelSize: 29
                        color: SkinColors.mainText
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Orderbook is currently in maintenance,<br>please check back soon."
                        font.family: fontRegular.name
                        font.pixelSize: 16
                        color: SkinColors.mainText
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        ColumnLayout {
            spacing: 0

            DexTabHeaderView {
               Layout.preferredHeight: 80
               Layout.fillWidth: true
               visible: false
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true

                Rectangle {
                    anchors.fill: parent
                    color: SkinColors.dexPageBackground
                }

                RowLayout {
                    id: layout
                    anchors.fill: parent
                    anchors.margins: 30
                    spacing: 0

                    MainDexView {
                        id: mainDexView
                        Layout.preferredWidth: 250
                        Layout.fillHeight: true
                        walletDexViewModel: root.walletDexViewModel
                    }

                    ColumnLayout {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        spacing: 10
                        visible: walletDexViewModel.hasSwapPair

                        DexChartView {
                            id: chart
                            Layout.fillWidth: true
                            Layout.preferredHeight: mainWindow.height > 840 ? mainWindow.height * 0.5 : 400
                        }

                        TabHeaderListView {
                            id: dexHeaderView
                            Layout.preferredHeight: 42
                            Layout.fillWidth: true

                            readonly property var largeScreenHeaders:["Order book", "Open orders", "My trade history"]
                            readonly property var smallScreenHeaders:["Order book", "Open orders", "My trade history", "Trade history"]

                            model: isLargeScreenLayout ? largeScreenHeaders : smallScreenHeaders
                            currentIndex: 0
                            delegateWidth: 180
                        }

                        StackLayout {
                            id: stackLayout
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            currentIndex: dexHeaderView.currentIndex

                            OrderBookView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                walletDexViewModel: root.walletDexViewModel
                                onSelectOrder: setOpenOrdersData(amount, price, isBuy)
                            }

                            OpenOrdersView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                walletDexViewModel: root.walletDexViewModel
                            }

                            OrdersHistoryView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                walletDexViewModel: root.walletDexViewModel
                            }

                            TradeHistoryView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                walletDexViewModel: root.walletDexViewModel
                                onSelectTrade: setOpenOrdersData(amount, price, isBuy)
                            }
                        }
                    }

                    Rectangle {
                        color: SkinColors.assetsMenuBackground
                        Layout.maximumWidth: 400
                        Layout.preferredWidth: 400
                        Layout.fillHeight: true
                        visible: isLargeScreenLayout && walletDexViewModel.hasSwapPair

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20
                            anchors.rightMargin: 20
                            anchors.topMargin: 20
                            spacing: 20

                            Text {
                                Layout.leftMargin: 10
                                text: "Trade history"
                                font.pixelSize: 14
                                font.family: fontMedium.name
                                font.weight: Font.Medium
                                font.capitalization: Font.AllUppercase
                                color: SkinColors.mainText
                            }

                            TradeHistoryView {
                                id: tradeHistory
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                walletDexViewModel: root.walletDexViewModel
                                onSelectTrade: setOpenOrdersData(amount, price, isBuy)
                            }
                        }
                    }
                }
            }
        }
    }

    Settings {
        id: settings
        category: "Dex"
    }

    function openCloseNotificationDialog(params) {
        closeDialogOpened = true
        return openDialog(closeNotification, params)
    }

    function setOpenOrdersData(amount, price, isBuy) {
        var selectTypeOption = settings.value("selectTypeOptionName")
        switch (selectTypeOption) {
        case "Predictive":
            if (mainDexView.isBuy === isBuy) {
                if (isBuy) {
                    mainDexView.buySellViewIndex = 1
                } else {
                    mainDexView.buySellViewIndex = 0
                }
            }
            break
        case "Matching":
            if (mainDexView.isBuy !== isBuy) {
                if (isBuy) {
                    mainDexView.buySellViewIndex = 0
                } else {
                    mainDexView.buySellViewIndex = 1
                }
            }
            break
        case "Default":
            break
        }

        var newAmount = Utils.formatBalance(amount)
        var newPrice = Utils.formatBalance(price)

        if (!mainDexView.isLimit && mainDexView.isBuy) {
            var newTotal = Utils.formatBalance(Utils.parseCoinsToSatoshi(
                                                   newAmount) * parseFloat(
                                                   newPrice), 8)
            mainDexView.totalFocus = true
            mainDexView.totalText = newTotal
            mainDexView.totalText = newTotal // ugly workaround but we need this to break invalid
            return
        }

        mainDexView.amountFocus = true
        mainDexView.amountText = newAmount
        mainDexView.amountText = newAmount // ugly workaround but we need this to break invalid

        if (mainDexView.isLimit) {
            mainDexView.priceText = newPrice
            mainDexView.priceText = newPrice // ugly workaround but we need this to break invalid
        }
    }
}
