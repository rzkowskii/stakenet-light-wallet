import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls 2.2 as NewControls
import QtGraphicalEffects 1.0
import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root

    property int assetID: 0
    property int currentIndex: -1
    property string currentSymbol: assetModel.get(currentIndex).symbol
    property real currentBalance: assetModel.get(currentIndex).balance
    property string currentBalanceStr: Utils.formatBalance(currentBalance)
    property double currentBalanceOnChain: assetModel.get(
                                               currentIndex).balanceOnChain
    property string currentBalanceOnChainStr: Utils.formatBalance(
                                                  currentBalanceOnChain)
    property double currentLndBalance: assetModel.get(
                                           currentIndex).availableLndBalance
    property string currentLndBalanceStr: Utils.formatBalance(
                                              assetModel.get(
                                                  currentIndex).availableLndBalance)
    property string currentName: assetModel.get(currentIndex).name
    property var networkFee: 0
    property double recommendedNetworkFeeRate: 0
    property double networkFeeRate: 0
    property string currentCurrencyCode: ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode

    function refreshBalances() {
        var asset = assetModel.get(currentIndex)
        currentBalance = asset.balance
        currentBalanceOnChain = asset.balanceOnChain
        currentLndBalance = asset.availableLndBalance
    }

    WalletAssetsListModel {
        id: assetModel

        onAverageFeeForAssetFinished: {
            networkFeeRate = recommendedNetworkFeeRate = value
        }

        Component.onCompleted: {
            initialize(ApplicationViewModel)
            root.currentIndex = getInitial(assetID)
            averageFeeForAsset(assetID)
        }
        onDataChanged: refreshBalances()
    }

    PaymentNodeViewModel {
        id: paymentNodeViewModel
        currentAssetID: assetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    SendTransactionViewModel {
        id: sendTransactionViewModel

        onTransactionSendingFailed: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": error,
                               "resultMsg": "Failed!",
                               "imgPath": "qrc:/images/crossIcon.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }

        onTransactionSendingFinished: function (transactionID) {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": "Transaction %1 : \n ".arg(
                                                   paymentNodeViewModel.type
                                                   === Enums.PaymentNodeType.Lnd ? "Id" : "hash"),
                               "txID": transactionID,
                               "resultMsg": "Success!",
                               "imgPath": "qrc:/images/SUCCESS_ICON.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }

        Component.onCompleted: {
            initialize(ApplicationViewModel)
            requestAddressDetails(assetID)
        }
    }

    LightningSendTransactionViewModel {
        id: lightningSendViewModel

        Component.onCompleted: {
            initialize(assetID, ApplicationViewModel)
        }

        onTransactionSendingFailed: function (error) {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": error,
                               "resultMsg": "Failed!",
                               "imgPath": "qrc:/images/crossIcon.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }

        onTransactionSendingFinished: function (transactionID) {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": "Transaction Id :",
                               "txID": transactionID,
                               "resultMsg": "Success!",
                               "imgPath": "qrc:/images/SUCCESS_ICON.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }
    }

    popUpText: "Send"
    width: 575
    height: 530

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.topMargin: 22
        anchors.bottomMargin: 22
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        clip: true

        initialItem: Item {

            ColumnLayout {
                anchors.leftMargin: 5
                anchors.rightMargin: 5
                anchors.fill: parent
                spacing: 15

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15

                    Image {
                        sourceSize: Qt.size(55, 60)
                        source: currentName !== "" ? "qrc:/images/ICON_%1.svg".arg(
                                                         currentName) : ""
                    }

                    Column {
                        Layout.fillWidth: true
                        spacing: 5

                        XSNLabel {
                            font.family: mediumFont.name
                            font.pixelSize: 20
                            text: "Send %1 - %2".arg(currentName).arg(
                                      currentSymbol)
                            color: SkinColors.mainText
                            elide: Text.ElideRight
                            width: parent.width
                        }

                        Column {
                            Layout.fillWidth: true

                            RowLayout {
                                spacing: 5

                                XSNLabel {
                                    text: "Balance     : "
                                    font.family: regularFont.name
                                    font.pixelSize: 13
                                    color: SkinColors.secondaryText
                                    font.capitalization: Font.AllUppercase
                                }

                                SelectedText {
                                    text: "%1 %2".arg(
                                              mainPage.balanceVisible ? currentBalanceStr : hideBalance(
                                                                            currentBalanceStr)).arg(
                                              currentSymbol)
                                    font.family: regularFont.name
                                    font.pixelSize: 14
                                    color: SkinColors.mainText
                                }
                            }

                            RowLayout {
                                spacing: 5

                                XSNLabel {
                                    text: "On-chain    : "
                                    font.family: regularFont.name
                                    font.pixelSize: 13
                                    color: SkinColors.secondaryText
                                    font.capitalization: Font.AllUppercase
                                }

                                Row {
                                    spacing: 0

                                    SelectedText {
                                        text: "%1 %2".arg(
                                                  mainPage.balanceVisible ? currentBalanceOnChainStr : hideBalance(currentBalanceOnChainStr)).arg(
                                                  currentSymbol)
                                        font.family: regularFont.name
                                        font.pixelSize: 14
                                        color: SkinColors.mainText
                                    }

                                    XSNLabel {
                                        font.family: regularFont.name
                                        font.pixelSize: 14
                                        color: SkinColors.secondaryText
                                        text: " ("
                                    }

                                    Image {
                                        source: "qrc:/images/ic_lightning_small.png"
                                    }

                                    SelectedText {
                                        font.family: regularFont.name
                                        font.pixelSize: 14
                                        color: SkinColors.secondaryText
                                        text: "%1 %2".arg(
                                                  mainPage.balanceVisible ? currentLndBalanceStr : hideBalance(currentLndBalanceStr)).arg(
                                                  currentSymbol)
                                    }

                                    XSNLabel {
                                        font.family: regularFont.name
                                        font.pixelSize: 14
                                        color: SkinColors.secondaryText
                                        text: ")"
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    CloseButton {
                        Layout.preferredHeight: 30
                        Layout.preferredWidth: 30
                        Layout.alignment: Qt.AlignRight | Qt.AlignTop
                        onClicked: {
                            root.close()
                        }
                    }
                }

                StackLayout {
                    id: stackLayout
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    currentIndex: 0

                    Loader {
                        sourceComponent: paymentNodeViewModel ? (paymentNodeViewModel.type === Enums.PaymentNodeType.Lnd ? initialComponent : ethInitialComponent) : undefined
                    }
                }
            }
        }

        Component {
            id: channelsDialogComponent

            LNChannelsDetailsPopup {
                id: lNChannelsDetailsPopup
                pubKey: paymentNodeViewModel.stateModel.identifier

                onClosed: {
                    if (paymentNodeViewModel.stateModel.isChannelOpened
                            && checkPaymentNodeState(
                                paymentNodeViewModel.stateModel)) {
                        goToConfirmComponent(
                                    lightningSendViewModel.payRequest.numSatoshis,
                                    addressTo.text)
                    } else {
                        stackView.push(
                                    "qrc:/Components/OperationResultComponent.qml",
                                    {
                                        "operationMsg": "No opened channels",
                                        "resultMsg": "Failed!",
                                        "imgPath": "qrc:/images/crossIcon.png",
                                        "confirmBtnAction": function () {
                                            root.close()
                                        }
                                    })
                    }
                }
            }
        }

        Component {
            id: ethInitialComponent

            ColumnLayout {
                id: initialComponentLayout
                property var remainingBalance: calcRemainingBalance(
                                                   amountCoin.text,
                                                   includeFee, false)
                property bool isLightningAddress: paymentNodeViewModel.isLightningAddress(
                                                      addressTo.text)

                property var moneyToSend: undefined
                property bool txCreated: false
                property bool includeFee: false
                property var gasType: Enums.GasType.Average

                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 25

                Connections {
                    target: sendTransactionViewModel
                    function onTransactionCreatingFailed(errorMessage) {
                        notification.text = errorMessage
                        notification.color = SkinColors.transactionItemSent
                        txCreated = false
                    }

                    function onTransactionCreated(moneyToSend, networkFee) {
                        root.networkFee = networkFee
                        if (remainingBalance >= 0) {
                            notification.color = SkinColors.secondaryText
                            if (includeFee) {
                                //                                amountCoin.text = Utils.formatBalance(moneyToSend - networkFee)
                                amountCoin.text = Utils.formatBalance(
                                            moneyToSend)
                            }
                            notification.text = "REMAINING :"
                            initialComponentLayout.moneyToSend = moneyToSend
                            txCreated = true
                        } else {
                            notification.text = "Insufficient balance"
                            notification.color = SkinColors.transactionItemSent
                            txCreated = false
                        }
                        includeFee = false
                        amountCoin.visible = true
                    }
                }

                Connections {
                    target: lightningSendViewModel
                    function onPayRequestChanged() {
                        amountCoin.focus = true
                        amountCoin.text = Utils.formatBalance(
                                    lightningSendViewModel.payRequest.numSatoshis)
                        amountCoin.focus = false
                        notification.color = SkinColors.secondaryText
                        notification.text = "REMAINING :"
                        txCreated = true

                        amountCoin.enabled = false
                        maxAmoutCoinBtn.enabled = false
                        amountLocal.enabled = false
                    }

                    function onPayRequestError() {
                        notification.text = "Invalid payment request"
                        notification.color = SkinColors.transactionItemSent
                        amountCoin.text = ""
                        amountLocal.text = ""
                        txCreated = false
                        amountCoin.enabled = false
                        maxAmoutCoinBtn.enabled = false
                        amountLocal.enabled = false
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 24

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        StyledTextInput {
                            id: addressTo
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            placeholderText: "Address"

                            onTextChanged: {
                                txCreated = false
                                notification.text = ""

                                amountCoin.enabled = true
                                maxAmoutCoinBtn.enabled = true
                                amountLocal.enabled = true

                                if (amountCoin.text !== ""
                                        && addressTo.text !== "") {

                                    includeFee = amountCoin.text == currentBalanceOnChainStr
                                    createAccountTransaction(amountCoin.text,
                                                             addressTo.text,
                                                             false, gasType)
                                }

                                if (addressTo.text === "") {
                                    amountCoin.text = ""
                                    amountLocal.text = ""
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        spacing: 7

                        StyledTextInput {
                            id: amountCoin
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            placeholderText: "Amount"
                            validator: RegExpValidator {
                                regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                            }
                            unitValue: currentSymbol

                            onTextChanged: {
                                txCreated = false
                                notification.text = ""
                                if (text === ".") {
                                    amountCoin.text = "0."
                                }

                                if (parseFloat(text) <= 0 || isNaN(
                                            parseFloat(text))) {
                                    if (!amountLocal.focus) {
                                        amountLocal.text = ""
                                        return
                                    }
                                }

                                var newAmoutLocal = ApplicationViewModel.localCurrencyViewModel.convert(
                                            assetID, text)
                                if (!amountLocal.focus) {
                                    amountLocal.text = newAmoutLocal
                                }

                                if (addressTo.text !== "" && text !== ""
                                        && !paymentNodeViewModel.isLightningAddress(
                                            addressTo.text)) {
                                    includeFee = text == currentBalanceOnChainStr
                                    createAccountTransaction(amountCoin.text,
                                                             addressTo.text,
                                                             false, gasType)
                                }
                            }
                        }

                        StyledTextInput {
                            id: amountLocal
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            placeholderText: "Amount"
                            validator: RegExpValidator {
                                regExp: /^(([1-9]{1,})+[0-9]*(\.?[0-9]{0,2})$|0?(\.[0-9]{0,2}))$/gm
                            }
                            unitValue: currentCurrencyCode

                            onTextChanged: {
                                if (text === ".") {
                                    amountLocal.text = "0."
                                }

                                if (parseFloat(text) <= 0 || isNaN(
                                            parseFloat(text))) {
                                    if (!amountCoin.focus) {
                                        amountCoin.text = ""
                                        return
                                    }
                                }

                                var newAmoutCoin = ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                            assetID, text)
                                if (!amountCoin.focus) {
                                    amountCoin.text = newAmoutCoin
                                }
                            }
                        }

                        MaxButton {
                            id: maxAmoutCoinBtn
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 40
                            onClicked: {
                                amountCoin.focus = true
                                amountCoin.visible = false
                                amountCoin.text = currentBalanceOnChainStr
                                amountCoin.focus = false
                                includeFee = true
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        Layout.minimumHeight: 35

                        RowLayout {
                            Layout.fillWidth: true

                            XSNLabel {
                                text: "Gas type :"
                                font.family: regularFont.name
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: SkinColors.secondaryText
                            }

                            SelectedText {
                                id: networkFeeRateText
                                selectByMouse: networkFeeRate > 0
                                font.family: regularFont.name
                                font.pixelSize: 12
                                color: SkinColors.mainText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                text: choosingListView.currentOption
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: amountCoin.text != "" && parseFloat(
                                         amountCoin.text) > 0

                            XSNLabel {
                                text: "NETWORK FEE  :"
                                font.family: regularFont.name
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: SkinColors.secondaryText
                            }

                            SelectedText {
                                selectByMouse: networkFee
                                text: networkFee ? "%1 ETH (%2 %3)".arg(
                                                       Utils.formatBalance(
                                                           networkFee)).arg(
                                                       ApplicationViewModel.localCurrencyViewModel.convert(
                                                           60,
                                                           Utils.formatBalance(
                                                               networkFee))).arg(
                                                       currentCurrencyCode) : "Automatic"
                                font.family: regularFont.name
                                font.pixelSize: 12
                                color: SkinColors.mainText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    ChoosingListView {
                        id: choosingListView
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 25
                        color: SkinColors.mainBackground
                        highlightBorderColor: SkinColors.popupFieldBorder
                        highlighRectangleColor: SkinColors.menuBackground
                        model: ["Low", "Medium", "High"]
                        textItemPixelSize: 12
                        textItemCapitalization: Font.AllUppercase
                        currentIndex: 1
                        onCurrentOptionChanged: {
                            gasType = getGasType(currentOption)

                            if (amountCoin.text !== "" && addressTo.text !== ""
                                    && !paymentNodeViewModel.isLightningAddress(
                                        addressTo.text)) {
                                createAccountTransaction(amountCoin.text,
                                                         addressTo.text,
                                                         false, gasType)
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                    color: SkinColors.popupFieldBorder
                }

                RowLayout {
                    Layout.fillWidth: true

                    RowLayout {

                        XSNLabel {
                            id: notification
                            font.family: regularFont.name
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        SelectedText {
                            text: txCreated ? "%1  %2".arg(
                                                  Utils.formatBalance(
                                                      remainingBalance)).arg(
                                                  currentSymbol) : ""
                            font.family: regularFont.name
                            font.pixelSize: 12
                            color: SkinColors.mainText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    IntroButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        Layout.maximumWidth: 100
                        Layout.minimumWidth: 100
                        text: "Send"
                        Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                        enabled: txCreated
                        onClicked: {
                            if (addressTo.text === "") {
                                amountCoin.text = ""
                                amountLocal.text = ""
                            } else if (isLightningAddress) {
                                if (paymentNodeViewModel.stateModel.isChannelOpened) {
                                    stackView.push(confirmLightningComponent, {
                                                       "amount": lightningSendViewModel.payRequest.numSatoshis,
                                                       "invoice": addressTo.text,
                                                       "pubKey": lightningSendViewModel.payRequest.destination,
                                                       "confirmHandler": function () {
                                                           lightningSendViewModel.makePayment()
                                                       },
                                                       "cancelHandler": function () {
                                                           lightningSendViewModel.cancelPayment()
                                                       }
                                                   })
                                } else {
                                    stackView.push(noOpenedChannelsComponent)
                                }
                            } else {

                                if (amountCoin.text === "") {
                                    amountCoin.placeholderTextColor = SkinColors.transactionItemSent
                                } else {
                                    stackView.push(confirmComponent, {
                                                       "amount": initialComponentLayout.moneyToSend,
                                                       "fee": root.networkFee,
                                                       "address": addressTo.text,
                                                       "confirmHandler": function () {
                                                           sendTransactionViewModel.confirmSending()
                                                       },
                                                       "cancelHandler": function () {
                                                           sendTransactionViewModel.cancelSending()
                                                       }
                                                   })
                                }
                            }
                        }
                    }
                }
            }
        }

        Component {
            id: initialComponent

            ColumnLayout {
                id: initialComponentLayout
                property var remainingBalance: calcRemainingBalance(
                                                   amountCoin.text, includeFee,
                                                   isLightningAddress)
                property bool isLightningAddress: paymentNodeViewModel.isLightningAddress(
                                                      addressTo.text)

                property var moneyToSend: undefined
                property bool txCreated: false
                property bool includeFee: false

                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 25

                Connections {
                    target: sendTransactionViewModel
                    function onTransactionCreatingFailed(errorMessage) {
                        notification.text = errorMessage
                        notification.color = SkinColors.transactionItemSent
                        txCreated = false
                    }

                    function onTransactionCreated(moneyToSend, networkFee) {
                        notification.color = SkinColors.secondaryText
                        notification.text = "REMAINING :"
                        root.networkFee = networkFee
                        initialComponentLayout.moneyToSend = moneyToSend
                        txCreated = true
                    }
                }

                Connections {
                    target: lightningSendViewModel
                    function onPayRequestChanged() {
                        amountCoin.focus = true
                        amountCoin.text = Utils.formatBalance(
                                    lightningSendViewModel.payRequest.numSatoshis)
                        amountCoin.focus = false
                        notification.color = SkinColors.secondaryText
                        notification.text = "REMAINING :"
                        txCreated = true

                        amountCoin.enabled = false
                        maxAmoutCoinBtn.enabled = false
                        amountLocal.enabled = false
                    }

                    function onPayRequestError() {
                        notification.text = "Invalid payment request"
                        notification.color = SkinColors.transactionItemSent
                        amountCoin.text = ""
                        amountLocal.text = ""
                        txCreated = false
                        amountCoin.enabled = false
                        maxAmoutCoinBtn.enabled = false
                        amountLocal.enabled = false
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    color: SkinColors.mainBackground

                    Text {
                        anchors.fill: parent
                        font.pixelSize: 12
                        font.family: regularFont.name
                        color: "#F29F1C"
                        text: "Lightning Network funds can only be spent on the Lightning Network. \nTo withdraw, please close your channels and the funds will move into your main wallet."
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 24

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        StyledTextInput {
                            id: addressTo
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            placeholderText: "Address or Payment request"

                            onTextChanged: {
                                txCreated = false
                                notification.text = ""
                                if (paymentNodeViewModel.isLightningAddress(
                                            addressTo.text)) {
                                    lightningSendViewModel.decodePayRequest(
                                                addressTo.text)
                                }

                                if (!paymentNodeViewModel.isLightningAddress(
                                            addressTo.text)) {
                                    amountCoin.enabled = true
                                    maxAmoutCoinBtn.enabled = true
                                    amountLocal.enabled = true

                                    if (amountCoin.text !== ""
                                            && addressTo.text !== "") {

                                        includeFee = amountCoin.text == currentBalanceOnChainStr

                                        createUTXOTransaction(amountCoin.text,
                                                              addressTo.text,
                                                              includeFee,
                                                              networkFeeRate)
                                    }
                                }

                                if (addressTo.text === "") {
                                    amountCoin.text = ""
                                    amountLocal.text = ""
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        spacing: 7

                        StyledTextInput {
                            id: amountCoin
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            placeholderText: "Amount"
                            validator: RegExpValidator {
                                regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                            }
                            unitValue: currentSymbol

                            onTextChanged: {
                                txCreated = false
                                notification.text = ""
                                if (text === ".") {
                                    amountCoin.text = "0."
                                }

                                if (parseFloat(text) <= 0 || isNaN(
                                            parseFloat(text))) {
                                    if (!amountLocal.focus) {
                                        amountLocal.text = ""
                                        return
                                    }
                                }

                                var newAmoutLocal = ApplicationViewModel.localCurrencyViewModel.convert(
                                            assetID, text)
                                if (!amountLocal.focus) {
                                    amountLocal.text = newAmoutLocal
                                }

                                if (addressTo.text !== "" && text !== ""
                                        && !paymentNodeViewModel.isLightningAddress(
                                            addressTo.text)) {
                                    includeFee = text == currentBalanceOnChainStr
                                    createUTXOTransaction(amountCoin.text,
                                                          addressTo.text,
                                                          includeFee,
                                                          networkFeeRate)
                                }
                            }
                        }

                        StyledTextInput {
                            id: amountLocal
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            placeholderText: "Amount"
                            validator: RegExpValidator {
                                regExp: /^(([1-9]{1,})+[0-9]*(\.?[0-9]{0,2})$|0?(\.[0-9]{0,2}))$/gm
                            }
                            unitValue: currentCurrencyCode

                            onTextChanged: {
                                if (text === ".") {
                                    amountLocal.text = "0."
                                }

                                if (parseFloat(text) <= 0 || isNaN(
                                            parseFloat(text))) {
                                    if (!amountCoin.focus) {
                                        amountCoin.text = ""
                                        return
                                    }
                                }

                                var newAmoutCoin = ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                            assetID, text)
                                if (!amountCoin.focus) {
                                    amountCoin.text = newAmoutCoin
                                }
                            }
                        }

                        MaxButton {
                            id: maxAmoutCoinBtn
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 40
                            onClicked: {
                                amountCoin.focus = true
                                amountCoin.text = currentBalanceOnChainStr
                                amountCoin.focus = false
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: !isLightningAddress
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        Layout.minimumHeight: 35

                        RowLayout {
                            Layout.fillWidth: true

                            XSNLabel {
                                text: "NETWORK FEE RATE :"
                                font.family: regularFont.name
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: SkinColors.secondaryText
                                visible: !isLightningAddress
                            }

                            SelectedText {
                                id: networkFeeRateText
                                selectByMouse: networkFeeRate > 0
                                font.family: regularFont.name
                                font.pixelSize: 12
                                color: SkinColors.mainText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                visible: !isLightningAddress
                                text: networkFeeRate > 0 ? "%1  %2".arg(
                                                               networkFeeRate).arg(
                                                               "satoshi/vbyte") : "Automatic"
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: !isLightningAddress
                                     && amountCoin.text != "" && parseFloat(
                                         amountCoin.text) > 0

                            XSNLabel {
                                text: "NETWORK FEE  :"
                                font.family: regularFont.name
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: SkinColors.secondaryText
                            }

                            SelectedText {
                                selectByMouse: networkFee
                                text: networkFee ? "%1  %2 (%3 %4)".arg(
                                                       Utils.formatBalance(
                                                           networkFee)).arg(
                                                       currentSymbol).arg(
                                                       ApplicationViewModel.localCurrencyViewModel.convert(
                                                           assetID,
                                                           Utils.formatBalance(
                                                               networkFee))).arg(
                                                       currentCurrencyCode) : "Automatic"
                                font.family: regularFont.name
                                font.pixelSize: 12
                                color: SkinColors.mainText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    ChoosingListView {
                        id: choosingListView
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 25
                        color: SkinColors.mainBackground
                        highlightBorderColor: SkinColors.popupFieldBorder
                        highlighRectangleColor: SkinColors.menuBackground
                        model: ["Low", "Medium", "High"]
                        textItemPixelSize: 12
                        textItemCapitalization: Font.AllUppercase
                        currentIndex: 1
                        onCurrentOptionChanged: {
                            root.networkFeeRate = calculateNetworkFeeRate(
                                        currentOption,
                                        recommendedNetworkFeeRate)

                            if (amountCoin.text !== "" && addressTo.text !== ""
                                    && !paymentNodeViewModel.isLightningAddress(
                                        addressTo.text)) {
                                createUTXOTransaction(amountCoin.text,
                                                      addressTo.text,
                                                      includeFee,
                                                      networkFeeRate)
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                    color: SkinColors.popupFieldBorder
                }

                RowLayout {
                    Layout.fillWidth: true

                    RowLayout {

                        XSNLabel {
                            id: notification
                            font.family: regularFont.name
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        SelectedText {
                            text: txCreated ? "%1  %2".arg(
                                                  Utils.formatBalance(
                                                      remainingBalance)).arg(
                                                  currentSymbol) : ""
                            font.family: regularFont.name
                            font.pixelSize: 12
                            color: SkinColors.mainText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    IntroButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        Layout.maximumWidth: 100
                        Layout.minimumWidth: 100
                        text: "Send"
                        Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                        enabled: txCreated
                        onClicked: {
                            if (addressTo.text === "") {
                                amountCoin.text = ""
                                amountLocal.text = ""
                            } else if (isLightningAddress) {
                                if (paymentNodeViewModel.stateModel.isChannelOpened) {
                                    stackView.push(confirmLightningComponent, {
                                                       "amount": lightningSendViewModel.payRequest.numSatoshis,
                                                       "invoice": addressTo.text,
                                                       "pubKey": lightningSendViewModel.payRequest.destination,
                                                       "confirmHandler": function () {
                                                           lightningSendViewModel.makePayment()
                                                       },
                                                       "cancelHandler": function () {
                                                           lightningSendViewModel.cancelPayment()
                                                       }
                                                   })
                                } else {
                                    stackView.push(noOpenedChannelsComponent)
                                }
                            } else {

                                if (amountCoin.text === "") {
                                    amountCoin.placeholderTextColor = SkinColors.transactionItemSent
                                } else {
                                    stackView.push(confirmComponent, {
                                                       "amount": initialComponentLayout.moneyToSend,
                                                       "fee": root.networkFee,
                                                       "address": addressTo.text,
                                                       "confirmHandler": function () {
                                                           sendTransactionViewModel.confirmSending()
                                                       },
                                                       "cancelHandler": function () {
                                                           sendTransactionViewModel.cancelSending()
                                                       }
                                                   })
                                }
                            }
                        }
                    }
                }
            }
        }

        Component {
            id: noOpenedChannelsComponent

            ColumnLayout {
                property bool autoPilotEnabled: false
                property string errorMessage: autoPilotEnabled ? "There are no open channels, you can open it manually or wait for autopilot to do it  !" : "There are no open channels, you can do it manually or launch autopilot !"
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 30

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 40

                    XSNLabel {
                        Layout.alignment: Qt.AlignHCenter
                        font.family: mediumFont.name
                        font.pixelSize: 22
                        text: "Attention!"
                        color: "red"
                    }

                    Item {
                        Layout.preferredHeight: 50
                        Layout.preferredWidth: 410
                        Layout.alignment: Qt.AlignHCenter

                        XSNLabel {
                            anchors.centerIn: parent
                            horizontalAlignment: Text.AlignHCenter
                            font.family: mediumFont.name
                            width: parent.width
                            height: parent.height
                            font.pixelSize: 16
                            text: errorMessage
                            wrapMode: Text.WordWrap
                            elide: Text.ElideRight
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                    spacing: 10

                    IntroButton {
                        Layout.preferredWidth: 210
                        Layout.preferredHeight: 45
                        text: qsTr("Launch Autopilot")
                        enabled: false
                        visible: false

                        onClicked: {
                            paymentNodeViewModel.activateAutopilot(true)
                        }
                    }

                    IntroButton {
                        Layout.preferredWidth: 210
                        Layout.preferredHeight: 45
                        text: qsTr("Open Channels")

                        onClicked: openDialog(channelsDialogComponent, {
                                                  "assetID": assetID
                                              })
                    }
                }

                Connections {
                    target: paymentNodeViewModel.stateModel

                    function onLndChannelsOpenedChanged() {
                        if (paymentNodeViewModel.stateModel.isChannelOpened) {
                            stackView.push(confirmComponent, {
                                               "amount": lightningSendViewModel.payRequest.numSatoshis,
                                               "fee": "0",
                                               "address": addressTo.text,
                                               "confirmHandler": function () {
                                                   lightningSendViewModel.makePayment()
                                               },
                                               "cancelHandler": function () {
                                                   lightningSendViewModel.cancelPayment()
                                               }
                                           })
                        }
                    }
                }
            }
        }

        Component {
            id: confirmComponent

            ColumnLayout {
                id: layout
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignCenter
                spacing: 15

                property var amount: undefined
                property real fee: undefined
                property var address: undefined
                property var confirmHandler
                property var cancelHandler

                Column {
                    Layout.fillWidth: true
                    Layout.topMargin: 15
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 180
                    spacing: 7

                    XSNLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.family: mediumFont.name
                        font.pixelSize: 22
                        text: "Are you sure you want to send "
                    }

                    XSNLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.family: mediumFont.name
                        font.pixelSize: 22
                        color: SkinColors.sendPopupConfirmText
                        text: "%1 %2".arg(Utils.formatBalance(
                                              layout.amount)).arg(currentSymbol)
                    }

                    XSNLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.family: mediumFont.name
                        font.pixelSize: 22
                        text: "to"
                    }

                    XSNLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        width: parent.width
                        font.family: mediumFont.name
                        font.pixelSize: 20
                        text: '<font color="%1"> %2 </font> ?'.arg(
                                  SkinColors.sendPopupConfirmText).arg(
                                  layout.address)
                    }

                    XSNLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.family: mediumFont.name
                        font.pixelSize: 22
                        text: '%1 <font color="%2"> %3  %4</font> '.arg(
                                  "Network fee: ").arg(
                                  SkinColors.sendPopupConfirmText).arg(
                                  Utils.formatBalance(layout.fee)).arg(
                                  currentSymbol)
                        visible: parseFloat(layout.fee) > 0
                    }
                }

                RowLayout {
                    Layout.fillHeight: true
                    Layout.preferredHeight: 50
                    Layout.alignment: Qt.AlignCenter
                    spacing: 32

                    IntroButton {
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 50
                        text: qsTr("Cancel")
                        onClicked: {
                            layout.cancelHandler()
                            stackView.pop()
                        }
                    }

                    IntroButton {
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 50
                        font.family: mediumFont.name
                        text: qsTr("Send")
                        onClicked: {
                            layout.confirmHandler()
                            stackView.push(
                                        "qrc:/Components/PopupProgressBarComponent.qml",
                                        {
                                            "progressBarMsg": "Sending transaction ..."
                                        })
                        }
                    }
                }
            }
        }

        Component {
            id: confirmLightningComponent

            ColumnLayout {
                id: layout
                Layout.alignment: Qt.AlignCenter
                spacing: 15

                property real amount: undefined
                property var invoice: undefined
                property var pubKey: undefined
                property var confirmHandler
                property var cancelHandler

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                }

                Column {
                    Layout.fillWidth: true
                    Layout.topMargin: 15
                    Layout.alignment: Qt.AlignCenter
                    Layout.fillHeight: true
                    spacing: 10

                    XSNLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        verticalAlignment: Text.AlignVCenter
                        font.family: mediumFont.name
                        font.pixelSize: 22
                        text: "Are you sure you want to send "
                    }

                    XSNLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        verticalAlignment: Text.AlignVCenter
                        font.family: mediumFont.name
                        font.pixelSize: 20
                        color: SkinColors.sendPopupConfirmText
                        text: '<font color="%1"> %2 %3 </font> ?'.arg(
                                  SkinColors.sendPopupConfirmText).arg(
                                  Utils.formatBalance(layout.amount)).arg(
                                  currentSymbol)
                    }

                    XSNLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        verticalAlignment: Text.AlignVCenter
                        width: parent.width
                        font.family: mediumFont.name
                        wrapMode: Text.WrapAnywhere
                        font.pixelSize: 12
                        text: 'Pubkey: <font color="%1"> %2 </font>'.arg(
                                  SkinColors.sendPopupConfirmText).arg(
                                  layout.pubKey)
                    }
                }

                RowLayout {
                    Layout.preferredHeight: 50
                    Layout.alignment: Qt.AlignCenter
                    spacing: 32

                    IntroButton {
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 50
                        text: qsTr("Cancel")
                        onClicked: {
                            layout.cancelHandler()
                            stackView.pop()
                        }
                    }

                    IntroButton {
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 50
                        font.family: mediumFont.name
                        text: qsTr("Send")
                        onClicked: {
                            layout.confirmHandler()
                            stackView.push(
                                        "qrc:/Components/PopupProgressBarComponent.qml",
                                        {
                                            "progressBarMsg": "Sending transaction ..."
                                        })
                        }
                    }
                }
            }
        }
    }

    function createUTXOTransaction(amountCoins, address, isFeeIncluding, feeRate) {
        sendTransactionViewModel.createSendTransaction({
                                                           "type": "utxo",
                                                           "userSelectedFeeRate": feeRate,
                                                           "substractFeeFromAmount": isFeeIncluding,
                                                           "addressTo": address,
                                                           "amount": amountCoins
                                                       })
    }

    function createAccountTransaction(amountCoins, address, isFeeIncluding, gasType) {
        sendTransactionViewModel.createSendTransaction({
                                                           "type": "account",
                                                           "addressTo": address,
                                                           "amount": amountCoins,
                                                           "gasType": gasType
                                                       })
    }

    function calcRemainingBalance(strValue, includeFee, isLightning) {
        var sendingAmount = Math.floor(parseFloat(strValue) * 100000000)
        var currentBalance = isLightning ? currentLndBalance : currentBalanceOnChain
        if (includeFee) {
            return currentBalance - sendingAmount
        } else {
            return currentBalance - sendingAmount - networkFee
        }
    }

    function goToConfirmComponent(numSatoshiValue, paymentRequset) {

        stackView.push(confirmComponent, {
                           "amount": numSatoshiValue,
                           "fee": "0",
                           "address": paymentRequset,
                           "confirmHandler": function () {
                               lightningSendViewModel.makePayment()
                           },
                           "cancelHandler": function () {
                               lightningSendViewModel.cancelPayment()
                           }
                       })
    }
}
