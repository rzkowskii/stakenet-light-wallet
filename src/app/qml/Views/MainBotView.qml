import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Item {
    id: mainBotView
    property WalletDexViewModel walletDexViewModel

    property alias baseAmountText: baseAmount.text
    property alias quoteAmountText: quoteAmount.text

    property alias baseAmountLocalText: baseAmountLocal.text
    property alias quoteAmountLocalText: quoteAmountLocal.text

    property alias baseComboBox: baseAmount.comboBox
    property alias quoteComboBox: quoteAmount.comboBox

    property var baseAsset: walletDexViewModel.hasSwapPair ? assetModel.get(
                                                                 assetModel.getInitial(
                                                                     walletDexViewModel.baseAssetID)) : undefined
    property var quoteAsset: walletDexViewModel.hasSwapPair ? assetModel.get(
                                                                  assetModel.getInitial(
                                                                      walletDexViewModel.quoteAssetID)) : undefined

    property string currentBaseAssetSymbol: baseAsset ? baseAsset.symbol : ""
    property string currentQuoteAssetSymbol: quoteAsset ? quoteAsset.symbol : ""

    property string currentBaseAssetName: baseAsset ? baseAsset.name : ""
    property string currentQuoteAssetName: quoteAsset ? quoteAsset.name : ""

    property string currentBaseAssetColor: baseAsset ? baseAsset.color : ""
    property string currentQuoteAssetColor: quoteAsset ? quoteAsset.color : ""

    property var channelsBalance: channelBalancesViewModel.channelsBalance

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

    property string quoteLocalBalanceLocalStr: walletDexViewModel.hasSwapPair ? ApplicationViewModel.localCurrencyViewModel.convert(walletDexViewModel.quoteAssetID, Utils.formatBalance(quoteBalance)) : ""
    property string baseLocalBalanceLocalStr: walletDexViewModel.hasSwapPair ? ApplicationViewModel.localCurrencyViewModel.convert(walletDexViewModel.baseAssetID, Utils.formatBalance(baseBalance)) : ""

    property string quoteRemoteBalanceLocalStr: walletDexViewModel.hasSwapPair ? ApplicationViewModel.localCurrencyViewModel.convert(walletDexViewModel.quoteAssetID, Utils.formatBalance(quoteRemoteBalance)) : ""
    property string baseRemoteBalanceLocalStr: walletDexViewModel.hasSwapPair ? ApplicationViewModel.localCurrencyViewModel.convert(walletDexViewModel.baseAssetID, Utils.formatBalance(baseRemoteBalance)) : ""

    property var tradingPairInfo: walletDexViewModel.tradingPairInfo
    property double buyOrderFee: 0.0
    property double sellOrderFee: 0.0
    property string priceText: ""

    property bool botButtonEnabled: ApplicationViewModel.tradingBotModel.isActive ? true : (walletDexViewModel.hasSwapPair && !baseAmountNotification.visible && !quoteAmountNotification.visible && !commonNotification.visible)

    property bool marketHasOrders: priceText !== ""
                                   && Utils.parseCoinsToSatoshi(
                                       priceText) !== 0.0

    property int botLevels: 0
    property int minimumQuoteAmountSatsToPlaceOrder: 10000 * botLevels * 2
    property int minimumBaseAmountSatsToPlaceOrder: 10 * botLevels * 2
    property double orderFeePercent: 0.2 / 100

    onPriceTextChanged: {
        quoteAmountText = calculateTotal(baseAmountText, priceText)
        buyOrderFee = calculatePlaceOrderFee(Utils.parseCoinsToSatoshi(
                                                 quoteAmountText),
                                             orderFeePercent)
                                             //tradingPairInfo.sellFeePercent)
        sellOrderFee = calculatePlaceOrderFee(Utils.parseCoinsToSatoshi(
                                                  baseAmountText),
                                              orderFeePercent)
                                              //tradingPairInfo.buyFeePercent)
        evaluateBalances()
    }

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

    Component.onCompleted: {
        botLevels = ApplicationViewModel.tradingBotModel.gridLevels()
    }

    Timer {
        id: timer
        repeat: false
        interval: 1500
        onTriggered: {
            if (ApplicationViewModel.tradingBotModel.isActive) {
                actionButton.enabled = true
            }
        }
    }

    Connections {
        target: ApplicationViewModel.paymentNodeViewModel
        function onChangeSwapAssetsFailed(error) {
            if (root.focus) {
                commonNotification.text = "No such trading pair"
            }
        }
    }

    Connections {
        target: mainPage
        function onAcceptedLiabilityTermsChanged() {
            resetTradePairModel()
        }
    }

    Connections {
        enabled: walletDexViewModel.hasSwapPair
        target: walletDexViewModel
                && walletDexViewModel.stateModel ? walletDexViewModel.stateModel.buyOrdersListModel : null
        function onBestPriceChanged() {
            updateBestPrice()
        }
    }

    Connections {
        enabled: walletDexViewModel.hasSwapPair
        target: walletDexViewModel
                && walletDexViewModel.stateModel ? walletDexViewModel.stateModel.sellOrdersListModel : null
        function onBestPriceChanged() {
            updateBestPrice()
        }
    }

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
    }

    WalletDexChannelBalanceViewModel {
        id: channelBalancesViewModel

        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    Connections {
        target: walletDexViewModel
        function onSwapAssetsChanged() {
            if (walletDexViewModel.baseAssetID !== baseComboBox.currentAssetID
                    || walletDexViewModel.quoteAssetID !== quoteComboBox.currentAssetID) {
                changeComboBoxPair(walletDexViewModel.baseAssetID,
                                   walletDexViewModel.quoteAssetID)
            }
            if (walletDexViewModel.hasSwapPair) {
                channelBalancesViewModel.changeTraidingPair(
                            walletDexViewModel.baseAssetID,
                            walletDexViewModel.quoteAssetID)
            } else {
                channelBalancesViewModel.reset()
            }
            updateBestPrice()
        }
    }

    GradientBackgroundComponent {
        anchors.fill: parent
        radius: 10
        gradientOpacity: 1
        firstColor: SkinColors.botPageMainBackgroundColorFirst
        secondColor: SkinColors.botPageMainBackgroundColorSecond
        thirdColor: SkinColors.botPageMainBackgroundColorThird
        gradientSourceRadius: 10
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 0.04 * mainBotView.height

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 0.23 * mainBotView.height
            Layout.maximumHeight: 250
            visible: walletDexViewModel.hasSwapPair

            RowLayout {
                anchors.fill: parent
                spacing: 0.1 * mainBotView.width

                Repeater {
                    model: [{
                            "assetName": currentBaseAssetName,
                            "balance": baseBalance,
                            "localBalance": baseLocalBalanceLocalStr,
                            "remoteBalance": baseRemoteBalance,
                            "remoteLocalBalance": baseRemoteBalanceLocalStr,
                            "symbol": currentBaseAssetSymbol,
                            "name": currentBaseAssetName
                        }, {
                            "assetName": currentQuoteAssetName,
                            "balance": quoteBalance,
                            "localBalance": quoteLocalBalanceLocalStr,
                            "remoteBalance": quoteRemoteBalance,
                            "remoteLocalBalance": quoteRemoteBalanceLocalStr,
                            "symbol": currentQuoteAssetSymbol,
                            "name": currentQuoteAssetName
                        }]

                    delegate: RowLayout {
                        height: parent.height
                        width: parent.width
                        spacing: 0.02 * mainBotView.width

                        BotAssetImage {
                            Layout.alignment: Qt.AlignTop
                            Layout.preferredHeight: 0.16 * mainBotView.height
                            Layout.preferredWidth: 0.16 * mainBotView.height
                            Layout.maximumHeight: 150
                            assetName: modelData.assetName
                        }

                        Item {
                            Layout.alignment: Qt.AlignTop
                            Layout.fillWidth: true
                            Layout.preferredHeight: 0.23 * mainBotView.height

                            ColumnLayout {
                                spacing: 10

                                Column {
                                    spacing: 5

                                    Text {
                                        anchors.leftMargin: 10
                                        anchors.left: parent.left
                                        text: "Can send"
                                        font.pixelSize: 14
                                        font.family: fontRegular.name
                                        font.weight: Font.Light
                                        font.capitalization: Font.AllUppercase
                                        color: SkinColors.secondaryText
                                    }

                                    Column {
                                        spacing: 1
                                        XSNLabel {
                                            anchors.leftMargin: 10
                                            anchors.left: parent.left
                                            text: "%1 %2".arg(
                                                      Utils.formatBalance(
                                                          modelData.balance)).arg(
                                                      modelData.symbol)
                                            font.pixelSize: 18
                                            font.family: fontRegular.name
                                            font.weight: Font.Light
                                        }

                                        XSNLabel {
                                            anchors.leftMargin: 10
                                            anchors.left: parent.left
                                            text: "%1 %2 %3".arg(
                                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                                      modelData.localBalance).arg(
                                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                                            font.pixelSize: 14
                                            font.family: fontRegular.name
                                        }
                                    }
                                }

                                Column {
                                    spacing: 5

                                    Text {
                                        anchors.leftMargin: 10
                                        anchors.left: parent.left
                                        text: "Can receive"
                                        font.pixelSize: 14
                                        font.family: fontRegular.name
                                        font.weight: Font.Light
                                        font.capitalization: Font.AllUppercase
                                        color: SkinColors.secondaryText
                                    }

                                    Column {
                                        spacing: 1

                                        XSNLabel {
                                            anchors.leftMargin: 10
                                            anchors.left: parent.left
                                            text: "%1 %2".arg(
                                                      Utils.formatBalance(
                                                          modelData.remoteBalance)).arg(
                                                      modelData.symbol)
                                            font.pixelSize: 18
                                            font.family: fontRegular.name
                                            font.weight: Font.Light
                                        }

                                        XSNLabel {
                                            anchors.leftMargin: 10
                                            anchors.left: parent.left
                                            text: "%1 %2 %3".arg(
                                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                                      modelData.remoteLocalBalance).arg(
                                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                                            font.pixelSize: 14
                                            font.family: fontRegular.name
                                        }
                                    }
                                }
                            }

                            MouseArea {
                                id: balanceMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                            }

                            CustomToolTip {
                                height: 70
                                width: 280
                                x: -30
                                y: -65
                                visible: balanceMouseArea.containsMouse
                                tailPosition: 45
                                text: "May show less than wallet balance,\ncoins held in reserve for force closing"
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            spacing: 0.033 * mainBotView.height

            Text {
                text: "Start Bot Trading"
                color: SkinColors.mainText
                font.pixelSize: 17
                font.family: fontRegular.name
                font.capitalization: Font.AllUppercase
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 15

                RowLayout {
                    Layout.preferredHeight: 60
                    Layout.fillWidth: true
                    spacing: 0.1 * mainBotView.width

                    ColumnLayout {
                        spacing: 0.01 * mainBotView.height

                        BotTextInput {
                            id: baseAmount
                            Layout.preferredWidth: 0.4 * mainBotView.width
                            Layout.preferredHeight: 40
                            placeholderText: "Amount"
                            validator: RegExpValidator {
                                regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                            }

                            comboBox.model: QMLSortFilterListProxyModel {
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
                            onActivated: {
                                if (comboBox.currentIndex !== -1
                                        && comboBox.currentIndex !== comboBox.actualIndex
                                        && quoteComboBox.currentIndex !== -1) {
                                    activatePair()
                                }
                            }
                            onTextChanged: {
                                var newQuote = calculateTotal(text, priceText)

                                if (!quoteAmount.focus) {
                                    quoteAmountText = newQuote
                                }

                                buyOrderFee = calculatePlaceOrderFee(
                                            Utils.parseCoinsToSatoshi(
                                                quoteAmountText),
                                            orderFeePercent)
                                            //tradingPairInfo.sellFeePercent)
                                sellOrderFee = calculatePlaceOrderFee(
                                            Utils.parseCoinsToSatoshi(
                                                baseAmountText),
                                            orderFeePercent)
                                            //tradingPairInfo.buyFeePercent)
                                evaluateBalances()
                            }
                        }

                        Text {
                            id: baseAmountLocal
                            Layout.leftMargin: 15
                            text: "%1 %2".arg(
                                      ApplicationViewModel.localCurrencyViewModel.convert(
                                          walletDexViewModel.baseAssetID,
                                          baseAmountText)).arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                            color: SkinColors.mainText
                            font.pixelSize: 14
                            font.family: fontRegular.name
                        }

                        Item {
                            Layout.preferredHeight: 15
                            Layout.fillWidth: true

                            Text {
                                id: baseAmountNotification
                                anchors.fill: parent
                                color: SkinColors.transactionItemSent
                                font.pixelSize: 12
                                font.family: fontRegular.name
                                visible: text.length > 0
                                font.capitalization: Font.MixedCase
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 0.01 * mainBotView.height

                        BotTextInput {
                            id: quoteAmount
                            Layout.preferredWidth: 0.4 * mainBotView.width
                            Layout.preferredHeight: 40
                            placeholderText: "Amount"
                            validator: RegExpValidator {
                                regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                            }
                            comboBox.model: QMLSortFilterListProxyModel {
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

                            onActivated: {
                                if (comboBox.currentIndex !== -1
                                        && baseComboBox.currentIndex !== -1
                                        && comboBox.currentIndex !== comboBox.actualIndex) {

                                    activatePair()
                                }
                            }
                            onTextChanged: {
                                if (text === ".") {
                                    text = "0."
                                }
                                if (parseFloat(text) <= 0) {
                                    return
                                }

                                var newBase = calculateAmount(
                                            Utils.parseCoinsToSatoshi(text),
                                            priceText)
                                if (!baseAmount.focus) {
                                    baseAmountText = newBase
                                }

                                evaluateBalances()
                            }
                        }

                        Text {
                            id: quoteAmountLocal
                            Layout.leftMargin: 15
                            text: "%1 %2".arg(
                                      ApplicationViewModel.localCurrencyViewModel.convert(
                                          walletDexViewModel.quoteAssetID,
                                          quoteAmountText)).arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                            color: SkinColors.mainText
                            font.pixelSize: 14
                            font.family: fontRegular.name
                        }
                        Item {
                            Layout.preferredHeight: 15
                            Layout.fillWidth: true

                            Text {
                                anchors.fill: parent
                                id: quoteAmountNotification
                                color: SkinColors.transactionItemSent
                                font.pixelSize: 12
                                font.family: fontRegular.name
                                visible: text.length > 0
                                font.capitalization: Font.MixedCase
                            }
                        }
                    }
                }

                Text {
                    id: commonNotification
                    Layout.alignment: Qt.AlignHCenter
                    color: SkinColors.transactionItemSent
                    font.pixelSize: 14
                    font.family: fontRegular.name
                    visible: text.length > 0
                }
            }
        }

        Row {
            spacing: 5

            DexPercentSwapView {
                id: swapPortion
                width: 172
                height: 30
                model: ["25%", "50%", "75%", "100%"]
                currentIndex: -1
                enabled: priceText.length > 0
                color: SkinColors.botTextFieldInactiveStateColor
                opacityBackground: 0.1
                visible: walletDexViewModel.hasSwapPair
                onClicked: {
                    focus = true
                    var newAmount = 0.0
                    let price = Utils.convertSatoshiToCoin(
                            (calculateBestPrice(false) + calculateBestPrice(
                                                         true)) / 2)

                    var minimumBalance = Math.min(baseBalance,
                                                  quoteBalance / price,
                                                  baseRemoteBalance,
                                                  quoteRemoteBalance / price)
                    var maximum = minimumBalance * parseInt(
                            percent) / 100.0

                    newAmount = Math.floor(
                                    maximum / 1.002) // 1.002 = amount + 0.2% * amount(order fee)

                    var newAmountStr = Utils.formatBalance(newAmount, 8)
                    baseAmountText = newAmountStr
                    baseAmountText = newAmountStr // ugly workaround but we need this to break invalid
                }

                Connections {
                    target: baseAmount
                    function onTextChanged() {
                        swapPortion.currentIndex = -1
                    }
                }
            }

            HelpComponent {
                anchors.topMargin: 3
                anchors.top: parent.top
                toolTip.text: "Part of %1 amount which\n can be used for bot trading.".arg(currentBaseAssetSymbol)
                toolTip.tailPosition: 25
                toolTip.height: 65
                toolTip.width: 250
                toolTip.x: -16
                toolTip.y: -65
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 0.15 * mainBotView.height
            spacing: 5
            visible: walletDexViewModel.hasSwapPair

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Item {
                    Layout.preferredHeight: 50
                    Layout.preferredWidth: 50

                    Rectangle {
                        anchors.fill: parent
                        radius: 10
                        opacity: 0.1
                        color: SkinColors.botDepthCurveImageBackground
                    }

                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/images/icon_24_storm.svg"
                        sourceSize: Qt.size(24, 24)
                    }
                }

                Column {
                    spacing: 5

                    Row {
                        spacing: 5

                        Text {
                            text: "Depth curve"
                            color: SkinColors.mainText
                            font.pixelSize: 17
                            font.family: fontRegular.name
                            font.capitalization: Font.AllUppercase
                        }

                        HelpComponent {
                            toolTip.text: "Increasing the risk will result \n in the orders being posted very \n close to the current market price  \n and therefore likely to be filled \n fast and vice versa with decreasing."
                            toolTip.tailPosition: 25
                            toolTip.height: 120
                            toolTip.width: 300
                            toolTip.x: -16
                            toolTip.y: -120
                        }
                    }

                    Text {
                        text: "%1%".arg(slider.value)
                        color: SkinColors.mainText
                        font.pixelSize: 28
                        font.family: fontRegular.name
                    }
                }
            }

            BotSlider {
                id: slider
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                from: 1
                to: 100
                value: 64
                stepSize: 1
            }
        }

        GradientButton {
            id: actionButton
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            Layout.preferredHeight: 0.07 * mainBotView.height
            Layout.maximumHeight: 50
            Layout.preferredWidth: 0.21 * mainBotView.width
            borderColor: SkinColors.popupFieldBorder
            buttonGradientLeftHoveredColor: currentBaseAssetColor
            buttonGradientRightHoveredColor: currentQuoteAssetColor
            font.family: fontRegular.name
            font.pixelSize: 18
            text: ApplicationViewModel.tradingBotModel.isActive ? "Stop" : "Start"
            radius: 10
            visible: walletDexViewModel.hasSwapPair
            enabled: botButtonEnabled
            onClicked: {
                if (!ApplicationViewModel.tradingBotModel.isActive) {
                    if (baseAmountText.length === 0) {
                        baseAmountNotification.text = "Enter liquidity amount"
                        return
                    }
                    if (walletDexViewModel.hasSwapPair) {
                        ApplicationViewModel.tradingBotModel.start(
                                    slider.value,
                                    Utils.convertCoinToSatoshi(baseAmountText),
                                    Utils.convertCoinToSatoshi(quoteAmountText))
                        enabled = false
                        timer.start()
                    }
                } else {
                    ApplicationViewModel.tradingBotModel.stop()
                    enabled = botButtonEnabled
                }
            }
        }
    }

    function evaluateBalances() {
        evaluateBaseBalance()
        evaluateQuoteBalance()
    }

    function evaluateBaseBalance() {
        if (baseAmountText === ".") {
            baseAmountText = "0."
        }

        let base = Utils.parseCoinsToSatoshi(baseAmountText)
        var minimumFee = calculatePlaceOrderFee(minimumBaseAmountSatsToPlaceOrder,
                                                orderFeePercent / 100)

        if (baseAmountText.length > 0 && base < minimumBaseAmountSatsToPlaceOrder + minimumFee) {
            baseAmountNotification.text = "Base amount must be at least %1 satoshis".arg(minimumBaseAmountSatsToPlaceOrder + minimumFee)
            return
        }

        // can send on sell - when selling we need to check if we have enough balance to send base currency
        if (!hasEnoughBalance(base + sellOrderFee, baseBalance)) {
            baseAmountNotification.text = "Not enough %1 balance in lightning channels.".arg(
                        currentBaseAssetSymbol) //"Amount cannot be greater than available \nbalance"
            return
        }

        if (!hasEnoughBalance(base, baseRemoteBalance)) {
            baseAmountNotification.text
                    = "Not enough \u26a1 balance to receive, please\n rent %1 channel".arg(
                        currentBaseAssetSymbol)
            return
        }

        baseAmountNotification.text = ""
    }

    function evaluateQuoteBalance() {
        let quote = Utils.parseCoinsToSatoshi(quoteAmountText)

        var minimumFee = calculatePlaceOrderFee(minimumQuoteAmountSatsToPlaceOrder,
                                                orderFeePercent)

        if (quoteAmountText.length > 0 && quote < minimumQuoteAmountSatsToPlaceOrder + minimumFee) {
            quoteAmountNotification.text = "Quote amount must be at least %1 satoshis".arg(minimumQuoteAmountSatsToPlaceOrder + minimumFee)
            return
        }

        if (!hasEnoughBalance(quote + buyOrderFee, quoteBalance)) {
            quoteAmountNotification.text = "Not enough %1 balance in lightning channels.".arg(
                        currentQuoteAssetSymbol) //"Total amount cannot be greater than available \nbalance"
            return
        }

        // can receive on sell - when selling we need to check if we have enough balance to to receive quote currency
        if (!hasEnoughBalance(quote, quoteRemoteBalance)) {
            quoteAmountNotification.text
                    = "Not enough \u26a1 balance to receive, please\n rent %1 channel".arg(
                        currentQuoteAssetSymbol)
            return
        }
        quoteAmountNotification.text = ""
    }

    function changeComboBoxPair(baseAssetID, quoteAssetID) {
        if (!walletDexViewModel.hasSwapPair && !swapAssetsModel.hasPair(
                    baseAssetID, quoteAssetID)) {
            baseComboBox.currentIndex = -1
            baseComboBox.accepted(baseComboBox.currentIndex)
            quoteComboBox.currentIndex = -1
            quoteComboBox.accepted(quoteComboBox.currentIndex)
            baseAmountText = ""
            quoteAmountText = ""
        } else {
            commonNotification.text = ""
            for (var i = 0; i < sortedLeftAssetModel.count; i++) {
                if (sortedLeftAssetModel.get(i).id === baseAssetID) {
                    baseComboBox.currentIndex = i
                    baseComboBox.accepted(baseComboBox.currentIndex)
                }
            }

            for (i = 0; i < sortedRightAssetModel.count; i++) {
                if (sortedRightAssetModel.get(i).id === quoteAssetID) {
                    quoteComboBox.currentIndex = i
                    quoteComboBox.accepted(quoteComboBox.currentIndex)
                }
            }
        }
    }

    function resetTradePairModel() {
        if (acceptedLiabilityTerms && baseComboBox.currentIndex === -1
                && quoteComboBox.currentIndex === -1
                && swapAssetsModel.rowCount() > 0) {
            if (swapAssetsModel.hasPair(swapAssetsModel.lastBaseAssetID(),
                                        swapAssetsModel.lastQuoteAssetID())) {
                changeComboBoxPair(swapAssetsModel.lastBaseAssetID(),
                                   swapAssetsModel.lastQuoteAssetID())
            }
        }
    }

    function resetComboBoxes() {
        if (acceptedLiabilityTerms && baseComboBox.currentIndex === -1
                && quoteComboBox.currentIndex === -1
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

        if (baseComboBox.currentAssetID !== -1
                && quoteComboBox.currentAssetID !== -1) {
            var baseAssetID = -1
            var quoteAssetID = -1
            var baseAssetSymbol = ""
            var quoteAssetSymbol = ""

            baseAssetID = baseComboBox.currentAssetID
            quoteAssetID = quoteComboBox.currentAssetID
            baseAssetSymbol = baseComboBox.currentSymbol
            quoteAssetSymbol = quoteComboBox.currentSymbol

            if (walletDexViewModel.baseAssetID === baseAssetID
                    && walletDexViewModel.quoteAssetID === quoteAssetID) {
                updateBestPrice()
                return
            }

            if (ApplicationViewModel.tradingBotModel.isActive) {
                ApplicationViewModel.tradingBotModel.stop()
            }

            if (!walletDexViewModel.hasSwapPair
                    || walletDexViewModel.stateModel.ownOrderBookListModel.rowCount(
                        ) === 0) {
                ApplicationViewModel.paymentNodeViewModel.changeSwapAssets(
                            baseAssetID, quoteAssetID,
                            baseAssetSymbol + "_" + quoteAssetSymbol)
            } else {
                var popup = openDialog(dexNotificationComponent, {
                                           "walletDexViewModel": walletDexViewModel,
                                           "baseAssetID": baseAssetID,
                                           "quoteAssetID": quoteAssetID,
                                           "baseSymbol": baseAssetSymbol,
                                           "quoteSymbol": quoteAssetSymbol
                                       })

                popup.rejected.connect(function () {
                    if (baseComboBox.actualIndex !== -1) {
                        baseComboBox.currentIndex = baseComboBox.actualIndex
                        baseComboBox.activated(baseComboBox.currentIndex)
                    }
                    if (quoteComboBox.actualIndex !== -1) {
                        quoteComboBox.currentIndex = quoteComboBox.actualIndex
                        quoteComboBox.activated(quoteComboBox.currentIndex)
                    }
                })
            }
        }
    }

    function updateBestPrice() {
        commonNotification.text = ""
        if (!walletDexViewModel.hasSwapPair) {
            return
        }

        var buyPrice = calculateBestPrice(false)
        var sellPrice = calculateBestPrice(true)

        if (buyPrice > 0 && sellPrice > 0) {
            priceText = Utils.formatBalance((buyPrice + sellPrice) / 2)
            baseAmount.textFieldEnabled = true
            quoteAmount.textFieldEnabled = true
        } else {
            priceText = ""
            if(buyPrice === 0) {
               commonNotification.text = "No market buy price available"
            }
            if(sellPrice === 0) {
               commonNotification.text = "No market sell price available"
            }
            baseAmountText = ""
            quoteAmountText = ""
            baseAmountLocalText = ""
            quoteAmountLocalText = ""
            baseAmount.textFieldEnabled = false
            quoteAmount.textFieldEnabled = false
            baseAmountNotification.text = ""
            quoteAmountNotification.text = ""
        }
    }
}
