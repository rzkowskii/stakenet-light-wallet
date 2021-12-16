import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0

import "../Components"
import "../Views"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

Page {
    id: mainPage
    property string currentAssetID: walletPage.currentAssetID
    property bool isOpenRescanNotificationPopup: false
    property bool acceptedLiabilityTerms: false
    property bool balanceVisible: true
    property bool simpleSwapInProcess: false // swapPage.swapInProcess
    property alias copyMessage: copyMessagePopup

    Settings {
        id: settings
        category: "Wallet"
    }

    background: Rectangle {
        color: "transparent"
    }

    Component.onCompleted: {
        if (isOpenRescanNotificationPopup) {
            openResetNotificationDialog()
        }

        var ignoreTerms = settings.value("ignoreDisclaimer")
        if (!ignoreTerms) {
            openDisclaimer()
        } else {
            acceptedLiabilityTerms = true
        }
    }

    Timer {
        repeat: true
        interval: 1000
        onTriggered: {
            AppUpdater.checkForUpdates()
            interval = 60 * 1000
        }

        Component.onCompleted: {
            start()
        }
    }

    Component {
        id: sendDialogComponent
        SendPopup {}
    }

    Component {
        id: receiveDialogComponent
        ReceivePopup {}
    }

    Component {
        id: transactionDetailsComponent
        TransactionsDetailsPopup {}
    }

    Component {
        id: lnInvoiceDetailsPopup
        LightningInvoiceDetailsPopup {}
    }

    Component {
        id: lnPaymentDetailsPopup
        LightningPaymentDetailsPopup {}
    }

    Component {
        id: connextPaymentDetailsPopup
        ConnextPaymentDetailsPopup {}
    }

    Component {
        id: ethTransactionDetailsComponent
        EthTransactionsDetailsPopup {}
    }

    Component {
        id: transactionAddresesComponent
        TransactionAddresesPopup {}
    }

    Component {
        id: channelDialogComponent
        OpenChannelPopup {}
    }

    Component {
        id: mobileLocalizationsPopup
        MobileLocalizationPopup {}
    }

    Component {
        id: dexNotificationComponent
        DexNotificationPopup {}
    }

    Component {
        id: dexOrderPlacedComponent
        DexOrderPlacedPopup {}
    }

    Component {
        id: notificationComponent
        NotificationPopup {}
    }

    Component {
        id: dexPlaceOrderNotification
        DexOrderPlacedNotificationPopup {}
    }

    Component {
        id: resetNotification

        ConfirmationPopup {
            width: 480
            height: 280
            message: "Rescan of active blockchains is in progress. Please wait for this to finish as it is rescanning the respective blockchains for any coins held within your wallet."
            cancelButton.visible: false
            confirmButton.text: "Ok"
            onConfirmClicked: {
                close()
            }
        }
    }

    Component {
        id: confirmResetNotification

        ConfirmationPopup {
            width: 540
            height: 270
            message: "Please make sure you have written down your seed. You will need it when you restore your wallet.\n\nAre you sure you want to reset the wallet?"
            cancelButton.text: "Cancel"
            confirmButton.text: "Reset"
            onCancelClicked: {
                close()
            }
            onConfirmClicked: {
                ApplicationViewModel.walletViewModel.resetState(
                            WalletViewModel.LoadingState.Done)
                close()
            }
        }
    }

    Component {
        id: confirmRemoveOrder

        ConfirmationPopup {
            width: 480
            height: 230
            cancelButton.text: "No"
            onCancelClicked: {
                close()
            }
            confirmButton.text: "Yes"
        }
    }

    FastBlur {
        anchors.fill: parent
        anchors.leftMargin: menu.width
        anchors.topMargin: errorView.visible ? errorView.height : 0
        z: 1000
        source: menuStackLayout
        radius: 32
        visible: activePopup !== undefined && activePopup.visible
    }

    Component {
        id: rentalChannelComponent
        RentChannelPopup {}
    }

    Component {
        id: dexDisclaimerComponent
        DexDisclaimerPopup {
            onAccepted: acceptedLiabilityTerms = true
            onRejected: mainWindow.close()
        }
    }

    Component {
        id: openChannelDialogComponent
        OpenChannelPopup {}
    }

    Connections {
        target: MouseEventSpy
        function onMouseEventDetected() {
            if (activePopup === undefined) {
                menuStackLayout.forceActiveFocus()
            }
        }
    }

    Component {
        id: unlockWalletComponent
        UnlockWalletPopup {}
    }

    Connections {
        target: AppUpdater
        function onUpdaterStateChanged() {
            if (AppUpdater.updaterState === AppUpdater.UpdaterState.ExistNew) {
                var popup = openUpdateNotificationDialog({
                                                             "confirmButtonText": "Download"
                                                         })
                popup.accepted.connect(function () {
                    AppUpdater.downloadUpdate()
                })
            }
        }
    }

    Component {
        id: updateNotificationComponent
        UpdateNotificationPopup {}
    }

    WalletDexViewModel {
        id: dexViewModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        WalletErrorView {
            id: errorView
            Layout.fillWidth: true
            Layout.preferredHeight: isMobile ? 90 : 50
            clip: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            Item {
                id: mainContent
                Layout.fillWidth: true
                Layout.fillHeight: true

                WalletMenuListView {
                    id: menu
                    z: 3000
                    walletBalance: portfolioPage.accountBalance
                    balanceVisible: mainPage.balanceVisible
                    onBalanceVisibileChanged: {
                        mainPage.balanceVisible = !mainPage.balanceVisible
                        portfolioPage.balanceVisibileChanged()
                    }
                }

                Item {
                    anchors.top: parent.top
                    anchors.bottom: isMobile ? menu.top : parent.bottom
                    anchors.left: isMobile ? parent.left : menu.right
                    anchors.right: parent.right

                    CopyPopup {
                        id: copyMessagePopup

                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: isMobile ? 10 : 20
                    }

                    StackLayout {
                        id: menuStackLayout
                        anchors.fill: parent
                        currentIndex: menu.currentIndex
                        onCurrentIndexChanged: {
                            // MouseEventSpy and its setEventFilerEnabled need for changing active focus on pages.
                            // When event filter is enabled then after clicking on an empty page space MouseEventSpy captures
                            // this event and forces focus on the current page.
                            // (For example when we on search field and click on empty space -> focus forces on current page)
                            switch (currentIndex) {
                            case 1:
                                portfolioPage.rootLayout.forceActiveFocus()
                                MouseEventSpy.setEventFilerEnabled(true)
                                break
                            case 2:
                                walletPage.rootLayout.forceActiveFocus()
                                MouseEventSpy.setEventFilerEnabled(true)
                                break
                            case 3:
                                dexPage.forceActiveFocus()
                                MouseEventSpy.setEventFilerEnabled(false)
                                break
                            case 4:
                                settingsPage.rootLayout.forceActiveFocus()
                                MouseEventSpy.setEventFilerEnabled(true)
                                break
//                            case 3:
//                                swapPage.rootItem.forceActiveFocus()
//                                MouseEventSpy.setEventFilerEnabled(true)
//                                break
//                            case 5:
//                                cloudPage.forceActiveFocus()
//                                MouseEventSpy.setEventFilerEnabled(true)
//                                break
//                            case 4:
//                                botPage.forceActiveFocus()
//                                MouseEventSpy.setEventFilerEnabled(true)
//                                break
                            }
                        }

                        PortfolioPage {
                            id: portfolioPage
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }

                        WalletPage {
                            id: walletPage
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }

//                        SwapPage {
//                            id: swapPage
//                            Layout.fillHeight: true
//                            Layout.fillWidth: true
//                            walletDexViewModel: dexViewModel
//                        }

                        DexPage {
                            id: dexPage
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            walletDexViewModel: dexViewModel
                        }

//                        Item {
//                            id: cloudPage
//                            Layout.fillHeight: true
//                            Layout.fillWidth: true
//                            Layout.alignment: Qt.AlignCenter

//                            XSNLabel {
//                                anchors.centerIn: parent
//                                text: "Coming soon"
//                                color: SkinColors.mainText
//                                font.pixelSize: 20
//                            }
//                        }

//                        BotPage {
//                            id: botPage
//                            Layout.fillHeight: true
//                            Layout.fillWidth: true
//                            walletDexViewModel: dexViewModel
//                        }

                        SettingsPage {
                            id: settingsPage
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }
                    }
                }

                NotificationsView {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: showNotifications ? 0 : 50
                    anchors.rightMargin: showNotifications ? 0 : 15
                    width: showNotifications ? 300 : 40
                    height: showNotifications ? parent.height : 40
                    visible: false
                }
            }

            EmulatorPage {
                visible: ApplicationViewModel.emulated
                Layout.fillHeight: true
                Layout.minimumWidth: parent.width / 6
                Layout.maximumWidth: parent.width / 6
                modelName: currentAssetID
            }
        }
    }

    ConnextBrowserNode {}

    function hideBalance(balanceStr) {
        var lenght = balanceStr.length
        var horizontalLine = '-'
        return horizontalLine.repeat(lenght)
    }

    function checkPaymentNodeState(paymentNodeStateModel) {
        if (paymentNodeStateModel.nodeType === Enums.PaymentNodeType.Lnd) {
            return paymentNodeStateModel.nodeStatus === LnDaemonStateModel.LndStatus.LndActive
        } else {
            return paymentNodeStateModel.nodeStatus
                    === ConnextStateModel.ConnextStatus.ConnextActive
        }
    }

    function isPaymentNodeSyncing(paymentNodeStateModel) {
        if (paymentNodeStateModel.nodeType === Enums.PaymentNodeType.Lnd) {
            return paymentNodeStateModel.nodeStatus === LnDaemonStateModel.LndStatus.LndChainSyncing
                    || paymentNodeStateModel.nodeStatus
                    === LnDaemonStateModel.LndStatus.LndGraphSyncing
                    || paymentNodeStateModel.nodeStatus
                    === LnDaemonStateModel.LndStatus.WaitingForPeers
        } else {
            return paymentNodeStateModel.nodeStatus
                    === ConnextStateModel.ConnextStatus.ConnextSyncing
        }
    }

    function isPaymentNodeNotRunning(paymentNodeStateModel) {
        if (paymentNodeStateModel.nodeType === Enums.PaymentNodeType.Lnd) {
            return paymentNodeStateModel.nodeStatus === LnDaemonStateModel.LndStatus.LndNotRunning
        } else {
            return paymentNodeStateModel.nodeStatus
                    === ConnextStateModel.ConnextStatus.ConnextNotRunnig
        }
    }

    function calculateNetworkFeeRate(option, recommendedNetworkFee) {
        switch (option) {
        case "Low":
            return Math.ceil(recommendedNetworkFee * 0.6)
        case "Medium":
            return recommendedNetworkFee
        case "High":
            return Math.ceil(recommendedNetworkFee * 1.4)
        default:
            return 0
        }
    }

    function getGasType(option) {
        switch (option) {
        case "Low":
            return Enums.GasType.Slow
        case "Medium":
            return Enums.GasType.Average
        case "High":
            return Enums.GasType.Fast
        default:
            return Enums.GasType.Average
        }
    }

    function openSendDialog(id) {
        openDialog(sendDialogComponent, {
                       "assetID": id
                   })
    }

    function openReceiveDialog(id) {
        openDialog(receiveDialogComponent, {
                       "assetID": id
                   })
    }

    function openTransactionDetailsDialog(params) {
        return openDialog(transactionDetailsComponent, params)
    }

    function openLnInvoiceDetailsDialog(params) {
        return openDialog(lnInvoiceDetailsPopup, params)
    }

    function openLnPaymentDetailsDialog(params) {
        return openDialog(lnPaymentDetailsPopup, params)
    }

    function openEthTransactionDetailsDialog(params) {
        return openDialog(ethTransactionDetailsComponent, params)
    }

    function openConnextPaymentDetailsDialog(params) {
        return openDialog(connextPaymentDetailsPopup, params)
    }

    function openTransactionAddressesDialog(params) {
        openDialog(transactionAddresesComponent, params)
    }

    function openMobileLocalizationDialog(params) {
        openDialog(mobileLocalizationsPopup, params)
    }

    function openConfirmResetNotificationDialog() {
        return openDialog(confirmResetNotification)
    }

    function openResetNotificationDialog() {
        return openDialog(resetNotification)
    }

    function openConfirmRemoveOrderDialog(params) {
        return openDialog(confirmRemoveOrder, params)
    }

    function openChannelRentalDialog(params) {
        return openDialog(rentalChannelComponent, params)
    }

    function openChannelDialog(params) {
        return openDialog(openChannelDialogComponent, params)
    }

    function openDexDisclaimerDialog() {
        return openDialog(dexDisclaimerComponent)
    }

    function openUnlockWalletDialog() {
        return openDialog(unlockWalletComponent)
    }

    function openDisclaimer() {
        if (!acceptedLiabilityTerms) {
            openDexDisclaimerDialog()
        }
    }

    function openUpdateNotificationDialog(params) {
        return openDialog(updateNotificationComponent, params)
    }

    function calculateTotal(amountStr, priceStr) {
        var amount = Utils.parseCoinsToSatoshi(amountStr)
        var price = parseFloat(priceStr) // parse as coins

        if (price > 0 && amount > 0) {
            var product = amount * price
            if (product >= 1) {
                return Utils.formatBalance(product, 8)
            } else {
                return Utils.formatBalance(0, 8)
            }
        } else {
            return ""
        }
    }

    function calculateAmount(newTotal, priceStr) {
        let price = parseFloat(priceStr)
        if (price > 0 && newTotal > 0) {
            return Utils.formatBalance(newTotal / price, 8)
        } else {
            return ""
        }
    }

    function calculateBestPrice(isBuy) {
        return dexViewModel.hasSwapPair ? (isBuy ? dexViewModel.stateModel.sellOrdersListModel.bestPrice : dexViewModel.stateModel.buyOrdersListModel.bestPrice) : 0
    }

    function showBubblePopup(messageText) {
        copyMessage.message = messageText
        copyMessage.shown = true
        copyMessage.startTimer()
    }

    function parseAssetsModel(model) {
        var result = ""
        for (var i = 0; i < model.length; i++) {
            result = result.concat(result === "" ? "" : "|", "^" + model[i])
        }
        return result
    }

    function hasEnoughBalance(proposedQuantity, availableBalance) {
        return proposedQuantity <= availableBalance
    }

    function calculatePlaceOrderFee(amount, feePercent) {
        var fee = Math.round(amount * feePercent)

        return fee > 0 ? fee : 1
    }
}
