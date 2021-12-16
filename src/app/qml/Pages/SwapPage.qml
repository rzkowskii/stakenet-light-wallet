import QtQuick 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.15
import Qt.labs.settings 1.0

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

Page {
    id: root
    property alias swapInProcess: swapViewModel.marketSwapInProcess

    property alias rootItem: item

    property alias leftAmountText: leftAmount.text
    property alias rightAmountText: rightAmount.text

    property alias leftAmountLocalText: leftAmountLocal.text
    property alias rightAmountLocalText: rightAmountLocal.text

    property bool isActiveLeftComboBox: leftComboBox.currentIndex !== -1
    property bool isActiveRightComboBox: rightComboBox.currentIndex !== -1

    property bool marketHasOrders: priceText !== ""
                                   && Utils.parseCoinsToSatoshi(
                                       priceText) !== 0.0

    property double orderFee: 0.0
    property double recommendedNetworkFeeRateForLeftAsset: 0
    property double recommendedNetworkFeeRateForRightAsset: 0
    onRecommendedNetworkFeeRateForLeftAssetChanged: {
        calculatePreSwapAmounts(leftAmountText, rightAmountText)
    }
    onRecommendedNetworkFeeRateForRightAssetChanged: {
        calculatePreSwapAmounts(leftAmountText, rightAmountText)
    }

    property bool isBuy: false
    property string priceText: ""
    property var tradingPairInfo: walletDexViewModel.tradingPairInfo

    property double receiveAmount: 0

    property var swapRequest
    property var fee: swapRequest ? swapRequest.fee : undefined

    property string onChainFeeStr: fee ? ApplicationViewModel.localCurrencyViewModel.convert(
                                             leftComboBox.currentAssetID,
                                             Utils.formatBalance(
                                                 fee.onChainChannelFee)) : ""

    property string onChainRentalFeeStr: fee ? ApplicationViewModel.localCurrencyViewModel.convert(
                                                   leftComboBox.currentAssetID,
                                                   Utils.formatBalance(
                                                       fee.onChainRentalFee)) : ""

    property string rentalFeeStr: fee ? ApplicationViewModel.localCurrencyViewModel.convert(
                                            leftComboBox.currentAssetID,
                                            Utils.formatBalance(
                                                fee.rentalFee)) : ""

    property string totalFeeStr: fee ? calculateTotalFee(onChainFeeStr,
                                                         onChainRentalFeeStr,
                                                         rentalFeeStr,
                                                         orderFeeStr) : ""

    property string orderFeeStr: ApplicationViewModel.localCurrencyViewModel.convert(
                                     leftComboBox.currentAssetID,
                                     Utils.formatBalance(orderFee))

    property double totalBuyOrdersAmount: walletDexViewModel.hasSwapPair ? walletDexViewModel.stateModel.ownOrderBookListModel.buyTotalAmount : 0
    property double totalSellOrdersAmount: walletDexViewModel.hasSwapPair ? walletDexViewModel.stateModel.ownOrderBookListModel.sellTotalAmount : 0

    property double totalAllBuyOrders: walletDexViewModel.hasSwapPair ? walletDexViewModel.stateModel.ownOrderBookListModel.totalBuy : 0
    property double totalAllSellOrders: walletDexViewModel.hasSwapPair ? walletDexViewModel.stateModel.ownOrderBookListModel.totalSell : 0

    property double leftAvailableBalance: walletDexViewModel.hasSwapPair ? Math.max(leftComboBox.currentTotalBalance - (isBuy ? totalAllSellOrders : totalSellOrdersAmount), 0) : 0
    property double rightAvailableBalance: walletDexViewModel.hasSwapPair ? Math.max(rightComboBox.currentTotalBalance - (isBuy ? totalBuyOrdersAmount : totalAllBuyOrders), 0) : 0

    property double baseBalance: walletDexViewModel.hasSwapPair ? Math.max(
                                                                      channelsBalance.baseBalance.localBalance - totalSellOrdersAmount,
                                                                      0) : 0
    property double quoteBalance: walletDexViewModel.hasSwapPair ? Math.max(
                                                                       channelsBalance.quoteBalance.localBalance - totalBuyOrdersAmount,
                                                                       0) : 0

    property double baseRemoteBalance: walletDexViewModel.hasSwapPair ? Math.max(
                                                                            channelsBalance.baseBalance.remoteBalance - totalAllBuyOrders,
                                                                            0) : 0
    property double quoteRemoteBalance: walletDexViewModel.hasSwapPair ? Math.max(
                                                                             channelsBalance.quoteBalance.remoteBalance - totalAllSellOrders,
                                                                             0) : 0
    property var channelsBalance: channelBalancesViewModel.channelsBalance

    property string errorMessage: ""

    property double expectedTotalRentalFee: 0.0

    property int minLocalRentalAmount: 5
    property int maxLocalRentalAmount: 10000

    property double sendReserve: 0
    property double receiveReserve: 0

    property bool preSwapAmountsCalculated: false
    property bool calculateToggle: false

    onBaseBalanceChanged: {
        calculatePreSwapAmounts(leftAmountText, rightAmountText)
    }

    onQuoteBalanceChanged: {
        calculatePreSwapAmounts(leftAmountText, rightAmountText)
    }

    onBaseRemoteBalanceChanged: {
        calculatePreSwapAmounts(leftAmountText, rightAmountText)
    }

    onQuoteRemoteBalanceChanged: {
        calculatePreSwapAmounts(leftAmountText, rightAmountText)
    }

    onLeftAvailableBalanceChanged: {
        choosingListView.currentIndex = -1
        evaluateBalances()
    }

    onRightAvailableBalanceChanged: {
        evaluateBalances()
    }

    property WalletDexViewModel walletDexViewModel

    onPriceTextChanged: {
        exchange.update()
        if (isBuy) {
            rightAmountText = calculateAmount(Utils.parseCoinsToSatoshi(
                                                  leftAmountText), priceText)
        } else {
            rightAmountText = calculateTotal(leftAmountText, priceText)
        }
        orderFee = calculatePlaceOrderFee(Utils.parseCoinsToSatoshi(
                                              leftAmountText),
                                          isBuy ? tradingPairInfo.buyFeePercent : tradingPairInfo.sellFeePercent)
        evaluateBalances()
    }

    onIsBuyChanged: {
        updateBestPrice()
        orderFee = calculatePlaceOrderFee(Utils.parseCoinsToSatoshi(
                                              leftAmountText),
                                          isBuy ? tradingPairInfo.buyFeePercent : tradingPairInfo.sellFeePercent)
        choosingListView.currentIndex = -1
        evaluateBalances()
    }

    Connections {
        target: ApplicationViewModel.localCurrencyViewModel
        function onCurrencyRateChanged(assetID) {
            if (isActiveLeftComboBox
                    && leftComboBox.currentAssetID === assetID) {
                leftAmountLocal.localAmountRefresh()
            }
            if (isActiveRightComboBox
                    && rightComboBox.currentAssetID === assetID) {
                rightAmountLocal.localAmountRefresh()
            }
        }
    }

    PaymentNodeViewModel {
        id: lightningBaseViewModel
        currentAssetID: walletDexViewModel.baseAssetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    PaymentNodeViewModel {
        id: lightningQuoteViewModel
        currentAssetID: walletDexViewModel.quoteAssetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    WalletDexChannelBalanceViewModel {
        id: channelBalancesViewModel

        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    WalletMarketSwapViewModel {
        id: swapViewModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }

        onEvaluateChannelsRequested: {
            swapRequest = request
            item.state = "confirm"
        }

        onEvaluateChannelsFailed: {
            var popup = openDialog(dexPlaceOrderNotification, {
                                       "errorMessage": error
                                   })
            item.state = "default"
        }

        onSwapSuccess: {
            settings.receivedAmount = amount
            settings.receiveAssetSymbol = receiveSymbol
            item.state = "success"
            showBubblePopup("Swap successful for %1 %2".arg(
                                Utils.formatBalance(amount)).arg(receiveSymbol))
        }

        onSwapFailed: {
            settings.errMsg = error
            errorMessage = error
            item.state = "failed"
        }

        onRentalFeeFailed: {
             showBubblePopup("Rental fee failed".arg(error))
        }

        onCalculatePreSwapAmountsFinished: {
            preSwapAmountsCalculated = true
            expectedTotalRentalFee = totalRentalFee
            sendReserve = sendAssetReserve
            receiveReserve = receiveAssetReserve

            if (choosingListView.currentIndex !== -1 && calculateToggle) {
                choosingListView.calculateToggleAmount()
            }
            evaluateBalances()
        }
    }

    Connections {
        enabled: walletDexViewModel.hasSwapPair
        target: walletDexViewModel
                && walletDexViewModel.stateModel ? isBuy ? walletDexViewModel.stateModel.sellOrdersListModel : walletDexViewModel.stateModel.buyOrdersListModel : null
        function onBestPriceChanged() {
            updateBestPrice()
        }
    }

    Connections {
        target: mainPage
        function onAcceptedLiabilityTermsChanged() {
            resetComboBoxes()
        }
    }
    property bool changeSwapAssetsInProgress: false

    Connections {
        target: walletDexViewModel
        function onSwapAssetsChanged() {
            changeSwapAssetsInProgress = false;
            if ((isBuy
                 && (walletDexViewModel.baseAssetID !== rightComboBox.currentAssetID
                     || walletDexViewModel.quoteAssetID !== leftComboBox.currentAssetID))
                    || (!isBuy
                        && (walletDexViewModel.baseAssetID !== leftComboBox.currentAssetID
                            || walletDexViewModel.quoteAssetID !== rightComboBox.currentAssetID))) {
                changeComboBoxPair(walletDexViewModel.baseAssetID,
                                   walletDexViewModel.quoteAssetID)
            }
            updateBalancesModel()
            updateBestPrice()
        }
    }

    SwapAssetsModel {
        id: swapAssetsModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
        onModelReset: {
            resetComboBoxes()
        }
    }

    WalletAssetsListModel {
        id: assetModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }

        onAverageFeeForAssetFinished: {
            if (assetID === leftComboBox.currentAssetID) {
                recommendedNetworkFeeRateForLeftAsset = value
                return
            }
            if (assetID === rightComboBox.currentAssetID) {
                recommendedNetworkFeeRateForRightAsset = value
            }
        }
    }

    background: Rectangle {
        id: swapPageImageBgr
        color: "#050B1E"
    }

    Connections {
        target: ApplicationViewModel.paymentNodeViewModel
        function onChangeSwapAssetsFailed(error) {
            if(root.focus) {
                commonNotification.text = "No such trading pair"
            }
        }
    }

    /*Image {
        anchors.fill: parent
        source: "qrc:/images/swapPageBackground.svg"
    }*/
    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    FontLoader {
        id: fontLight
        source: "qrc:/Rubik-Light.ttf"
    }

    Text {
        text: "Swap"
        font.pixelSize: 30
        font.family: fontLight.name
        font.weight: Font.Light
        color: SkinColors.mainText
        anchors.topMargin: 35
        anchors.leftMargin: 35
        anchors.top: parent.top
        anchors.left: parent.left
    }

    StackLayout {
        id: item
        anchors.fill: parent
        anchors.margins: 35
        anchors.topMargin: 85
        state: settings.state

        onStateChanged: {
            switch (state) {
            case "default":
                settings.state = state
                currentIndex = 0
                break
            case "confirm":
                currentIndex = 0
                break
            case "in progress":
                currentIndex = 1
                break
            case "success":
                settings.state = state
                currentIndex = 1 //2
                break
            case "failed":
                settings.state = state
                currentIndex = 1 // 3
                break
            default:
                currentIndex = 0
            }
        }

        clip: true
        onHeightChanged: {
            if (state == "default") {
                headerItem.Layout.preferredHeight = Qt.binding(function () {
                    return item.height * 0.54
                })
                footerItem.Layout.preferredHeight = Qt.binding(function () {
                    return item.height * 0.46
                })
            } else if (state == "confirm") {
                headerItem.Layout.preferredHeight = Qt.binding(function () {
                    return item.height * 0.07
                })
                footerItem.Layout.preferredHeight = Qt.binding(function () {
                    return item.height * 0.93
                })
            } else {
                headerItem.Layout.preferredHeight = Qt.binding(function () {
                    return 0
                })
                footerItem.Layout.preferredHeight = Qt.binding(function () {
                    return 0
                })
            }
        }

        states: [
            State {
                name: "default"

                PropertyChanges {
                    target: cancelItem
                    visible: false
                }
                PropertyChanges {
                    target: confirmInfoView
                    visible: false
                }
                PropertyChanges {
                    target: bottomInfo
                    spacing: 0
                }
            },

            State {
                name: "confirm"

                PropertyChanges {
                    target: exchangeInProgressView.checkmark
                    scale: 0.0
                }

                PropertyChanges {
                    target: exchangeInProgressView.stateText
                    opacity: 0.0
                }

                PropertyChanges {
                    target: exchangeInProgressView.info
                    opacity: 0.0
                }

                PropertyChanges {
                    target: exchangeInProgressView.description
                    opacity: 0.0
                }

                PropertyChanges {
                    target: exchangeInProgressView.willReceiveLocal
                    opacity: 0.0
                }

                PropertyChanges {
                    target: exchangeInProgressView.willReceiveCoins
                    opacity: 0.0
                }

                PropertyChanges {
                    target: exchangeInProgressView.designedImage
                    opacity: 0.0
                }

                PropertyChanges {
                    target: exchangeInProgressView
                    opacity: 0.0
                }
            },

            State {
                name: "in progress"
            },

            State {
                name: "success"

                PropertyChanges {
                    target: exchangeInProgressView.stateText
                    opacity: 1.0
                }

                PropertyChanges {
                    target: exchangeInProgressView.cancelImage
                    opacity: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.designedImage
                    opacity: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.horizonalSeparatedLine1
                    opacity: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.willReceiveCoins
                    opacity: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.willReceiveLocal
                    opacity: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.checkmark
                    scale: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.receiveImage
                    scale: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.receiveAssetImage
                    scale: 1.0
                }
            },

            State {
                name: "failed"
                PropertyChanges {
                    target: exchangeInProgressView.stateText
                    opacity: 1.0
                }

                PropertyChanges {
                    target: exchangeInProgressView.cancelImage
                    opacity: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.designedImage
                    opacity: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.horizonalSeparatedLine1
                    opacity: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.checkmark
                    scale: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.receiveImage
                    scale: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.receiveAssetImage
                    scale: 1.0
                }
                PropertyChanges {
                    target: exchangeInProgressView.errorMessageText
                    opacity: 1.0
                }
            }
        ]

        transitions: [
            Transition {
                from: "default"
                to: "confirm"

                SequentialAnimation {
                    ParallelAnimation {
                        RotationAnimation {
                            target: swapIcon.swapArrowsItem
                            duration: 600
                            from: 0
                            to: 360
                        }

                        PropertyAnimation {
                            target: swapIcon
                            property: "opacity"
                            to: 0.0
                            duration: 600
                        }
                    }

                    ParallelAnimation {
                        SequentialAnimation {
                            ParallelAnimation {
                                PropertyAnimation {
                                    target: mainView
                                    property: "visible"
                                    to: false
                                    duration: 1000
                                }
                                PropertyAnimation {
                                    target: mainView
                                    property: "opacity"
                                    to: 0.0
                                    duration: 700
                                }
                                PropertyAnimation {
                                    target: headerItem
                                    property: "Layout.preferredHeight"
                                    to: item.height * 0.07
                                    duration: 1000
                                }
                                PropertyAnimation {
                                    target: footerItem
                                    property: "Layout.preferredHeight"
                                    to: item.height * 0.93
                                    duration: 1000
                                }

                                PropertyAnimation {
                                    target: confirmInfoView
                                    property: "visible"
                                    to: true
                                    duration: 1000
                                }
                            }

                            ParallelAnimation {
                                PropertyAnimation {
                                    target: cancelItem
                                    property: "visible"
                                    to: true
                                    duration: 500
                                }
                                PropertyAnimation {
                                    target: cancelItem
                                    property: "opacity"
                                    from: 0.0
                                    to: 1.0
                                    duration: 500
                                }

                                PropertyAnimation {
                                    target: willReceiveInfo
                                    property: "opacity"
                                    from: 0.0
                                    to: 1.0
                                    duration: 600
                                }

                                PropertyAnimation {
                                    target: willReceiveInfo
                                    property: "scale"
                                    from: 0.0
                                    to: 1.0
                                    duration: 600
                                }

                                PropertyAnimation {
                                    targets: [gradientHorizontalLine1, gradientHorizontalLine2, gradientVerticalLine]
                                    property: "opacity"
                                    to: 1.0
                                    duration: 1000
                                }

                                SequentialAnimation {

                                    PauseAnimation {
                                        duration: 400
                                    }

                                    ParallelAnimation {
                                        PropertyAnimation {
                                            targets: [leftInfo, rightInfo]
                                            property: "opacity"
                                            to: 1.0
                                            duration: 700
                                        }

                                        PropertyAnimation {
                                            target: bottomInfo
                                            property: "spacing"
                                            to: 32
                                            duration: 700
                                        }
                                    }
                                }
                            }
                        }

                        PropertyAnimation {
                            target: swapPageImageBgr
                            property: "scale"
                            to: 1.7
                            duration: 2000
                        }

                        RotationAnimation {
                            target: swapPageImageBgr
                            direction: RotationAnimation.Shortest
                            to: 335
                            duration: 2000
                        }
                    }
                }
            },

            Transition {
                from: "confirm"
                to: "default"

                ParallelAnimation {
                    SequentialAnimation {
                        PauseAnimation {
                            duration: 750
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                target: swapIcon
                                property: "opacity"
                                to: 1
                                duration: 650
                            }
                            RotationAnimation {
                                target: swapIcon.swapArrowsItem
                                duration: 650
                                from: 360
                                to: 0
                            }
                        }
                    }

                    PropertyAnimation {
                        target: exchangeInProgressView.errorMessageText
                        duration: 700
                        property: "opacity"
                        to: 0.0
                    }

                    PropertyAnimation {
                        target: headerItem
                        property: "Layout.preferredHeight"
                        to: item.height * 0.54
                        duration: 1000
                    }
                    PropertyAnimation {
                        target: footerItem
                        property: "Layout.preferredHeight"
                        to: item.height * 0.46
                        duration: 1000
                    }
                    PropertyAnimation {
                        target: cancelItem
                        property: "visible"
                        to: false
                        duration: 1000
                    }
                    PropertyAnimation {
                        target: cancelItem
                        property: "opacity"
                        to: 0.0
                        duration: 1000
                    }

                    PropertyAnimation {
                        target: mainView
                        property: "visible"
                        to: true
                        duration: 750
                    }
                    SequentialAnimation {

                        PauseAnimation {
                            duration: 750
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                target: mainView
                                property: "opacity"
                                to: 1.0
                                duration: 650
                            }
                        }
                    }

                    PropertyAnimation {
                        target: confirmInfoView
                        property: "visible"
                        to: false
                        duration: 1000
                    }

                    PropertyAnimation {
                        targets: [gradientHorizontalLine1, gradientHorizontalLine2, gradientVerticalLine, willReceiveInfo]
                        property: "opacity"
                        from: 0.0
                        to: 0.0
                        duration: 400
                    }

                    PropertyAnimation {
                        target: willReceiveInfo
                        property: "scale"
                        from: 1.0
                        to: 0.0
                        duration: 400
                    }
                    PropertyAnimation {
                        targets: [leftInfo, rightInfo]
                        property: "opacity"
                        to: 0.0
                        duration: 200
                    }

                    PropertyAnimation {
                        target: bottomInfo
                        property: "spacing"
                        to: 0
                        duration: 600
                    }

                    PropertyAnimation {
                        target: swapPageImageBgr
                        property: "scale"
                        to: 1.0
                        duration: 1200
                    }

                    RotationAnimation {
                        target: swapPageImageBgr
                        direction: RotationAnimation.Shortest
                        to: 0
                        duration: 1200
                    }
                }
            },

            Transition {
                from: "confirm"
                to: "in progress"

                ParallelAnimation {

                    PropertyAnimation {
                        targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                        duration: 0
                        property: "opacity"
                        to: 0.0
                    }

                    SequentialAnimation {
                        ParallelAnimation {
                            PropertyAnimation {
                                target: bottomContent
                                property: "opacity"
                                to: 0.0
                                duration: 600
                            }

                            PropertyAnimation {
                                targets: [gradientHorizontalLine1, gradientHorizontalLine2, gradientVerticalLine, willReceiveInfo, leftInfo, rightInfo]
                                property: "opacity"
                                to: 0.0
                                from: 1.0
                                duration: 600
                            }

                            PropertyAnimation {
                                target: willReceiveInfo
                                property: "scale"
                                from: 1.0
                                to: 0.0
                                duration: 600
                            }
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 600
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                target: headerItem
                                property: "Layout.preferredHeight"
                                to: 0
                                duration: 300
                            }

                            PropertyAnimation {
                                target: footerItem
                                property: "Layout.preferredHeight"
                                to: item.height
                                duration: 300
                            }

                            PropertyAnimation {
                                target: cancelItem
                                property: "visible"
                                to: false
                                duration: 300
                            }

                            PropertyAnimation {
                                targets: [cancelItem, footerItem]
                                property: "opacity"
                                to: 0.0
                                duration: 300
                            }

                            PropertyAnimation {
                                target: confirmInfoView
                                property: "visible"
                                to: false
                                duration: 300
                            }

                            PropertyAnimation {
                                target: footerItem
                                property: "visible"
                                to: false
                                duration: 300
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                                duration: 300
                            }
                        }
                    }

                    SequentialAnimation {
                        PropertyAnimation {
                            target: swapPageImageBgr
                            property: "scale"
                            to: 2.2
                            duration: 600
                        }

                        ParallelAnimation {

                            PropertyAnimation {
                                target: swapPageImageBgr
                                property: "scale"
                                to: 1.0
                                duration: 900
                            }

                            RotationAnimation {
                                target: swapPageImageBgr
                                direction: RotationAnimation.Shortest
                                to: 0
                                duration: 900
                            }
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 1300
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                targets: [exchangeInProgressView.horizonalSeparatedLine1, exchangeInProgressView.horizonalSeparatedLine2]
                                property: "opacity"
                                to: 1.0
                                from: 0.0
                                duration: 700
                            }
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 900
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                                duration: 600
                                property: "scale"
                                from: 0.0
                                to: 1.0
                            }
                            PropertyAnimation {
                                targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                                duration: 300
                                property: "opacity"
                                from: 0.5
                                to: 1.0
                            }

                            RotationAnimation {
                                targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                                from: 30
                                to: 360
                                duration: 600
                            }
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 1200
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                target: exchangeInProgressView.checkmark
                                duration: 600
                                property: "scale"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.stateText
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.stateText
                                property: "y"
                                from: -item.height * 0.04
                                to: exchangeInProgressView.stateItem.verticalCenter
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.description
                                duration: 700
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.description
                                property: "y"
                                from: -item.height * 0.04
                                to: item.height * 0.04
                                    - exchangeInProgressView.description.contentHeight / 2
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.willReceiveLocal
                                duration: 700
                                property: "opacity"
                                to: 1.0
                                from: 0.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.willReceiveLocal
                                property: "y"
                                from: -item.height * 0.04
                                to: item.height * 0.04
                                    - exchangeInProgressView.willReceiveLocal.contentHeight / 2
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.willReceiveCoins
                                duration: 700
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.willReceiveCoins
                                property: "y"
                                from: -item.height * 0.02
                                to: item.height * 0.12
                                    - exchangeInProgressView.willReceiveCoins.contentHeight / 2
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.stateText
                                duration: 700
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                            }
                            PropertyAnimation {
                                target: exchangeInProgressView.description
                                duration: 700
                                from: 0.0
                                property: "opacity"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.info
                                duration: 700
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.designedImage
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }
                        }
                    }
                }
            },

            Transition {
                from: "in progress"
                to: "default"

                SequentialAnimation {
                    ParallelAnimation {
                        PropertyAnimation {
                            target: exchangeInProgressView.newExchangeSwapButton
                            property: "opacity"
                            from: 1.0
                            to: 0.0
                            duration: 600
                        }

                        PropertyAnimation {
                            target: exchangeInProgressView.newExchangeSwapButton
                            property: "Layout.preferredWidth"
                            from: item.width * 0.22
                            to: 0
                            duration: 600
                        }

                        PropertyAnimation {
                            target: exchangeInProgressView
                            property: "opacity"
                            to: 0.0
                            duration: 600
                        }
                        PropertyAnimation {
                            target: exchangeInProgressView
                            property: "visible"
                            to: 0.0
                            duration: 600
                        }
                    }

                    ParallelAnimation {
                        PropertyAnimation {
                            target: headerItem
                            property: "Layout.preferredHeight"
                            to: 0
                            duration: 1
                        }

                        PropertyAnimation {
                            target: footerItem
                            property: "Layout.preferredHeight"
                            to: item.height
                            duration: 1
                        }

                        PropertyAnimation {
                            target: footerItem
                            property: "visible"
                            to: true
                            duration: 1
                        }

                        PropertyAnimation {
                            target: footerItem
                            property: "opacity"
                            to: 1.0
                            duration: 300
                        }
                        PropertyAnimation {
                            target: headerItem
                            property: "visible"
                            to: true
                            duration: 1
                        }

                        PropertyAnimation {
                            target: bottomContent
                            property: "opacity"
                            to: 1.0
                            duration: 1
                        }

                        SequentialAnimation {
                            PauseAnimation {
                                duration: 900
                            }

                            ParallelAnimation {
                                PropertyAnimation {
                                    target: swapIcon
                                    property: "opacity"
                                    to: 1
                                    duration: 600
                                }
                                RotationAnimation {
                                    target: swapIcon.swapArrowsItem
                                    duration: 600
                                    from: 360
                                    to: 0
                                }
                            }
                        }

                        ParallelAnimation {

                            PauseAnimation {
                                duration: 200
                            }

                            PropertyAnimation {
                                target: headerItem
                                property: "Layout.preferredHeight"
                                to: item.height * 0.54
                                duration: 1000
                            }

                            PropertyAnimation {
                                target: footerItem
                                property: "Layout.preferredHeight"
                                to: item.height * 0.46
                                duration: 1000
                            }

                            PropertyAnimation {
                                target: mainView
                                property: "opacity"
                                to: 1.0
                                duration: 1000
                            }
                        }

                        PropertyAnimation {
                            target: mainView
                            property: "visible"
                            to: true
                            duration: 400
                        }
                    }
                }
            },

            Transition {
                from: "in progress"
                to: "failed"

                ParallelAnimation {
                    PropertyAnimation {
                        targets: [exchangeInProgressView.stateText, exchangeInProgressView.errorMessageText, exchangeInProgressView.cancelImage, exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage, exchangeInProgressView.designedImage, exchangeInProgressView.horizonalSeparatedLine1]
                        property: "opacity"
                        to: 0.0
                        duration: 0
                    }

                    PropertyAnimation {
                        target: exchangeInProgressView
                        property: "opacity"
                        to: 1.0
                        duration: 700
                    }

                    PropertyAnimation {
                        targets: [exchangeInProgressView.checkmark, exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                        duration: 0
                        property: "scale"
                        to: 0.0
                    }

                    SequentialAnimation {
                        PropertyAnimation {
                            target: swapPageImageBgr
                            property: "scale"
                            to: 2.2
                            duration: 600
                        }

                        ParallelAnimation {

                            PropertyAnimation {
                                target: swapPageImageBgr
                                property: "scale"
                                to: 1.0
                                duration: 900
                            }

                            RotationAnimation {
                                target: swapPageImageBgr
                                direction: RotationAnimation.Shortest
                                to: 0
                                duration: 900
                            }
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 1300
                        }

                        PropertyAnimation {
                            target: exchangeInProgressView.horizonalSeparatedLine1
                            property: "opacity"
                            to: 1.0
                            from: 0.0
                            duration: 700
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 900
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                                duration: 600
                                property: "scale"
                                from: 0.0
                                to: 1.0
                            }
                            PropertyAnimation {
                                targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                                duration: 300
                                property: "opacity"
                                from: 0.5
                                to: 1.0
                            }

                            RotationAnimation {
                                targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                                from: 30
                                to: 360
                                duration: 600
                            }
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 1200
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                target: exchangeInProgressView.checkmark
                                duration: 600
                                property: "scale"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.stateText
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.cancelImage
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.stateText
                                property: "y"
                                from: -item.height * 0.04
                                to: exchangeInProgressView.stateItem.verticalCenter
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.errorMessageText
                                property: "y"
                                from: -item.height * 0.05
                                to: item.height * 0.05
                                    - exchangeInProgressView.errorMessageText.contentHeight / 2
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.stateText
                                duration: 700
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.errorMessageText
                                duration: 700
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.designedImage
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }
                        }
                    }
                }
            },

            Transition {
                from: "in progress"
                to: "success"

                ParallelAnimation {
                    PropertyAnimation {
                        targets: [exchangeInProgressView.stateText, exchangeInProgressView.cancelImage, exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage, exchangeInProgressView.designedImage, exchangeInProgressView.horizonalSeparatedLine1, exchangeInProgressView.willReceiveCoins, exchangeInProgressView.willReceiveLocal]
                        property: "opacity"
                        to: 0.0
                        duration: 0
                    }

                    PropertyAnimation {
                        target: exchangeInProgressView
                        property: "opacity"
                        from: 0.0
                        to: 1.0
                        duration: 700
                    }

                    PropertyAnimation {
                        targets: [exchangeInProgressView.checkmark, exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                        duration: 0
                        property: "scale"
                        to: 0.0
                    }

                    SequentialAnimation {
                        PropertyAnimation {
                            target: swapPageImageBgr
                            property: "scale"
                            to: 2.2
                            duration: 600
                        }

                        ParallelAnimation {

                            PropertyAnimation {
                                target: swapPageImageBgr
                                property: "scale"
                                to: 1.0
                                duration: 900
                            }

                            RotationAnimation {
                                target: swapPageImageBgr
                                direction: RotationAnimation.Shortest
                                to: 0
                                duration: 900
                            }
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 1300
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                targets: [exchangeInProgressView.horizonalSeparatedLine1, exchangeInProgressView.horizonalSeparatedLine2]
                                property: "opacity"
                                to: 1.0
                                from: 0.0
                                duration: 700
                            }
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 900
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                                duration: 600
                                property: "scale"
                                from: 0.0
                                to: 1.0
                            }
                            PropertyAnimation {
                                targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                                duration: 300
                                property: "opacity"
                                from: 0.5
                                to: 1.0
                            }

                            RotationAnimation {
                                targets: [exchangeInProgressView.receiveImage, exchangeInProgressView.receiveAssetImage]
                                from: 30
                                to: 360
                                duration: 600
                            }
                        }
                    }

                    SequentialAnimation {

                        PauseAnimation {
                            duration: 1200
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                target: exchangeInProgressView.checkmark
                                duration: 600
                                property: "scale"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.stateText
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.stateText
                                property: "y"
                                from: -item.height * 0.04
                                to: exchangeInProgressView.stateItem.verticalCenter
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.cancelImage
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.description
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.description
                                property: "y"
                                from: -item.height * 0.04
                                to: item.height * 0.02
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.willReceiveLocal
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.willReceiveLocal
                                property: "y"
                                from: -item.height * 0.04
                                to: item.height * 0.04
                                    - exchangeInProgressView.willReceiveLocal.contentHeight / 2
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.willReceiveCoins
                                duration: 700
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.willReceiveCoins
                                property: "y"
                                from: -item.height * 0.02
                                to: item.height * 0.12
                                    - exchangeInProgressView.willReceiveCoins.contentHeight / 2
                                duration: 700
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.stateText
                                duration: 700
                                property: "opacity"
                                from: 0.0
                                to: 1.0
                            }
                            PropertyAnimation {
                                target: exchangeInProgressView.description
                                duration: 700
                                from: 0.0
                                property: "opacity"
                                to: 1.0
                            }

                            PropertyAnimation {
                                target: exchangeInProgressView.designedImage
                                duration: 700
                                property: "opacity"
                                to: 1.0
                            }
                        }
                    }
                }
            },

            Transition {
                from: "success"
                to: "default"

                ParallelAnimation {
                    PropertyAnimation {
                        target: exchangeInProgressView
                        property: "opacity"
                        from: 1.0
                        to: 0.0
                        duration: 700
                    }
                    PropertyAnimation {
                        target: headerItem
                        property: "Layout.preferredHeight"
                        to: 0
                        duration: 1
                    }

                    PropertyAnimation {
                        target: footerItem
                        property: "Layout.preferredHeight"
                        to: item.height
                        duration: 1
                    }

                    PropertyAnimation {
                        target: footerItem
                        property: "visible"
                        to: true
                        duration: 1
                    }

                    PropertyAnimation {
                        target: footerItem
                        property: "opacity"
                        to: 1.0
                        duration: 300
                    }
                    PropertyAnimation {
                        target: headerItem
                        property: "visible"
                        to: true
                        duration: 1
                    }

                    PropertyAnimation {
                        target: bottomContent
                        property: "opacity"
                        to: 1.0
                        duration: 1
                    }

                    SequentialAnimation {
                        PauseAnimation {
                            duration: 900
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                target: swapIcon
                                property: "opacity"
                                to: 1
                                duration: 600
                            }
                            RotationAnimation {
                                target: swapIcon.swapArrowsItem
                                duration: 600
                                from: 360
                                to: 0
                            }
                        }
                    }

                    ParallelAnimation {

                        PauseAnimation {
                            duration: 200
                        }

                        PropertyAnimation {
                            target: headerItem
                            property: "Layout.preferredHeight"
                            to: item.height * 0.54
                            duration: 1000
                        }

                        PropertyAnimation {
                            target: footerItem
                            property: "Layout.preferredHeight"
                            to: item.height * 0.46
                            duration: 1000
                        }

                        PropertyAnimation {
                            target: mainView
                            property: "opacity"
                            to: 1.0
                            duration: 1000
                        }
                    }

                    PropertyAnimation {
                        target: mainView
                        property: "visible"
                        to: true
                        duration: 400
                    }
                }
            },

            Transition {
                from: "failed"
                to: "default"

                ParallelAnimation {
                    PropertyAnimation {
                        target: exchangeInProgressView
                        property: "opacity"
                        from: 1.0
                        to: 0.0
                        duration: 700
                    }
                    PropertyAnimation {
                        target: headerItem
                        property: "Layout.preferredHeight"
                        to: 0
                        duration: 1
                    }

                    PropertyAnimation {
                        target: footerItem
                        property: "Layout.preferredHeight"
                        to: item.height
                        duration: 1
                    }

                    PropertyAnimation {
                        target: footerItem
                        property: "visible"
                        to: true
                        duration: 1
                    }

                    PropertyAnimation {
                        target: footerItem
                        property: "opacity"
                        to: 1.0
                        duration: 300
                    }
                    PropertyAnimation {
                        target: headerItem
                        property: "visible"
                        to: true
                        duration: 1
                    }

                    PropertyAnimation {
                        target: bottomContent
                        property: "opacity"
                        to: 1.0
                        duration: 1
                    }

                    SequentialAnimation {
                        PauseAnimation {
                            duration: 900
                        }

                        ParallelAnimation {
                            PropertyAnimation {
                                target: swapIcon
                                property: "opacity"
                                to: 1
                                duration: 600
                            }
                            RotationAnimation {
                                target: swapIcon.swapArrowsItem
                                duration: 600
                                from: 360
                                to: 0
                            }
                        }
                    }

                    ParallelAnimation {

                        PauseAnimation {
                            duration: 200
                        }

                        PropertyAnimation {
                            target: headerItem
                            property: "Layout.preferredHeight"
                            to: item.height * 0.54
                            duration: 1000
                        }

                        PropertyAnimation {
                            target: footerItem
                            property: "Layout.preferredHeight"
                            to: item.height * 0.46
                            duration: 1000
                        }

                        PropertyAnimation {
                            target: mainView
                            property: "opacity"
                            to: 1.0
                            duration: 1000
                        }
                    }

                    PropertyAnimation {
                        target: mainView
                        property: "visible"
                        to: true
                        duration: 400
                    }
                }
            }
        ]

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Item {
                id: headerItem
                Layout.preferredHeight: item.height * 0.54
                Layout.fillWidth: true

                GradientBackgroundComponent {
                    anchors.fill: parent
                    gradientOpacity: 0.7
                    firstColor: SkinColors.swapPageBackgroundHeaderFirst
                    secondColor: SkinColors.swapPageBackgroundHeaderSecond
                    thirdColor: SkinColors.swapPageBackgroundHeaderThird
                }

                ColumnLayout {
                    id: mainView
                    anchors.fill: parent
                    anchors.topMargin: parent.height * 0.077 //63
                    anchors.leftMargin: item.width * 0.085 //130
                    anchors.rightMargin: item.width * 0.085 //130
                    anchors.bottomMargin: parent.height * 0.061 //50
                    spacing: item.height * 0.01 //15
                    clip: true

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: parent.width * 0.057 //85

                        ColumnLayout {
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: item.width * 0.259 //386
                            Layout.topMargin: 15
                            spacing: 8

                            SwapComboBox {
                                id: leftComboBox
                                Layout.fillWidth: true
                                Layout.preferredHeight: 65
                                currentIndex: -1
                                enabled: walletDexViewModel.online && acceptedLiabilityTerms

                                model: QMLSortFilterListProxyModel {
                                    id: sortedLeftAssetModel
                                    source: QMLSortFilterListProxyModel {
                                        source: assetModel
                                        filterRole: "isActive"
                                        filterString: "1"
                                        filterCaseSensitivity: Qt.CaseInsensitive
                                    }
                                    filterRole: "symbol"
                                    filterString: parseAssetsModel(
                                                      swapAssetsModel.baseAssets)
                                    filterCaseSensitivity: Qt.CaseInsensitive
                                }

                                onCurrentAssetIDChanged: {
                                    if (currentIndex !== -1) {
                                        assetModel.averageFeeForAsset(
                                                    currentAssetID)
                                    }
                                    if (leftAmount.text !== "") {
                                        leftAmount.focus = true
                                        leftAmountLocal.localAmountRefresh()
                                        leftAmount.focus = false
                                        exchange.update()
                                    }
                                }

                                onActivated: {
                                    if (currentIndex !== -1
                                            && currentIndex !== actualIndex
                                            && rightComboBox.currentIndex !== -1) {
                                        activatePair()
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 50
                                spacing: 8

                                SwapTextField {
                                    id: leftAmount
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 50 //item.height * 0.073 // 60
                                    placeholderText: "Amount"
                                    enabled: walletDexViewModel.hasSwapPair
                                    validator: RegExpValidator {
                                        regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                                    }
                                    unitValue: isActiveLeftComboBox ? leftComboBox.currentSymbol : ""
                                    onActiveFocusChanged: {
                                        if (activeFocus) {
                                            choosingListView.currentIndex = -1
                                        }
                                    }

                                    onTextChanged: {
                                        if (text === ".") {
                                            text = "0."
                                        }
                                        if (parseFloat(text) <= 0) {
                                            leftNotification.text
                                                    = "Amount must be greater than zero"
                                            if (!leftAmountLocal.focus
                                                    && focus) {
                                                leftAmountLocal.text = "0.0"
                                                return
                                            }
                                        }

                                        var newAmoutLocal = ApplicationViewModel.localCurrencyViewModel.convert(
                                                    leftComboBox.currentAssetID,
                                                    text)
                                        if (text === "") {
                                            newAmoutLocal = ""
                                            leftNotification.text = ""
                                        }

                                        if (!leftAmountLocal.focus) {
                                            leftAmountLocal.text = newAmoutLocal
                                        }
                                        var newRightAmount = ""
                                        if (isBuy) {
                                            newRightAmount = calculateAmount(
                                                        Utils.parseCoinsToSatoshi(
                                                            text), priceText)
                                        } else {
                                            newRightAmount = calculateTotal(
                                                        text, priceText)
                                        }

                                        if (!rightAmount.focus
                                                && !rightAmountLocal.focus) {
                                            rightAmountText = newRightAmount
                                        }

                                        orderFee = calculatePlaceOrderFee(
                                                    Utils.parseCoinsToSatoshi(
                                                        text),
                                                    isBuy ? tradingPairInfo.buyFeePercent : tradingPairInfo.sellFeePercent)

                                        if (choosingListView.currentIndex === -1
                                                || calculateToggle) {
                                            calculatePreSwapAmounts(text, rightAmountText)
                                        }

                                        evaluateBalances()
                                    }
                                }

                                SwapTextField {
                                    id: leftAmountLocal
                                    Layout.preferredWidth: parent.width * 0.3
                                    Layout.preferredHeight: 50 //item.height * 0.073 // 60
                                    enabled: walletDexViewModel.hasSwapPair
                                    placeholderText: ""
                                    validator: RegExpValidator {
                                        regExp: /^(([1-9]{1,})+[0-9]*(\.?[0-9]{0,2})$|0?(\.[0-9]{0,2}))$/gm
                                    }
                                    unitValue: ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol
                                    symbolOnLeft: true
                                    onActiveFocusChanged: {
                                        if (activeFocus) {
                                            choosingListView.currentIndex = -1
                                        }
                                    }
                                    onTextChanged: {
                                        if (text === ".") {
                                            leftAmountLocal.text = "0."
                                        }

                                        var newAmoutCoin = ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                                    leftComboBox.currentAssetID,
                                                    text)

                                        if (text === "") {
                                            newAmoutCoin = ""
                                        }
                                        if (!leftAmount.focus && focus
                                                && choosingListView.currentIndex === -1) {
                                            leftAmount.text = newAmoutCoin
                                        }
                                    }
                                    function localAmountRefresh() {
                                        if (leftAmountText == "") {
                                            text = ""
                                            return
                                        }

                                        text = ApplicationViewModel.localCurrencyViewModel.convert(
                                                    leftComboBox.currentAssetID,
                                                    leftAmountText)
                                    }
                                }
                            }
                            Item {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.fillWidth: true
                                Layout.preferredHeight: 35

                                Text {
                                    id: leftNotification
                                    visible: text.length > 0
                                    anchors.leftMargin: 14
                                    anchors.rightMargin: 14
                                    anchors.left: parent.left
                                    color: SkinColors.transactionItemSent
                                    font.pixelSize: 14
                                    font.family: fontRegular.name
                                    width: parent.width
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        SwapImage {
                            id: swapIcon
                            Layout.alignment: Qt.AlignCenter
                            leftArrowShadowColor: isActiveRightComboBox ? rightComboBox.currentColor : "transparent"
                            rightArrowShadowColor: isActiveLeftComboBox ? leftComboBox.currentColor : "transparent"
                            sourceSize.width: item.width * 0.19 > 220 ? 220 : item.width * 0.19
                            sourceSize.height: item.width * 0.19 > 220 ? 230 : item.width * 0.2
                            enabled: isActiveLeftComboBox
                                     && isActiveRightComboBox
                                     && leftComboBox.currentAssetID !== rightComboBox.currentAssetID

                            onClicked: {
                                swapCoins()
                                exchange.update()
                            }
                        }

                        ColumnLayout {
                            Layout.preferredWidth: item.width * 0.259 //385
                            Layout.alignment: Qt.AlignVCenter
                            Layout.topMargin: 15
                            spacing: 8

                            SwapComboBox {
                                id: rightComboBox
                                Layout.fillWidth: true
                                Layout.preferredHeight: 65
                                currentIndex: -1
                                enabled: walletDexViewModel.online && acceptedLiabilityTerms

                                model: QMLSortFilterListProxyModel {
                                    id: sortedRightAssetModel
                                    source: QMLSortFilterListProxyModel {
                                        source: assetModel
                                        filterRole: "isActive"
                                        filterString: "1"
                                        filterCaseSensitivity: Qt.CaseInsensitive
                                    }
                                    filterRole: "symbol"
                                    filterString: parseAssetsModel(
                                                      swapAssetsModel.quoteAssets)
                                    filterCaseSensitivity: Qt.CaseInsensitive
                                }

                                onCurrentAssetIDChanged: {
                                    if (currentIndex !== -1) {
                                        assetModel.averageFeeForAsset(
                                                    currentAssetID)
                                    }
                                    if (rightAmount.text !== "") {
                                        rightAmount.focus = true
                                        rightAmountLocal.localAmountRefresh()
                                        rightAmount.focus = false
                                        exchange.update()
                                    }
                                }

                                onActivated: {
                                    if (currentIndex !== -1
                                            && leftComboBox.currentIndex !== -1
                                            && currentIndex !== actualIndex) {
                                        activatePair()
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 50
                                spacing: 8

                                SwapTextField {
                                    id: rightAmount
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 50 //item.height * 0.073
                                    placeholderText: "Amount"
                                    enabled: walletDexViewModel.hasSwapPair
                                    validator: RegExpValidator {
                                        regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                                    }
                                    unitValue: isActiveRightComboBox ? rightComboBox.currentSymbol : ""

                                    onTextChanged: {
                                        if (text === ".") {
                                            text = "0."
                                        }

                                        if (parseFloat(text) <= 0) {
                                            rightNotification.text
                                                    = "Amount must be greater than zero"
                                            if (!rightAmountLocal.focus
                                                    && focus) {
                                                rightAmountLocal.text = "0.0"
                                                return
                                            }
                                        }

                                        var newAmoutLocal = ApplicationViewModel.localCurrencyViewModel.convert(
                                                    rightComboBox.currentAssetID,
                                                    text)

                                        if (text === "") {
                                            newAmoutLocal = ""
                                            rightNotification.text = ""
                                        }

                                        if (!rightAmountLocal.focus) {
                                            rightAmountLocal.text = newAmoutLocal
                                        }

                                        var newLeftAmount = ""
                                        if (isBuy) {
                                            newLeftAmount = calculateTotal(
                                                        text, priceText)
                                        } else {
                                            newLeftAmount = calculateAmount(
                                                        Utils.parseCoinsToSatoshi(
                                                            text), priceText)
                                        }

                                        if (!leftAmount.focus
                                                && !leftAmountLocal.focus
                                                && choosingListView.currentIndex === -1) {
                                            leftAmountText = newLeftAmount
                                        }

                                        if (choosingListView.currentIndex === -1
                                                || calculateToggle) {
                                            calculatePreSwapAmounts(leftAmountText, text)
                                        }
                                        evaluateBalances()
                                    }
                                }

                                SwapTextField {
                                    id: rightAmountLocal
                                    Layout.preferredWidth: parent.width * 0.3
                                    Layout.preferredHeight: 50 //item.height * 0.073 // 60
                                    enabled: walletDexViewModel.hasSwapPair
                                    placeholderText: ""
                                    validator: RegExpValidator {
                                        regExp: /^(([1-9]{1,})+[0-9]*(\.?[0-9]{0,2})$|0?(\.[0-9]{0,2}))$/gm
                                    }
                                    unitValue: ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol
                                    symbolOnLeft: true
                                    onTextChanged: {
                                        if (text === ".") {
                                            rightAmountLocal.text = "0."
                                        }

                                        var newAmoutCoin = ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                                    rightComboBox.currentAssetID,
                                                    text)
                                        if (text === "") {
                                            newAmoutCoin = ""
                                        }

                                        if (!rightAmount.focus && focus
                                                && choosingListView.currentIndex === -1) {
                                            rightAmount.text = newAmoutCoin
                                        }
                                    }
                                    function localAmountRefresh() {
                                        if (rightAmountText == "") {
                                            text = ""
                                            return
                                        }
                                        text = ApplicationViewModel.localCurrencyViewModel.convert(
                                                    rightComboBox.currentAssetID,
                                                    rightAmountText)
                                    }
                                }
                            }

                            Item {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.fillWidth: true
                                Layout.preferredHeight: 35

                                Text {
                                    id: rightNotification
                                    visible: text.length > 0
                                    anchors.leftMargin: 14
                                    anchors.rightMargin: 14
                                    anchors.left: parent.left
                                    color: SkinColors.transactionItemSent
                                    font.pixelSize: 14
                                    font.family: fontRegular.name
                                    width: parent.width
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    Item {
                        Layout.preferredHeight: 46
                        Layout.fillWidth: true

                        ChoosingListView {
                            id: choosingListView
                            property double calculatedLeftMin: 0.0
                            property double calculatedRightMin: 0.0
                            height: 32 // item.height /2 * 0.07
                            width: item.width * 0.22 //290
                            anchors.verticalCenter: parent.verticalCenter
                            model: ["MIN", "HALF", "MAX"]
                            radius: 10
                            textItemPixelSize: 13
                            currentIndex: -1
                            visible: isActiveLeftComboBox
                                     && isActiveRightComboBox
                            enabled: isActiveLeftComboBox
                                     && isActiveRightComboBox
//                                     && priceText !== ""

                            onCurrentOptionChanged: preCalculateToggleAmount()

                            function preCalculateToggleAmount() {
                                if (currentOption == "") {
                                    return
                                }
                                calculateToggle = true
                                var newAmount = 0.0

                                var proposedRightAmount = 0.0
                                var minRightAmount = 0.0
                                var allAvailableBalance = 0.0

                                var newRightAmountStr = ""
                                var newAmountStr = ""
                                if (currentOption === "MIN") {
                                    // calculate min amount for left side
                                    if (isBuy && hasEnoughBalance(
                                                10000,
                                                (isBuy ? quoteBalance : baseBalance))) {
                                        newAmount = 10000
                                    } else if (!isBuy && hasEnoughBalance(
                                                   10,
                                                   (isBuy ? quoteBalance : baseBalance))) {
                                        newAmount = 10
                                    } else {
                                        newAmount = leftComboBox.currentMinLndCapacity
                                    }

                                    // check if min value from left is good to swap for right side
                                    // 1) calculate right amount according to min left amount
                                    if (isBuy) {
                                        proposedRightAmount = calculateAmount(
                                                    newAmount, priceText)
                                    } else {
                                        proposedRightAmount = calculateTotal(
                                                    Utils.formatBalance(
                                                        newAmount), priceText)
                                    }

                                    let proposedRightAmountValue = Utils.parseCoinsToSatoshi(
                                            proposedRightAmount)

                                    var hasCanReceiveBalance = hasEnoughBalance(
                                                proposedRightAmount,
                                                (isBuy ? baseRemoteBalance : quoteRemoteBalance))

                                    // 2) calculate min amount for right side
                                    var proposedRightAmountLocalText = ApplicationViewModel.localCurrencyViewModel.convert(
                                                rightComboBox.currentAssetID,
                                                proposedRightAmount)

                                    if (hasCanReceiveBalance && isBuy
                                            && hasEnoughBalance(
                                                10,
                                                (isBuy ? baseRemoteBalance : quoteRemoteBalance))
                                            && proposedRightAmount.length > 0
                                            && proposedRightAmountValue < 10) {
                                        minRightAmount = 10
                                    } else if (hasCanReceiveBalance && !isBuy
                                               && hasEnoughBalance(
                                                   10000,
                                                   (isBuy ? baseRemoteBalance : quoteRemoteBalance))
                                               && proposedRightAmount.length > 0
                                               && proposedRightAmountValue < 10000) {
                                        minRightAmount = 10000
                                    } else {
                                        if (isNaN(proposedRightAmountLocalText)
                                                || minLocalRentalAmount > parseFloat(
                                                    proposedRightAmountLocalText)) {

                                            minRightAmount = Utils.parseCoinsToSatoshi(
                                                        ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                                            rightComboBox.currentAssetID,
                                                            minLocalRentalAmount))
                                        }
                                    }
                                    // 3) check if min value from left is good to swap
                                    if (proposedRightAmountValue < minRightAmount) {
                                        //if min right amount is bigger than equal proposed left amount then calculate left amount for it
                                        if (isBuy) {
                                            newAmountStr = calculateTotal(
                                                        Utils.formatBalance(
                                                            minRightAmount),
                                                        priceText)
                                        } else {
                                            newAmountStr = calculateAmount(
                                                        minRightAmount,
                                                        priceText)
                                        }
                                        calculatedLeftMin = newAmount = Utils.parseCoinsToSatoshi(
                                                    newAmountStr)
                                        calculatedRightMin = minRightAmount
                                    } else {
                                        calculatedLeftMin = newAmount
                                        calculatedRightMin = proposedRightAmountValue
                                    }
                                } else if (currentOption === "MAX") {
                                    allAvailableBalance = leftAvailableBalance
                                } else {
                                    allAvailableBalance = leftAvailableBalance * 0.5
                                }
                                if (currentOption === "MAX"
                                        || currentOption === "HALF") {
                                    newAmount = Math.floor(
                                                allAvailableBalance / (1 + (isBuy ? tradingPairInfo.buyFeePercent : tradingPairInfo.sellFeePercent)))
                                }
                                newAmountStr = Utils.formatBalance(newAmount)

                                if (isBuy) {
                                    newRightAmountStr = calculateAmount(
                                                newAmount, priceText)
                                } else {
                                    newRightAmountStr = calculateTotal(
                                                newAmountStr, priceText)
                                }

                                calculatePreSwapAmounts(newAmountStr,
                                                        newRightAmountStr)
                            }

                            function calculateToggleAmount() {

                                if (currentOption == "") {
                                    return
                                }
                                calculateToggle = false
                                var newAmount = 0.0
                                var allAvailableBalance = 0.0
                                var newAmountStr = ""

                                if (currentOption === "MIN") {
                                    newAmount = calculatedLeftMin
                                    calculatedLeftMin = 0.0
                                } else if (currentOption === "MAX") {
                                    allAvailableBalance = leftAvailableBalance
                                } else {
                                    allAvailableBalance = leftAvailableBalance * 0.5
                                }
                                if (currentOption === "MAX"
                                        || currentOption === "HALF") {

                                    allAvailableBalance -= (expectedTotalRentalFee + sendReserve)

                                    newAmount = allAvailableBalance > 0 ? Math.floor(allAvailableBalance / (1 + (isBuy ? tradingPairInfo.buyFeePercent : tradingPairInfo.sellFeePercent))) : 0
                                }

                                newAmountStr = Utils.formatBalance(newAmount)
                                leftAmountText = newAmountStr
                            }
                        }

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            height: parent.height
                            width: exchange.contentWidth + 60
                            radius: 23
                            color: SkinColors.swapPageSecondaryBackground
                            visible: isActiveLeftComboBox
                                     && isActiveRightComboBox && marketHasOrders && !commonNotification.visible

                            Text {
                                id: exchange
                                anchors.centerIn: parent
                                font.family: fontRegular.name
                                font.pixelSize: 16
                                color: SkinColors.mainText

                                function update() {
                                    text = '1 %1 = <font color="%2">%3 %4</font>'.arg(
                                                leftComboBox.currentSymbol).arg(
                                                rightComboBox.currentColor).arg(
                                                isBuy ? calculateAmount(
                                                            100000000,
                                                            priceText) : calculateTotal(
                                                            "1",
                                                            priceText)).arg(
                                                rightComboBox.currentSymbol)
                                }
                            }
                        }

                        Item {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: item.width / 4
                            height: 46

                            Text {
                                id: commonNotification
                                anchors.centerIn: parent
                                color: SkinColors.transactionItemSent
                                font.pixelSize: 14
                                font.family: fontRegular.name
                                visible: text.length > 0
                            }
                        }
                    }
                }

                RowLayout {
                    id: cancelItem
                    anchors.fill: parent
                    anchors.left: parent.left
                    anchors.leftMargin: 32
                    spacing: 12

                    Image {
                        source: "qrc:/images/cross.svg"
                        sourceSize: Qt.size(24, 24)

                        PointingCursorMouseArea {
                            anchors.fill: parent
                            onClicked: item.state = "default"
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Cancel"
                        color: SkinColors.mainText
                        font.family: fontRegular.name
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }
                }
            }

            Item {
                id: footerItem
                Layout.preferredHeight: item.height * 0.46
                Layout.fillWidth: true

                GradientBackgroundComponent {
                    anchors.fill: parent
                    gradientOpacity: 0.5
                    firstColor: SkinColors.swapPageBackgroundFooterFirst
                    secondColor: SkinColors.swapPageBackgroundFooterSecond
                    thirdColor: SkinColors.swapPageBackgroundFooterThird
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.centerIn: parent
                    anchors.topMargin: item.height * 0.05
                    anchors.bottomMargin: parent.height * 0.08 //55
                    visible: walletDexViewModel.hasSwapPair
                    spacing: 32

                    Item {
                        Layout.preferredHeight: item.height * 0.2
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop

                        RowLayout {
                            id: bottomContent
                            anchors.fill: parent
                            spacing: item.width / 15

                            RowLayout {
                                Layout.preferredWidth: item.width / 3
                                Layout.minimumWidth: item.width / 3
                                Layout.maximumWidth: item.width / 3
                                spacing: 35

                                Item {
                                    Layout.fillWidth: true
                                }

                                ColumnLayout {
                                    spacing: 10

                                    Text {
                                        Layout.alignment: Qt.AlignRight
                                        color: SkinColors.mainText
                                        text: isActiveLeftComboBox ? "%1 %2".arg(
                                                                         leftComboBox.currentSymbol).arg(
                                                                         leftComboBox.currentName) : ""
                                        font.family: fontRegular.name
                                        font.pixelSize: 15
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.alignment: Qt.AlignRight
                                        Layout.fillHeight: true
                                        spacing: 3
                                        visible: leftAmountText !== ""

                                        Text {
                                            Layout.fillWidth: true
                                            horizontalAlignment: Text.AlignRight
                                            color: SkinColors.mainText
                                            text: leftAmountText
                                            font.family: fontRegular.name
                                            font.pixelSize: 20
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            horizontalAlignment: Text.AlignRight
                                            elide: Text.ElideRight
                                            color: SkinColors.mainText
                                            text: "%1 %2".arg(
                                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                                      leftAmountLocalText)
                                            font.family: fontRegular.name
                                            font.pixelSize: 15
                                        }
                                    }
                                }

                                GlowImage {
                                    Layout.alignment: Qt.AlignRight
                                    Layout.preferredHeight: 70
                                    Layout.preferredWidth: 80
                                    sourceSize: Qt.size(70, 80)
                                    color: isActiveLeftComboBox ? leftComboBox.currentColor : "transparent"
                                    source: isActiveLeftComboBox
                                            && leftComboBox.currentName
                                            !== "" ? "qrc:/images/ICON_%1.svg".arg(
                                                         leftComboBox.currentName) : ""
                                }
                            }

                            Item {
                                Layout.preferredWidth: item.width / 6
                                Layout.fillHeight: true

                                Image {
                                    anchors.centerIn: parent
                                    sourceSize: Qt.size(95, 30)
                                    source: "qrc:/images/swapWhiteArrows.svg"
                                }
                            }

                            RowLayout {
                                Layout.preferredWidth: item.width / 3
                                Layout.minimumWidth: item.width / 3
                                Layout.maximumWidth: item.width / 3
                                spacing: 35

                                GlowImage {
                                    Layout.preferredHeight: 70
                                    Layout.preferredWidth: 80
                                    sourceSize: Qt.size(70, 80)
                                    color: isActiveRightComboBox ? rightComboBox.currentColor : ""
                                    source: isActiveRightComboBox
                                            && rightComboBox.currentName
                                            !== "" ? "qrc:/images/ICON_%1.svg".arg(
                                                         rightComboBox.currentName) : ""
                                }

                                ColumnLayout {
                                    spacing: 10

                                    Text {
                                        Layout.fillWidth: true
                                        color: SkinColors.mainText
                                        text: isActiveRightComboBox ? "%1 %2".arg(
                                                                          rightComboBox.currentSymbol).arg(rightComboBox.currentName) : ""
                                        font.family: fontRegular.name
                                        font.pixelSize: 15
                                    }

                                    ColumnLayout {
                                        spacing: 3
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        visible: rightAmountText !== ""

                                        Text {
                                            Layout.fillWidth: true
                                            color: SkinColors.mainText
                                            text: rightAmountText
                                            font.family: fontRegular.name
                                            font.pixelSize: 20
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            color: SkinColors.mainText
                                            text: "%1 %2".arg(
                                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                                      rightAmountLocalText)
                                            font.family: fontRegular.name
                                            font.pixelSize: 15
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        id: confirmInfoView
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 15

                            SwapSeparatedLine {
                                id: gradientHorizontalLine1
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                opacity: 0.0
                            }

                            ColumnLayout {
                                id: willReceiveInfo
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 8
                                scale: 0.0
                                opacity: 0.0

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    color: SkinColors.mainText
                                    font.family: fontRegular.name
                                    font.pixelSize: 22
                                    text: isActiveRightComboBox ? "You will receive %1 %2".arg(
                                                                      Utils.formatBalance(
                                                                          receiveAmount)).arg(
                                                                      rightComboBox.currentSymbol) : ""
                                    font.weight: Font.DemiBold
                                }

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    color: SkinColors.mainText
                                    font.family: fontRegular.name
                                    font.pixelSize: 15
                                    font.letterSpacing: 0.2
                                    text: isActiveRightComboBox ? "%1 %2".arg(
                                                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(ApplicationViewModel.localCurrencyViewModel.convert(rightComboBox.currentAssetID, Utils.formatBalance(receiveAmount))) : ""
                                }
                            }

                            SwapSeparatedLine {
                                id: gradientHorizontalLine2
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                opacity: 0.0
                            }

                            RowLayout {
                                id: bottomInfo
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 0

                                RowLayout {
                                    id: leftInfo
                                    Layout.preferredWidth: item.width / 2
                                    spacing: 20
                                    opacity: 0.0

                                    Item {
                                        Layout.fillWidth: true
                                    }

                                    Image {
                                        visible: false
                                        Layout.topMargin: 15
                                        sourceSize: Qt.size(62, 61)
                                        source: "qrc:/images/ESTIMATE.svg"
                                    }

                                    ColumnLayout {
                                        spacing: 0

                                        Text {
                                            color: SkinColors.secondaryText
                                            text: "Total fee" //"Estimated time"
                                            font.capitalization: Font.AllUppercase
                                            font.family: fontRegular.name
                                            font.pixelSize: 14
                                            font.letterSpacing: 0.2
                                        }

                                        Text {
                                            color: SkinColors.mainText
                                            text: "%1 %2".arg(
                                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                                      totalFeeStr) //"20 minutes"
                                            font.family: fontRegular.name
                                            font.pixelSize: 20
                                        }
                                    }
                                }

                                SwapSeparatedLine {
                                    id: gradientVerticalLine
                                    Layout.preferredWidth: 1
                                    Layout.fillHeight: true
                                    Layout.alignment: Qt.AlignHCenter
                                    isHorizontal: false
                                    opacity: 0.0
                                }

                                ColumnLayout {
                                    id: rightInfo
                                    Layout.preferredWidth: item.width / 2
                                    spacing: item.height * 0.02
                                    opacity: 0.0

                                    Repeater {
                                        model: [{
                                                "header": "ON CHAIN FEES (Open + Close)",
                                                "fee": onChainRentalFeeStr
                                            }, {
                                                "header": "RENTAL FEE",
                                                "fee": rentalFeeStr
                                            }, {
                                                "header": "PLACE ORDER FEE",
                                                "fee": orderFeeStr
                                            }]

                                        delegate: Column {
                                            spacing: 0
                                            visible: modelData.fee !== "0.00"
                                                     && modelData.fee !== ""

                                            Text {
                                                text: modelData.header
                                                color: SkinColors.secondaryText
                                                font.family: fontRegular.name
                                                font.pixelSize: 14
                                                font.letterSpacing: 0.2
                                            }

                                            Text {
                                                color: SkinColors.mainText
                                                text: "%1 %2".arg(
                                                          ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                                          modelData.fee)
                                                font.family: fontRegular.name
                                                font.pixelSize: 20
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    GradientButton {
                        id: actionButton
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                        Layout.preferredHeight: item.height / 2 * 0.14
                        Layout.maximumHeight: 50
                        Layout.preferredWidth: item.width / 5
                        borderColor: SkinColors.popupFieldBorder
                        buttonGradientLeftHoveredColor: isActiveLeftComboBox ? leftComboBox.currentColor : "transparent"
                        buttonGradientRightHoveredColor: isActiveRightComboBox ? rightComboBox.currentColor : "transparent"
                        font.family: fontRegular.name
                        font.pixelSize: 16
                        enabled: walletDexViewModel.hasSwapPair
                                 && isActiveLeftComboBox && isActiveRightComboBox
                                 && !commonNotification.visible && !leftNotification.visible
                                 && !rightNotification.visible && leftAmountText !== ""
                        && (lightningBaseViewModel.stateModel && lightningQuoteViewModel.stateModel
                                            && checkPaymentNodeState(lightningBaseViewModel.stateModel)
                                            && checkPaymentNodeState(lightningQuoteViewModel.stateModel))
                                    && preSwapAmountsCalculated
                                 && !swapInProcess

                        text: item.state == "default" ? "Confirm" : "Exchange"
                        radius: 10
                        onClicked: {
                            if (item.state == "default") {
                                var leftCapacity = Utils.parseCoinsToSatoshi(
                                            leftAmountText)
                                var rightCapacity = Utils.parseCoinsToSatoshi(
                                            rightAmountText)

                                receiveAmount = rightCapacity
                                swapViewModel.evaluateChannels(
                                            leftComboBox.currentAssetID,
                                            rightComboBox.currentAssetID,
                                            leftCapacity, rightCapacity, isBuy,
                                            orderFee, sendReserve,
                                            receiveReserve)
                            } else if (item.state == "confirm") {
                                swapViewModel.executeSwap(
                                            swapRequest,
                                            recommendedNetworkFeeRateForLeftAsset)
                                item.state = "in progress"
                            }
                        }
                    }
                }
            }
        }

        PendingExchangeSwapView {
            id: exchangeInProgressView
            Layout.fillWidth: true
            Layout.fillHeight: true
            receiveAssetID: rightComboBox.currentAssetID
            receiveAssetName: rightComboBox.currentName
            receiveAssetColor: rightComboBox.currentColor
            receiveAssetSymbol: settings.state === "success" ? settings.receiveAssetSymbol : rightComboBox.currentSymbol
            willReceveAmount: settings.state === "success" ? settings.receivedAmount : receiveAmount
            currentState: exchangeState()
            exchangeSt: item.state
            swapErrorMsg: settings.errMsg
            opacity: settings.state === "success" || settings.state === "failed"
                     || item.state === "in progress" ? 1.0 : 0.0
            onCloseButtonClicked: item.state = "default"
        }
    }

    Settings {
        id: settings
        category: "SwapPage"
        property string state: "default"

        property double receivedAmount: 0
        property string receiveAssetSymbol: ""

        property string errMsg: ""
    }

    function exchangeState() {
        switch (item.state) {
        case "in progress":
            if (swapViewModel.sendChannelState) {
                return "Opening %1 Channels".arg(leftComboBox.currentSymbol)
            } else if (swapViewModel.recvChannelState) {
                return "Opening %1 Channels".arg(rightComboBox.currentSymbol)
            }
            return "Exchange in progress!"
        case "success":
            return "Exchange success!"
        case "failed":
            return "Exchange failed!"
        default:
            return ""
        }
    }

    function updateBestPrice() {
        commonNotification.text = ""
        if (!walletDexViewModel.hasSwapPair) {
            return
        }

        var bestPrice = calculateBestPrice(isBuy)
        if (bestPrice > 0) {
            priceText = Utils.formatBalance(bestPrice)
            leftAmount.enabled = true
            rightAmount.enabled = true
            leftAmountLocal.enabled = true
            rightAmountLocal.enabled = true
        } else {
            priceText = ""
            commonNotification.text = "No market price available"
            leftAmountText = ""
            rightAmountText = ""
            leftAmountLocalText = ""
            rightAmountLocalText = ""
            leftAmount.enabled = false
            rightAmount.enabled = false
            leftAmountLocal.enabled = false
            rightAmountLocal.enabled = false
            leftNotification.text = ""
            rightNotification.text = ""
        }
    }

    function resetComboBoxes() {
        if (acceptedLiabilityTerms && leftComboBox.currentIndex === -1
                && rightComboBox.currentIndex === -1
                && swapAssetsModel.rowCount() > 0) {
            if (swapAssetsModel.hasPair(swapAssetsModel.lastBaseAssetID(),
                                        swapAssetsModel.lastQuoteAssetID())) {
                changeComboBoxPair(swapAssetsModel.lastBaseAssetID(),
                                   swapAssetsModel.lastQuoteAssetID())
                activatePair()
            }
        }
    }

    function activatePair() {

        if (leftComboBox.currentAssetID !== -1
                && rightComboBox.currentAssetID !== -1) {
            var baseAssetID = -1
            var quoteAssetID = -1
            var baseAssetSymbol = ""
            var quoteAssetSymbol = ""

            if (isBuy) {
                baseAssetID = rightComboBox.currentAssetID
                quoteAssetID = leftComboBox.currentAssetID
                baseAssetSymbol = rightComboBox.currentSymbol
                quoteAssetSymbol = leftComboBox.currentSymbol
            } else {
                baseAssetID = leftComboBox.currentAssetID
                quoteAssetID = rightComboBox.currentAssetID
                baseAssetSymbol = leftComboBox.currentSymbol
                quoteAssetSymbol = rightComboBox.currentSymbol
            }

            if (walletDexViewModel.baseAssetID === baseAssetID
                    && walletDexViewModel.quoteAssetID === quoteAssetID) {
                updateBestPrice();
                return
            }

            if(ApplicationViewModel.tradingBotModel.isActive) {
                ApplicationViewModel.tradingBotModel.stop()
            }

            if (!walletDexViewModel.hasSwapPair
                    || walletDexViewModel.stateModel.ownOrderBookListModel.rowCount(
                        ) === 0) {
                ApplicationViewModel.paymentNodeViewModel.changeSwapAssets(
                            baseAssetID, quoteAssetID,
                            baseAssetSymbol + "_" + quoteAssetSymbol)
                changeSwapAssetsInProgress = true
            } else {
                var popup = openDialog(dexNotificationComponent, {
                                           "walletDexViewModel": walletDexViewModel,
                                           "baseAssetID": baseAssetID,
                                           "quoteAssetID": quoteAssetID,
                                           "baseSymbol": baseAssetSymbol,
                                           "quoteSymbol": quoteAssetSymbol
                                       })

                popup.rejected.connect(function () {
                    if (leftComboBox.actualIndex !== -1) {
                        leftComboBox.currentIndex = leftComboBox.actualIndex
                        leftComboBox.activated(leftComboBox.currentIndex)
                    }
                    if (rightComboBox.actualIndex !== -1) {
                        rightComboBox.currentIndex = rightComboBox.actualIndex
                        rightComboBox.activated(rightComboBox.currentIndex)
                    }
                    calculatePreSwapAmounts(leftAmountText, rightAmountText)
                })
            }
        }
    }

    function swapCoins() {
        if(!changeSwapAssetsInProgress){
            isBuy = !isBuy
            var currentLeftIndex = leftComboBox.currentIndex
            var currentRightIndex = rightComboBox.currentIndex

            var leftModel = leftComboBox.model
            leftComboBox.model = rightComboBox.model
            rightComboBox.model = leftModel

            leftComboBox.currentIndex = currentRightIndex
            leftComboBox.accepted(currentRightIndex)

            rightComboBox.currentIndex = currentLeftIndex
            rightComboBox.accepted(currentLeftIndex)

            calculatePreSwapAmounts(leftAmountText, rightAmountText)

            console.log("Changed to %1 %2 / %3".arg(isBuy ? "Buy" : "Sell").arg(isBuy ? walletDexViewModel.quoteAssetID : walletDexViewModel.baseAssetID ).arg(isBuy ? walletDexViewModel.baseAssetID : walletDexViewModel.quoteAssetID))
        }
        else {
            //console.log("Can not swap coins till change trade pair in progress!")
        }
    }

    function changeComboBoxPair(baseAssetID, quoteAssetID) {
        if (!walletDexViewModel.hasSwapPair && !swapAssetsModel.hasPair(
                    baseAssetID, quoteAssetID)) {
            leftComboBox.currentIndex = -1
            leftComboBox.accepted(leftComboBox.currentIndex)
            rightComboBox.currentIndex = -1
            rightComboBox.accepted(rightComboBox.currentIndex)
            leftAmountText = ""
            rightAmountText = ""
            leftAmountLocalText = ""
            rightAmountLocalText = ""
        } else {
            var leftSideModel = !isBuy ? sortedLeftAssetModel : sortedRightAssetModel
            var rightSideModel = !isBuy ? sortedRightAssetModel : sortedLeftAssetModel

            for (var i = 0; i < leftSideModel.count; i++) {
                if (leftSideModel.get(i).id === baseAssetID) {
                    leftComboBox.currentIndex = i
                    leftComboBox.accepted(leftComboBox.currentIndex)
                }
            }

            for (i = 0; i < rightSideModel.count; i++) {
                if (rightSideModel.get(i).id === quoteAssetID) {
                    rightComboBox.currentIndex = i
                    rightComboBox.accepted(rightComboBox.currentIndex)
                }
            }
            calculatePreSwapAmounts(leftAmountText, rightAmountText)
        }
    }

    function evaluateBalances() {
        evaluateLeftBalance()
        evaluateRightBalance()
    }

    function evaluateLeftBalance() {
        if (leftAmountText === ".") {
            leftAmountText = "0."
        }

        let leftAmountValue = Utils.parseCoinsToSatoshi(leftAmountText)

        var hasCanSendBalance = hasEnoughBalance(
                    leftAmountValue + orderFee + expectedTotalRentalFee,
                    (isBuy ? quoteBalance : baseBalance))

        if (hasCanSendBalance) {
            if (!isBuy && leftAmountText.length > 0 && leftAmountValue < 10) {
                leftNotification.text = "Amount must be at least 10 satoshis"
                return
            }
            if (isBuy && leftAmountText.length > 0 && leftAmountValue < 10000) {
                leftNotification.text = "Amount must be at least 10000 satoshis"
                return
            }
        } else {
            var proposed = leftAmountValue + sendReserve + orderFee + expectedTotalRentalFee

            if (leftComboBox.currentMinLndCapacity > proposed) {

                if (leftAmountValue === 0.0) {
                    leftNotification.text = "Not enough balance to cover swap fees."
                    return
                }

                leftNotification.text
                        = "Amount is less than minimum capacity to open channel (%1%2).".arg(
                            ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                            ApplicationViewModel.localCurrencyViewModel.convert(
                                leftComboBox.currentAssetID,
                                Utils.formatBalance(
                                    leftComboBox.currentMinLndCapacity)))
                return
            }

            if (!hasEnoughBalance(proposed, leftAvailableBalance)) {

                leftNotification.text = "Not enough %1 %2 (reserve + fee).".arg(
                            Utils.formatBalance(
                                proposed - leftAvailableBalance)).arg(
                            leftComboBox.currentSymbol)
                return
            }
        }

        leftNotification.text = ""
    }

    function evaluateRightBalance() {
        let rightAmountValue = Utils.parseCoinsToSatoshi(rightAmountText)

        var hasCanReceiveBalance = hasEnoughBalance(
                    rightAmountValue,
                    (isBuy ? baseRemoteBalance : quoteRemoteBalance))

        if (hasCanReceiveBalance) {
            if (isBuy && rightAmountText.length > 0 && rightAmountValue < 10) {
                rightNotification.text = "Amount must be at least 10 satoshis"
                return
            }
            if (!isBuy && rightAmountText.length > 0
                    && rightAmountValue < 10000) {
                rightNotification.text = "Amount must be at least 10000 satoshis"
                return
            }
        } else {
            if (isNaN(rightAmountLocalText)
                    || minLocalRentalAmount > parseFloat(
                        rightAmountLocalText)) {
                rightNotification.text
                        = "Amount is less than minimum capacity to open channel ($5)."
                return
            }
            if (maxLocalRentalAmount < parseFloat(rightAmountLocalText)) {
                rightNotification.text
                        = "Amount is more than maximum capacity to open channel ($10000)."
                return
            }
        }

        rightNotification.text = ""
    }

    function updateBalancesModel() {
        if (walletDexViewModel.hasSwapPair) {
            channelBalancesViewModel.changeTraidingPair(
                        walletDexViewModel.baseAssetID,
                        walletDexViewModel.quoteAssetID)
        } else {
            channelBalancesViewModel.reset()
        }
    }

    function calculateTotalFee(onChainStr, onChainRentalStr, rentalFeeStr, orderStr) {
        var onChainLocal = 0.0
        var onChainRentaLocal = 0.0
        var rentalFeeLocal = 0.0
        var orderLocal = 0.0

        if (!isNaN(onChainRentalStr)) {
            onChainLocal = parseFloat(onChainRentalStr)
        }
        if (!isNaN(rentalFeeStr)) {
            rentalFeeLocal = parseFloat(rentalFeeStr)
        }
        if (!isNaN(orderStr)) {
            orderLocal = parseFloat(orderStr)
        }

        var result = onChainLocal + onChainRentaLocal + rentalFeeLocal + orderLocal
        return result === 0 ? "< 0.01" : result.toFixed(2)
    }

    function calculatePreSwapAmounts(leftAmountTx, rightAmountTx) {
        preSwapAmountsCalculated = false
        expectedTotalRentalFee = 0.0
        sendReserve = 0.0
        receiveReserve = 0.0

        var rightAmountTxLocal = ApplicationViewModel.localCurrencyViewModel.convert(
                    rightComboBox.currentAssetID, rightAmountTx)
        var hasCanReceiveBalance = hasEnoughBalance(
                    Utils.parseCoinsToSatoshi(rightAmountTx),
                    (isBuy ? baseRemoteBalance : quoteRemoteBalance))

        var needToRentChannel = !hasCanReceiveBalance && isActiveLeftComboBox && isActiveRightComboBox && !isNaN(
                        rightAmountTxLocal) && minLocalRentalAmount < parseFloat(rightAmountTxLocal)

        if (leftAmountTx !== "" && parseFloat(leftAmountTx) > 0 && rightAmountTx !== "" && parseFloat(rightAmountTx) > 0){
            var leftCapacity = Utils.parseCoinsToSatoshi(leftAmountTx)
            var rightCapacity = Utils.parseCoinsToSatoshi(rightAmountTx)
            swapViewModel.calculatePreSwapAmounts(leftComboBox.currentAssetID,
                                                  rightComboBox.currentAssetID,
                        leftCapacity, rightCapacity,
                        recommendedNetworkFeeRateForLeftAsset,
                        recommendedNetworkFeeRateForRightAsset,
                        needToRentChannel)
        }
    }
}
