import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0

import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Flickable {
    id: flickable
    contentHeight: height < mainDexViewContentHeight
                   && walletDexViewModel.hasSwapPair ? mainDexViewContentHeight : height
    boundsBehavior: Flickable.StopAtBounds
    clip: true

    ScrollBar.vertical: ScrollBar {
        anchors.top: flickable.top
        anchors.right: flickable.right
        anchors.bottom: flickable.bottom
        size: flickable.height / flickable.contentHeight
    }

    property WalletDexViewModel walletDexViewModel

    property double totalBuyOrdersAmount: walletDexViewModel.hasSwapPair ? walletDexViewModel.stateModel.ownOrderBookListModel.buyTotalAmount : 0
    property double totalSellOrdersAmount: walletDexViewModel.hasSwapPair ? walletDexViewModel.stateModel.ownOrderBookListModel.sellTotalAmount : 0

    property double baseBalance: walletDexViewModel.hasSwapPair ? Math.max(
                                                                      channelsBalance.baseBalance.localBalance - totalSellOrdersAmount,
                                                                      0) : 0
    property double quoteBalance: walletDexViewModel.hasSwapPair ? Math.max(
                                                                       channelsBalance.quoteBalance.localBalance - totalBuyOrdersAmount,
                                                                       0) : 0

    property double totalAllBuyOrders: walletDexViewModel.hasSwapPair ? walletDexViewModel.stateModel.ownOrderBookListModel.totalBuy : 0
    property double totalAllSellOrders: walletDexViewModel.hasSwapPair ? walletDexViewModel.stateModel.ownOrderBookListModel.totalSell : 0

    property double baseRemoteBalance: walletDexViewModel.hasSwapPair ? Math.max(
                                                                            channelsBalance.baseBalance.remoteBalance - totalAllBuyOrders,
                                                                            0) : 0
    property double quoteRemoteBalance: walletDexViewModel.hasSwapPair ? Math.max(
                                                                             channelsBalance.quoteBalance.remoteBalance - totalAllSellOrders,
                                                                             0) : 0

    property var tradingPairInfo: walletDexViewModel.tradingPairInfo
    property double orderFee: 0.0
    property var channelsBalance: channelBalancesViewModel.channelsBalance
    property int mainDexViewContentHeight: 870
    property bool rentalBase: false

    onBaseBalanceChanged: {
        evaluateBalances()
    }

    onQuoteBalanceChanged: {
        evaluateBalances()
    }

    onBaseRemoteBalanceChanged: {
        evaluateBalances()
    }

    onQuoteRemoteBalanceChanged: {
        evaluateBalances()
    }

    property string quoteLocalBalanceStr: walletDexViewModel.hasSwapPair ? ApplicationViewModel.localCurrencyViewModel.convert(walletDexViewModel.quoteAssetID, Utils.formatBalance(quoteBalance)) : ""
    property string baseLocalBalanceStr: walletDexViewModel.hasSwapPair ? ApplicationViewModel.localCurrencyViewModel.convert(walletDexViewModel.baseAssetID, Utils.formatBalance(baseBalance)) : ""

    property string quoteRemoteBalanceStr: walletDexViewModel.hasSwapPair ? ApplicationViewModel.localCurrencyViewModel.convert(walletDexViewModel.quoteAssetID, Utils.formatBalance(quoteRemoteBalance)) : ""
    property string baseRemoteBalanceStr: walletDexViewModel.hasSwapPair ? ApplicationViewModel.localCurrencyViewModel.convert(walletDexViewModel.baseAssetID, Utils.formatBalance(baseRemoteBalance)) : ""

    property bool isBuy: buySellView.currentIndex === 0
    property bool isLimit: limitMarketView.currentIndex === 0

    onIsBuyChanged: {
        updateBestPrice(!isLimit)
        orderFee = calculatePlaceOrderFee(
                    Utils.parseCoinsToSatoshi(isBuy ? totalText : amountText),
                    isBuy ? tradingPairInfo.buyFeePercent : tradingPairInfo.sellFeePercent)
        evaluateBalances()
    }

    onIsLimitChanged: {
        updateBestPrice(true)
        orderFee = calculatePlaceOrderFee(
                    Utils.parseCoinsToSatoshi(isBuy ? totalText : amountText),
                    isBuy ? tradingPairInfo.buyFeePercent : tradingPairInfo.sellFeePercent)
        evaluateBalances()
    }

    property alias priceText: priceField.text
    property alias amountText: amountTextField.text
    property alias totalText: totalField.text
    property alias buySellViewIndex: buySellView.currentIndex

    property alias amountFocus: amountTextField.focus
    property alias totalFocus: totalField.focus

    Settings {
        id: settings
        category: "Dex"
    }

    Connections {
        target: menuStackLayout
        function onCurrentIndexChanged() {
            useCoinsForRemoteBalance = settings.value(
                        "useCoinsForRemoteBalance") === "Coin"
        }
    }

    property bool useCoinsForRemoteBalance: settings.value(
                                                "useCoinsForRemoteBalance") === "Coin"

    Connections {
        enabled: walletDexViewModel.hasSwapPair
        target: walletDexViewModel
                && walletDexViewModel.stateModel ? (isBuy ? walletDexViewModel.stateModel.sellOrdersListModel : walletDexViewModel.stateModel.buyOrdersListModel) : null
        function onBestPriceChanged() {
            updateBestPrice()
        }
    }

    Connections {
        target: mainPage
        function onAcceptedLiabilityTermsChanged() {
            resetTradePairModel()
        }
    }

    Connections {
        target: walletDexViewModel
        function onPlaceOrderFailed(errorText) {
            var popup = openDialog(dexPlaceOrderNotification, {
                                       "errorMessage": errorText
                                   })
        }
        function onSwapAssetsChanged() {
            priceText = ""
            if (walletDexViewModel.hasSwapPair) {
                if (comboBox.currentIndex !== swapAssetsModel.lastTradingPairIndex(
                            )) {
                    changePair()
                }
                channelBalancesViewModel.changeTraidingPair(
                            walletDexViewModel.baseAssetID,
                            walletDexViewModel.quoteAssetID)
            } else {
                channelBalancesViewModel.reset()
            }
        }
    }

    Component {
        id: rentalNotificationComponent

        ConfirmationPopup {
            width: 480
            height: 280
            cancelButton.text: "Cancel"
            confirmButton.text: "Confirm"
            onConfirmClicked: {
                close()
                increaseCanReceiveBalance(rentalBase)
            }
            onCancelClicked: close()
        }
    }

    WalletDexChannelBalanceViewModel {
        id: channelBalancesViewModel

        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    PaymentNodeViewModel {
        id: lightningBaseViewModel
        currentAssetID: walletDexViewModel.baseAssetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    Connections {
        target: lightningBaseViewModel.stateModel
        function onNodeStatusChanged() {
            if (checkPaymentNodeState(lightningBaseViewModel.stateModel)
                    && checkPaymentNodeState(
                        lightningQuoteViewModel.stateModel)) {
                tryOpenRental()
            }
        }
    }

    PaymentNodeViewModel {
        id: lightningQuoteViewModel
        currentAssetID: walletDexViewModel.quoteAssetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    Connections {
        target: lightningQuoteViewModel.stateModel
        function onNodeStatusChanged() {
            if (checkPaymentNodeState(lightningQuoteViewModel.stateModel)
                    && checkPaymentNodeState(
                        lightningBaseViewModel.stateModel)) {
                tryOpenRental()
            }
        }
    }

    RoundedRectangle {
        anchors.fill: parent
        corners.bottomLeftRadius: 10
        corners.topLeftRadius: 10
        customGradient: {
            "vertical": false,
            "colors": [{
                           "position": 0.0,
                           "color": SkinColors.mainDexViewBackgroundGradient1
                       }, {
                           "position": 1.0,
                           "color": SkinColors.mainDexViewBackgroundGradient2
                       }]
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        anchors.topMargin: 15
        spacing: 20

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                spacing: 15

                Text {
                    text: "Dex"
                    font.pixelSize: 32
                    font.family: fontMedium.name
                    color: SkinColors.mainText
                    font.weight: Font.Medium
                    font.capitalization: Font.AllUppercase
                }

                Text {
                    text: walletDexViewModel.online ? "Online" : "Offline"
                    font.pixelSize: 24
                    font.family: fontMedium.name
                    font.capitalization: Font.AllUppercase
                    color: walletDexViewModel.online ? "#1DB182" : "#E2344F"
                    font.weight: Font.Medium
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 15

                Text {
                    text: qsTr("MARKET")
                    font.pixelSize: 12
                    font.family: fontRegular.name
                    color: SkinColors.secondaryText
                }

                DexComboBox {
                    id: comboBox
                    Layout.preferredHeight: 35
                    Layout.fillWidth: true
                    enabled: walletDexViewModel.online && acceptedLiabilityTerms
                    currentIndex: -1
                    model: QMLSortFilterListProxyModel {
                        source: SwapAssetsModel {
                            id: swapAssetsModel
                            Component.onCompleted: {
                                initialize(ApplicationViewModel)
                            }
                            onModelReset: {
                                resetTradePairModel()
                            }
                        }
                        filterRole: "swapPair"
                        filterString: comboBox.contentSearch.text
                        filterCaseSensitivity: Qt.CaseInsensitive
                    }

                    onActivated: {
                        if (currentIndex !== -1
                                && currentComboBoxText !== actualComboBoxText) {
                            var comboBoxModel = model.get(
                                        currentIndex).baseAssetID
                                    !== undefined ? model : swapAssetsModel
                            if (!walletDexViewModel.hasSwapPair
                                    || walletDexViewModel.stateModel.ownOrderBookListModel.rowCount(
                                        ) === 0) {

                                if (ApplicationViewModel.tradingBotModel.isActive) {
                                    ApplicationViewModel.tradingBotModel.stop()
                                }

                                ApplicationViewModel.paymentNodeViewModel.changeSwapAssets(
                                            comboBoxModel.get(
                                                currentIndex).baseAssetID,
                                            comboBoxModel.get(
                                                currentIndex).quoteAssetID,
                                            comboBoxModel.get(
                                                currentIndex).swapPair.replace(
                                                "/", "_"))
                            } else {
                                var popup = openDialog(
                                            dexNotificationComponent, {
                                                "walletDexViewModel": walletDexViewModel,
                                                "baseAssetID": model.get(
                                                                   currentIndex).baseAssetID,
                                                "quoteAssetID": model.get(
                                                                    currentIndex).quoteAssetID,
                                                "baseSymbol": model.get(
                                                                  currentIndex).baseAssetSymbol,
                                                "quoteSymbol": model.get(
                                                                   currentIndex).quoteAssetSymbol
                                            })

                                popup.rejected.connect(function () {
                                    contentSearch.editText.textFormat = actualComboBoxText
                                            === "" ? TextEdit.PlainText : TextEdit.RichText
                                    contentSearch.text = currentComboBoxText = actualComboBoxText
                                    currentSwapPair = actualComboBoxText.substring(
                                                0, 7)
                                })
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: walletDexViewModel.hasSwapPair
                spacing: 5

                Repeater {
                    model: [{
                            "balance": baseBalance,
                            "localBalance": baseLocalBalanceStr,
                            "remoteBalance": baseRemoteBalance,
                            "remoteLocalBalance": baseRemoteBalanceStr,
                            "symbol": currentBaseAssetSymbol,
                            "name": currentBaseAssetName
                        }, {
                            "balance": quoteBalance,
                            "localBalance": quoteLocalBalanceStr,
                            "remoteBalance": quoteRemoteBalance,
                            "remoteLocalBalance": quoteRemoteBalanceStr,
                            "symbol": currentQuoteAssetSymbol,
                            "name": currentQuoteAssetName
                        }]

                    delegate: Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70

                        Rectangle {
                            anchors.fill: parent
                            opacity: 0.5
                            color: SkinColors.dexBalancesViewBackground
                            radius: 10
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.margins: 15
                            spacing: 15

                            GlowImage {
                                id: coinImage
                                Layout.alignment: Qt.AlignTop
                                Layout.preferredHeight: 26
                                Layout.preferredWidth: 28
                                source: modelData.name !== "" ? "qrc:/images/ICON_%1.svg".arg(
                                                                    modelData.name) : ""
                                sourceSize: Qt.size(26, 28)
                                color: "white"

                                MouseArea {
                                    id: balanceToolTipMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                }
                            }

                            Item {
                                Layout.fillHeight: true
                                Layout.fillWidth: true

                                Column {
                                    width: parent.width
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 1

                                    XSNLabel {
                                        text: "%1 %2".arg(
                                                  mainPage.balanceVisible ? Utils.formatBalance(modelData.balance) : hideBalance(Utils.formatBalance(modelData.balance))).arg(
                                                  modelData.symbol)
                                        font.pixelSize: 15
                                        font.family: fontRegular.name
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                        width: parent.width

                                        MouseArea {
                                            id: balanceMouseArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                        }
                                    }

                                    Text {
                                        text: "= %1 %2".arg(
                                                  ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(mainPage.balanceVisible ? modelData.localBalance : hideBalance(modelData.localBalance))
                                        font.pixelSize: 12
                                        font.family: fontRegular.name
                                        color: SkinColors.secondaryText
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }

                                    FadedText {
                                        id: fadedText
                                        text: "Can receive: %1 %2".arg(
                                                  useCoinsForRemoteBalance ? Utils.formatBalance(modelData.remoteBalance) : ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(useCoinsForRemoteBalance ? modelData.symbol : modelData.remoteLocalBalance)

                                        font.pixelSize: 11
                                        font.family: fontRegular.name
                                        color: SkinColors.secondaryText
                                        font.capitalization: Font.MixedCase
                                        elide: Text.ElideRight
                                        width: parent.width

                                        PointingCursorMouseArea {
                                            anchors.fill: parent
                                            onEntered: fadedText.startFade()
                                            onExited: fadedText.stopFade()
                                            onClicked: increaseCanReceiveBalance(
                                                           modelData.symbol
                                                           === currentBaseAssetSymbol)
                                        }
                                    }
                                }

                                CustomToolTip {
                                    height: 70
                                    width: 280
                                    x: -40
                                    y: -75
                                    visible: balanceMouseArea.containsMouse
                                             || balanceToolTipMouseArea.containsMouse
                                    tailPosition: 45
                                    text: "May show less than wallet balance,\ncoins held in reserve for force closing"
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: walletDexViewModel.hasSwapPair
                spacing: 10

                DexHeaderView {
                    id: buySellView
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    model: ListModel {
                        ListElement {
                            name: "Buy"
                            size: 110
                        }
                        ListElement {
                            name: "Sell"
                            size: 110
                        }
                    }
                    currentIndex: 0
                    onCurrentIndexChanged: {
                        swapPortion.currentIndex = -1
                    }
                }

                DexHeaderView {
                    id: limitMarketView
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    model: ListModel {
                        ListElement {
                            name: "Limit"
                            size: 110
                        }
                        ListElement {
                            name: "Market"
                            size: 110
                        }
                    }
                    currentIndex: 0
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 20
                visible: walletDexViewModel.hasSwapPair

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 15

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 5
                            visible: isLimit

                            StyledTextInput {
                                id: priceField
                                Layout.fillWidth: true
                                textFieldHeight: 35
                                validator: RegExpValidator {
                                    regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                                }
                                unitValue: currentQuoteAssetSymbol
                                unitTextColor: SkinColors.mainText
                                radius: 10
                                placeholderText: "%1 price".arg(
                                                     isBuy ? "Buy" : "Sell")
                                onTextChanged: {
                                    if (text === ".") {
                                        text = "0."
                                    }

                                    if (parseFloat(text) <= 0) {
                                        priceNotification.text = "Price must be greater than zero"
                                        return
                                    }

                                    priceNotification.text = ""

                                    totalText = calculateTotal(amountText, text)
                                    orderFee = calculatePlaceOrderFee(
                                                Utils.parseCoinsToSatoshi(
                                                    isBuy ? totalText : amountText),
                                                isBuy ? tradingPairInfo.buyFeePercent : tradingPairInfo.sellFeePercent)
                                    evaluateBalances()
                                }
                            }

                            Text {
                                Layout.rightMargin: 10
                                Layout.alignment: Qt.AlignRight
                                font.pixelSize: 12
                                font.family: fontRegular.name
                                text: "%1 %2".arg(
                                          ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                          ApplicationViewModel.localCurrencyViewModel.convert(
                                              walletDexViewModel.quoteAssetID,
                                              priceText))
                                color: SkinColors.mainText
                            }

                            Text {
                                id: priceNotification
                                Layout.fillWidth: true
                                Layout.leftMargin: 5
                                color: SkinColors.transactionItemSent
                                visible: text.length > 0
                                font.pixelSize: 12
                                font.family: fontRegular.name
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 5

                            StyledTextInput {
                                id: amountTextField
                                enabled: isLimit || !isBuy
                                Layout.fillWidth: true
                                textFieldHeight: 35
                                validator: RegExpValidator {
                                    regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                                }
                                unitValue: currentBaseAssetSymbol
                                radius: 10
                                placeholderText: isLimit ? "Amount to %1".arg(
                                                               isBuy ? "receive" : "spend") : (isBuy ? "Estimated amount to receive" : "Amount to spend")
                                unitTextColor: SkinColors.mainText
                                focus: !isLimit && !isBuy
                                onTextChanged: {
                                    var newTotal = calculateTotal(text,
                                                                  priceText)

                                    if (!totalField.focus) {
                                        totalText = newTotal
                                    }

                                    orderFee = calculatePlaceOrderFee(
                                                Utils.parseCoinsToSatoshi(
                                                    isBuy ? totalText : text),
                                                isBuy ? tradingPairInfo.buyFeePercent : tradingPairInfo.sellFeePercent)
                                    evaluateBalances()
                                }
                            }

                            Text {
                                Layout.rightMargin: 10
                                Layout.alignment: Qt.AlignRight
                                font.pixelSize: 12
                                font.family: fontRegular.name
                                text: "%1 %2".arg(
                                          ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                          ApplicationViewModel.localCurrencyViewModel.convert(
                                              walletDexViewModel.baseAssetID,
                                              amountText))
                                color: SkinColors.mainText
                            }

                            Column {
                                Layout.fillWidth: true
                                Layout.leftMargin: 5
                                spacing: 0

                                Text {
                                    id: amountNotification
                                    property alias clickable: clickableAmountNotification.visible
                                    property alias clickableText: clickableAmountNotification.text
                                    property bool increaseCanSend: false
                                    color: SkinColors.transactionItemSent
                                    font.pixelSize: 12
                                    font.family: fontRegular.name
                                    visible: text.length > 0
                                    font.capitalization: Font.MixedCase
                                }

                                FadedText {
                                    id: clickableAmountNotification
                                    visible: amountNotification.clickable
                                    activeColor: "#f0464a"
                                    inactiveColor: SkinColors.transactionItemSent
                                    color: SkinColors.transactionItemSent
                                    font.pixelSize: 12
                                    font.family: fontRegular.name
                                    font.capitalization: Font.MixedCase
                                    font.underline: true

                                    PointingCursorMouseArea {
                                        id: amountNotifMouseArea
                                        anchors.fill: parent
                                        onEntered: clickableAmountNotification.startFade()
                                        onExited: clickableAmountNotification.stopFade()
                                        onClicked: {
                                            if (amountNotification.increaseCanSend) {
                                                increaseCanSendBalance(true)
                                            } else {
                                                increaseCanReceiveBalance(true)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        DexPercentSwapView {
                            id: swapPortion
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredHeight: 30
                            Layout.fillWidth: true
                            model: ["25%", "50%", "75%", "100%"]
                            currentIndex: -1
                            enabled: priceText.length > 0
                            onClicked: {
                                focus = true
                                var newAmount = 0.0
                                if (isBuy) {
                                    let price = isLimit ? parseFloat(
                                                              priceText) : Utils.convertSatoshiToCoin(
                                                              calculateBestPrice(
                                                                  isBuy))
                                    let allTotal = (quoteBalance * parseInt(
                                                        percent) / 100.0)
                                    newAmount = Math.floor(
                                                (allTotal / (1 + tradingPairInfo.buyFeePercent))
                                                / price)
                                } else {
                                    let allAmount = baseBalance * parseInt(
                                            percent) / 100.0
                                    newAmount = Math.floor(
                                                allAmount / (1 + tradingPairInfo.sellFeePercent))
                                }

                                var newAmountStr = Utils.formatBalance(
                                            newAmount, 8)
                                amountText = newAmountStr
                                amountText = newAmountStr // ugly workaround but we need this to break invalid
                            }

                            Connections {
                                target: amountTextField
                                function onTextChanged() {
                                    swapPortion.currentIndex = -1
                                }
                            }

                            Connections {
                                target: priceField
                                function onTextChanged() {
                                    swapPortion.currentIndex = -1
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 5

                        StyledTextInput {
                            id: totalField
                            enabled: isLimit || isBuy
                            Layout.fillWidth: true
                            radius: 10
                            textFieldHeight: 35
                            placeholderText: isLimit ? "Total" : (isBuy ? "Total to spend" : "Estimated total to receive")
                            validator: RegExpValidator {
                                regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                            }
                            unitTextColor: SkinColors.mainText
                            unitValue: currentQuoteAssetSymbol
                            focus: !isLimit && isBuy
                            onTextChanged: {
                                if (text === ".") {
                                    text = "0."
                                }
                                if (parseFloat(text) <= 0) {
                                    return
                                }

                                var newAmount = calculateAmount(
                                            Utils.parseCoinsToSatoshi(text),
                                            priceText)
                                if (!amountTextField.focus) {
                                    amountText = newAmount
                                }

                                evaluateBalances()
                            }
                        }

                        Text {
                            Layout.rightMargin: 10
                            Layout.alignment: Qt.AlignRight
                            font.pixelSize: 12
                            font.family: fontRegular.name
                            text: "%1 %2".arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                      ApplicationViewModel.localCurrencyViewModel.convert(
                                          walletDexViewModel.quoteAssetID,
                                          totalText))
                            color: SkinColors.mainText
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.leftMargin: 15
                            color: SkinColors.mainText
                            font.pixelSize: 12
                            font.family: fontRegular.name
                            visible: text.length > 0
                            text: orderFee > 0 ? "Place order fee: %1 %2".arg(
                                                     Utils.formatBalance(
                                                         orderFee, 8)).arg(
                                                     isBuy ? currentQuoteAssetSymbol : currentBaseAssetSymbol) : ""
                        }

                        Column {
                            Layout.fillWidth: true
                            Layout.leftMargin: 5
                            spacing: 0

                            Text {
                                id: totalNotification
                                property alias clickable: clickableTotalNotification.visible
                                property alias clickableText: clickableTotalNotification.text
                                property bool increaseCanSend: false
                                color: SkinColors.transactionItemSent
                                font.pixelSize: 12
                                font.family: fontRegular.name
                                visible: text.length > 0
                                font.capitalization: Font.MixedCase
                            }

                            FadedText {
                                id: clickableTotalNotification
                                visible: totalNotification.clickable
                                activeColor: "#f0464a"
                                inactiveColor: SkinColors.transactionItemSent
                                color: SkinColors.transactionItemSent
                                font.pixelSize: 12
                                font.family: fontRegular.name
                                font.capitalization: Font.MixedCase
                                font.underline: true

                                PointingCursorMouseArea {
                                    id: totalNotifMouseArea
                                    anchors.fill: parent
                                    onEntered: clickableTotalNotification.startFade()
                                    onExited: clickableTotalNotification.stopFade()
                                    onClicked: {
                                        if (totalNotification.increaseCanSend) {
                                            increaseCanSendBalance(false)
                                        } else {
                                            increaseCanReceiveBalance(false)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    IntroButton {
                        id: placeOrderButton
                        Layout.fillWidth: true
                        Layout.leftMargin: 10
                        Layout.rightMargin: 10
                        Layout.preferredHeight: 45
                        buttonColor: isBuy ? "#1a9f75" : "#a6303f"
                        buttonGradientColor: isBuy ? "#60c8a7" : "#e75c72"
                        buttonHoveredColor: isBuy ? "#21c792" : "#e44860"
                        buttonGradientHoveredColor: isBuy ? "#8ed8c0" : "#f3adb8"
                        borderColor: "transparent"
                        text: isBuy ? "Buy" : "Sell"
                        textColor: "white"
                        font.capitalization: Font.AllUppercase
                        radius: 10
                        enabled: walletDexViewModel.hasSwapPair
                                 && !amountNotification.visible
                                 && !priceNotification.visible
                                 && !totalNotification.visible
                                 && comboBox.currentStatus === Enums.DexTradingStatus.Online
                                 && !simpleSwapNotification.visible

                        onClicked: {
                            var isAmountEmpty = amountText.length === 0
                            var isLimitPriceEmpty = isLimit
                                    && priceText.length === 0
                            if (isAmountEmpty || isLimitPriceEmpty) {
                                if (isAmountEmpty) {
                                    amountNotification.text = "Enter amount to %1".arg(
                                                isBuy ? "receive" : "spend")
                                }
                                if (isLimitPriceEmpty) {
                                    priceNotification.text = "Enter %1 price".arg(
                                                isBuy ? "buy" : "sell")
                                }

                                return
                            }

                            if (!walletDexViewModel.canPlaceOrder) {
                                showBubblePopup(
                                            "Please wait while your previous order is submitted")
                                return
                            }

                            let priceStr = isLimit ? priceText : ""

                            if (walletDexViewModel.placeOrderBoxVisibility()) {
                                var popup = openDialog(dexOrderPlacedComponent,
                                                       {
                                                           "walletDexViewModel": walletDexViewModel,
                                                           "baseSymbol": currentBaseAssetSymbol,
                                                           "quoteSymbol": currentQuoteAssetSymbol,
                                                           "amount": amountText,
                                                           "total": totalText,
                                                           "price": priceStr,
                                                           "isBuy"// no price for market orders
                                                           : isBuy
                                                       })

                                popup.orderAccepted.connect(createOrder)
                            } else {
                                createOrder(amountText, totalText,
                                            Utils.parseCoinsToSatoshi(
                                                priceStr), isBuy)
                            }
                        }
                    }

                    Text {
                        id: simpleSwapNotification
                        Layout.fillWidth: true
                        Layout.leftMargin: 5
                        color: SkinColors.transactionItemSent
                        font.pixelSize: 12
                        font.family: fontRegular.name
                        visible: simpleSwapInProcess
                        text: "Cannot place order! Simple swap in progress."
                    }
                }
            }
        }
    }

    function evaluateBalances() {
        evaluateAmountAndBalance()
        evaluateTotalAndBalance()
    }

    function evaluateAmountAndBalance() {
        if (amountText === ".") {
            amountText = "0."
        }

        let amount = Utils.parseCoinsToSatoshi(amountText)
        amountNotification.clickable = false

        if (amountText.length > 0 && amount < 10) {
            amountNotification.text = "Amount must be at least 10 satoshis"
            return
        }

        if (ApplicationViewModel.localCurrencyViewModel.convert(
                    walletDexViewModel.baseAssetID,
                    amountText) > 100) {
            amountNotification.text
                    = "Amount must be less then 100$"
            return
        }

        if (!isBuy) {
            // can send on sell - when selling we need to check if we have enough balance to send base currency
            if (!hasEnoughBalance(amount + orderFee, baseBalance)) {
                amountNotification.text = "Not enough %1 balance in lightning \nchannels.".arg(
                            currentBaseAssetSymbol) //"Amount cannot be greater than available \nbalance"
                amountNotification.clickableText = "Add more"
                amountNotification.increaseCanSend = true
                amountNotification.clickable = true
                return
            }
        } else {
            // can receive on buy - when buying we need to check if we have enough balance to receive base currency
            if (!hasEnoughBalance(amount, baseRemoteBalance)) {
                amountNotification.text = "Not enough \u26a1 balance to receive, please"
                amountNotification.clickableText = "rent %1 channel".arg(
                            currentBaseAssetSymbol)
                amountNotification.increaseCanSend = false
                amountNotification.clickable = true
                return
            }
        }

        amountNotification.text = ""
    }

    function evaluateTotalAndBalance() {
        let total = Utils.parseCoinsToSatoshi(totalText)
        totalNotification.clickable = false

        if (!isLimit && Utils.parseCoinsToSatoshi(priceText) <= 0) {
            totalNotification.text = "Cannot place market order! \nNo market price available"
            return
        }

        if (totalText.length > 0 && total < 10000) {
            totalNotification.text
                    = "Cannot place order! \nTotal amount must be at least 10000 satoshis"
            return
        }

        if (ApplicationViewModel.localCurrencyViewModel.convert(
                    walletDexViewModel.quoteAssetID,
                    totalText) > 100) {
            totalNotification.text
                    = "Total amount must be less then 100$"
            return
        }

        if (isBuy) {
            // can send on buy - when buying we need to check if we have enough balance to send quote currency
            if (!hasEnoughBalance(total + orderFee, quoteBalance)) {
                totalNotification.text = "Not enough %1 balance in lightning \nchannels.".arg(
                            currentQuoteAssetSymbol) //"Total amount cannot be greater than available \nbalance"
                totalNotification.clickableText = "Add more"
                totalNotification.increaseCanSend = true
                totalNotification.clickable = true
                return
            }
        } else {
            // can receive on sell - when selling we need to check if we have enough balance to to receive quote currency
            if (!hasEnoughBalance(total, quoteRemoteBalance)) {
                totalNotification.text = "Not enough \u26a1 balance to receive, please"
                totalNotification.clickableText = "rent %1 channel".arg(
                            currentQuoteAssetSymbol)
                totalNotification.increaseCanSend = false
                totalNotification.clickable = true
                return
            }
        }
        totalNotification.text = ""
    }

    function updateBestPrice(force) {
        if (force || priceText.length === 0 || Utils.parseCoinsToSatoshi(
                    priceText) === 0.0) {
            var bestPrice = calculateBestPrice(isBuy)
            priceField.text = bestPrice > 0 ? Utils.formatBalance(
                                                  bestPrice) : ""
        }
    }

    function createOrder(amount, total, price, isBuy) {
        showBubblePopup("Order submitted")
        walletDexViewModel.createOrder(amount, total, price, isBuy)
    }

    function tryOpenRental() {
        var baseAssetHasActiveChannel = lightningBaseViewModel.stateModel.isChannelOpened
        var quoteAssetHasActiveChannel = lightningQuoteViewModel.stateModel.isChannelOpened

        if (((baseAssetHasActiveChannel && !quoteAssetHasActiveChannel)
             || (!baseAssetHasActiveChannel && quoteAssetHasActiveChannel))
                && activePopup === undefined
                && menuStackLayout.currentIndex === 3) //open only for dex page
        {
            rentalBase = !baseAssetHasActiveChannel
            var popup = openDialog(rentalNotificationComponent, {
                                       "message": "To trade %1/%2 pair you will need to enable a %3 channel rental.\n Please confirm to continue.".arg(
                                                      currentBaseAssetSymbol).arg(
                                                      currentQuoteAssetSymbol).arg(
                                                      baseAssetHasActiveChannel ? currentQuoteAssetSymbol : currentBaseAssetSymbol)
                                   })
            popup.closed.connect(function () {
                MouseEventSpy.setEventFilerEnabled(false)
            })
        }
    }

    function resetTradePairModel() {
        if (acceptedLiabilityTerms && comboBox.currentIndex === -1) {
            changePair()
        }
    }

    function allowOpenChannel(isBaseAsset) {
        var lndStatus = isBaseAsset ? lightningBaseViewModel.stateModel.nodeStatus : lightningQuoteViewModel.stateModel.nodeStatus
        if (lndStatus !== LnDaemonStateModel.LndStatus.LndActive
                || lndStatus === ConnextStateModel.ConnextStatus.ConnextActive) {
            showBubblePopup("Can't open channel until lnd is synced")
            return false
        }
        return true
    }

    function increaseCanReceiveBalance(isBaseAsset) {
        if (allowOpenChannel(isBaseAsset)) {
            var id = isBaseAsset ? walletDexViewModel.baseAssetID : walletDexViewModel.quoteAssetID
            var symbol = isBaseAsset ? currentBaseAssetSymbol : currentQuoteAssetSymbol
            var minCapacity = assetModel.get(assetModel.getInitial(
                                                 id)).minLndCapacity
            var popup = openChannelRentalDialog({
                                                    "assetID": id,
                                                    "symbol": symbol,
                                                    "minLndCapacity": minCapacity
                                                })
        }
    }

    function increaseCanSendBalance(isBaseAsset) {
        if (allowOpenChannel(isBaseAsset)) {
            var assetID = isBaseAsset ? walletDexViewModel.baseAssetID : walletDexViewModel.quoteAssetID
            var popup = openChannelDialog({
                                              "assetID": assetID
                                          })
        }
    }

    function changePair() {
        var index = swapAssetsModel.lastTradingPairIndex()
        if (index !== comboBox.currentIndex) {
            comboBox.contentSearch.editText.textFormat = TextEdit.RichText
            comboBox.contentSearch.text = comboBox.currentComboBoxText
                    = '%1 - <font color="%2">%3</font>'.arg(
                        swapAssetsModel.get(index).swapPair).arg(
                        comboBox.getColor(swapAssetsModel.get(
                                              index).dexStatus)).arg(
                        comboBox.getText(swapAssetsModel.get(index).dexStatus))
            comboBox.currentSwapPair = swapAssetsModel.get(index).swapPair
            comboBox.currentStatus = swapAssetsModel.get(index).dexStatus
            comboBox.currentIndex = index
            comboBox.activated(index)
        }
    }
}
