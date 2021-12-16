import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root

    property int assetID: 0
    property int currentIndex: -1
    property string currentSymbol: assetModel.get(currentIndex).symbol
    property string currentBalanceOnChain: Utils.formatBalance(
                                               assetModel.get(
                                                   currentIndex).balanceOnChain)
    property string currentName: assetModel.get(currentIndex).name
    property var receivingAddresses: undefined
    property var currentMinPaymentAmount: assetModel.get(
                                              currentIndex).minPaymentAmount
    property var currentMaxPaymentAmount: assetModel.get(
                                              currentIndex).maxPaymentAmount

    property string currentLndBalanceStr: Utils.formatBalance(
                                              paymentNodeViewModel.stateModel.nodeLocalRemoteBalances.allRemoteBalance)

    property bool isToken: walletViewModel.assetInfo.isToken

    property bool isSyncing: isPaymentNodeSyncing(
                                 paymentNodeViewModel.stateModel)
    property bool isNotRunning: isPaymentNodeNotRunning(
                                    paymentNodeViewModel.stateModel)

    Component.onCompleted: {
        loadingPaymentTypeModel()
    }

    Binding {
        target: ptCombobox.item
        property: "model"
        value: paymentsTypes
        when: ptCombobox.status == Loader.Ready
    }

    Connections {
        target: walletViewModel.receiveTxViewModel
        function onAllKnownAddressByIdGenerated(addresses) {
            receivingAddresses = addresses
        }
    }

    WalletAssetsListModel {
        id: assetModel

        Component.onCompleted: {
            initialize(ApplicationViewModel)
            root.currentIndex = getInitial(assetID)
        }
        onDataChanged: {
            var asset = assetModel.get(currentIndex)
            currentBalanceOnChain = Utils.formatBalance(asset.balanceOnChain)
        }
    }

    LightningSendTransactionViewModel {
        id: lightningSendViewModel

        Component.onCompleted: {
            initialize(assetID, ApplicationViewModel)
        }
    }

    PaymentNodeViewModel {
        id: paymentNodeViewModel
        currentAssetID: assetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    WalletAssetViewModel {
        id: walletViewModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
            walletViewModel.receiveTxViewModel.requestAllKnownAddressesById()
        }
        currentAssetID: root.assetID
    }

    Connections {
        target: lightningSendViewModel

        function onPayRequestReady(payReq) {
            payReqLayout.visible = true
            payReqTextField.text = payReq
        }

        function onTransactionSendingFailed() {
            payReqTextField.clear()
        }
    }

    ListModel {
        id: paymentsTypes
    }

    popUpText: "Receive"
    width: 550
    height: ptCombobox.visible ? 386 : 336

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 22
        anchors.bottomMargin: 22
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 10

        FontLoader {
            id: mediumFont
            source: "qrc:/Rubik-Medium.ttf"
        }
        FontLoader {
            id: regularFont
            source: "qrc:/Rubik-Regular.ttf"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

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
                    text: "Receive %1 - %2".arg(currentName).arg(currentSymbol)
                    color: SkinColors.mainText
                    elide: Text.ElideRight
                    width: parent.width
                }

                StackLayout {
                    currentIndex: ptCombobox.currentIndex === 0 ? 0 : 1

                    RowLayout {
                        spacing: 0

                        XSNLabel {
                            text: "On-chain  :  "
                            font.family: regularFont.name
                            font.pixelSize: 14
                            color: SkinColors.secondaryText
                            font.capitalization: Font.AllUppercase
                        }

                        SelectedText {
                            text: "%1 %2 = %3 %4".arg(
                                      mainPage.balanceVisible ? currentBalanceOnChain : hideBalance(
                                                                    currentBalanceOnChain)).arg(
                                      currentSymbol).arg(
                                      mainPage.balanceVisible ? ApplicationViewModel.localCurrencyViewModel.convert(assetID, currentBalanceOnChain) : hideBalance(ApplicationViewModel.localCurrencyViewModel.convert(assetID, currentBalanceOnChain))).arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                            font.family: regularFont.name
                            font.pixelSize: 14
                            color: SkinColors.mainText
                        }
                    }

                    RowLayout {
                        spacing: 0

                        XSNLabel {
                            text: "Lightning  :  "
                            font.family: regularFont.name
                            font.pixelSize: 14
                            color: SkinColors.secondaryText
                            font.capitalization: Font.AllUppercase
                        }

                        SelectedText {
                            text: "%1 %2 = %3 %4".arg(currentLndBalanceStr).arg(
                                      currentSymbol).arg(
                                      ApplicationViewModel.localCurrencyViewModel.convert(
                                          assetID, currentLndBalanceStr)).arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                            font.family: regularFont.name
                            font.pixelSize: 14
                            color: SkinColors.mainText
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
                onClicked: root.close()
            }
        }

        Loader {
            id: ptCombobox
            asynchronous: true
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.topMargin: 5
            Layout.bottomMargin: 5
            source: paymentNodeViewModel.type
                    === Enums.PaymentNodeType.Lnd ? "qrc:/Components/PaymentTypeCombobox.qml" : ""
            visible: paymentsTypes.count > 0 && status == Loader.Ready
            property var model: paymentsTypes
            property int currentIndex: 0
        }

        StackLayout {
            currentIndex: ptCombobox.item ? ptCombobox.item.currentIndex : 0

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 24

                WarningComponent {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    message: qsTr("Send only %1%2 to this address. Sending any other digital asset will result in permanent loss.").arg(
                                 isToken ? "ERC-20 " : "").arg(currentSymbol)
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        color: "transparent"
                        border.color: SkinColors.popupFieldBorder
                        border.width: 1

                        CustomizedComboBox {
                            id: addressesComboBox
                            anchors.fill: parent
                            font.family: regularFont.name
                            font.pixelSize: 14
                            background: Rectangle {
                                color: "transparent"
                            }

                            model: receivingAddresses
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        color: "transparent"
                        border.color: copyMouseArea.containsMouse ? SkinColors.headerText : SkinColors.popupFieldBorder
                        border.width: 1

                        Button {
                            anchors.fill: parent
                            background: Rectangle {
                                color: "transparent"
                            }

                            Image {
                                anchors.fill: parent
                                anchors.margins: 10
                                anchors.centerIn: parent
                                source: "qrc:/images/COPY ICONS.png"
                            }

                            PointingCursorMouseArea {
                                id: copyMouseArea
                                onClicked: {
                                    Clipboard.setText(
                                                addressesComboBox.currentText)
                                    showBubblePopup("Copied")
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    XSNLabel {
                        Layout.fillWidth: true
                        color: SkinColors.popupInfoText
                        font.pixelSize: 14
                        text: qsTr(
                                  "•  Coins will be deposited after %1 network confirmations.".arg(
                                      assetModel.get(
                                          currentIndex).confirmationsForApproved))
                    }

                    XSNLabel {
                        Layout.fillWidth: true
                        color: SkinColors.popupInfoText
                        font.pixelSize: 14
                        text: qsTr("•  You can check the progress on transactions page.")
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 13

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    spacing: 8

                    StyledTextInput {
                        id: amountCoin
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        placeholderText: "Amount"
                        validator: RegExpValidator {
                            regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                        }
                        unitValue: currentSymbol
                        onTextChanged: {
                            if (text === ".") {
                                text = "0."
                            }
                            if (parseFloat(text) <= 0 || isNaN(
                                        parseFloat(text))) {
                                if (!amountLocal.focus) {
                                    amountLocal.text = ""
                                    return
                                }
                            }
                            var newAmoutLocal = ApplicationViewModel.localCurrencyViewModel.convert(
                                        root.assetID, text)
                            if (!amountLocal.focus) {
                                amountLocal.text = newAmoutLocal
                            }
                        }
                    }

                    StyledTextInput {
                        id: amountLocal
                        Layout.preferredWidth: 135
                        Layout.preferredHeight: 35
                        placeholderText: "Amount"
                        validator: RegExpValidator {
                            regExp: /^(([1-9]{1,})+[0-9]*(\.?[0-9]{0,2})$|0?(\.[0-9]{0,2}))$/gm
                        }
                        unitValue: ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode

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
                                        root.assetID, text)
                            if (!amountCoin.focus) {
                                amountCoin.text = newAmoutCoin
                            }
                        }
                    }
                }

                XSNLabel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: SkinColors.transactionItemSent
                    font.pixelSize: 16
                    visible: isSyncing || isNotRunning
                    text: qsTr("• L2 is %1 •".arg(
                                   isSyncing ? "syncing" : "not running"))
                }

                RowLayout {
                    id: payReqLayout
                    Layout.fillWidth: true
                    spacing: 8

                    visible: false

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        color: "transparent"
                        border.color: SkinColors.popupFieldBorder
                        border.width: 1

                        TextField {
                            id: payReqTextField
                            anchors.fill: parent
                            placeholderTextColor: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 14
                            readOnly: true

                            background: Rectangle {
                                color: "transparent"
                            }
                            focus: true
                            color: SkinColors.mainText
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        color: "transparent"
                        border.color: SkinColors.popupFieldBorder
                        border.width: 1

                        Button {
                            anchors.fill: parent
                            background: Rectangle {
                                color: "transparent"
                            }

                            Image {
                                anchors.fill: parent
                                anchors.margins: 10
                                anchors.centerIn: parent
                                source: "qrc:/images/COPY ICONS.png"
                            }

                            PointingCursorMouseArea {
                                onClicked: {
                                    Clipboard.setText(payReqTextField.text)
                                    showBubblePopup("Copied")
                                }
                            }
                        }
                    }
                }

                IntroButton {
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 40
                    Layout.alignment: Qt.AlignRight
                    Layout.topMargin: payReqLayout.visible ? 0 : 50
                    font.family: mediumFont.name
                    text: qsTr("Receive")
                    enabled: checkPaymentNodeState(
                                 paymentNodeViewModel.stateModel)
                    onClicked: {
                        if (Utils.parseCoinsToSatoshi(
                                    amountCoin.text) > currentMaxPaymentAmount) {
                            showBubblePopup(
                                        qsTr("Payment of %1 %2 is too large, max payment allowed is %3 %2").arg(
                                            amountCoin.text).arg(
                                            currentSymbol).arg(
                                            Utils.formatBalance(
                                                currentMaxPaymentAmount)))
                            return
                        }
                        if (Utils.parseCoinsToSatoshi(
                                    amountCoin.text) < currentMinPaymentAmount) {
                            showBubblePopup(
                                        qsTr("Payment of %1 %2 is too small, min payment allowed is %3 %2").arg(
                                            amountCoin.text).arg(
                                            currentSymbol).arg(
                                            Utils.formatBalance(
                                                currentMinPaymentAmount)))
                            return
                        }

                        lightningSendViewModel.addInvoice(
                                    Utils.parseCoinsToSatoshi(amountCoin.text))
                    }
                }
            }
        }
    }

    function loadingPaymentTypeModel() {
        if (assetID == 0) {
            paymentsTypes.append({
                                     "name": "Bitcoin",
                                     "symbol": "BTC",
                                     "id": assetID
                                 })
            paymentsTypes.append({
                                     "name": "Lightning",
                                     "symbol": "BTC",
                                     "id": assetID
                                 })
        }
        if (assetID == 2) {
            paymentsTypes.append({
                                     "name": "Litecoin",
                                     "symbol": "LTC",
                                     "id": assetID
                                 })
            paymentsTypes.append({
                                     "name": "Lightning",
                                     "symbol": "LTC",
                                     "id": assetID
                                 })
        }
        if (assetID == 384) {
            paymentsTypes.append({
                                     "name": "Stakenet",
                                     "symbol": "XSN",
                                     "id": assetID
                                 })
            paymentsTypes.append({
                                     "name": "Lightning",
                                     "symbol": "XSN",
                                     "id": assetID
                                 })
        }
    }
}
