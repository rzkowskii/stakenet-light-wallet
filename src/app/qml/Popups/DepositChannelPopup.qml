import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    property int assetID: 0
    property string channelAddress: ""
    property int currentIndex: -1
    property string currentSymbol: assetModel.get(currentIndex).symbol
    property double currentBalanceOnChain: assetModel.get(
                                               currentIndex).balanceOnChain

    popUpText: "Deposit channel"
    width: 600
    height: 330
    clip: true

    function refreshBalances() {
        var asset = assetModel.get(currentIndex)
        currentBalanceOnChain = asset.balanceOnChain
    }

    WalletAssetsListModel {
        id: assetModel

        Component.onCompleted: {
            initialize(ApplicationViewModel)
            root.currentIndex = getInitial(assetID)
        }
        onDataChanged: refreshBalances()
    }

    PaymentNodeViewModel {
        id: pNodeViewModel
        currentAssetID: assetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
        onDepositChannelFinished: function (channelAddress) {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": "Channel deposit was successful!",
                               "resultMsg"// to do
                               : "Success!",
                               "imgPath": "qrc:/images/SUCCESS_ICON.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }

        onDepositChannelFailed: function (errMsg) {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": "Channel deposit failed: %1".arg(
                                                   errMsg),
                               "resultMsg": "Failed!",
                               "imgPath": "qrc:/images/crossIcon.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }
    }

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.margins: 20
        initialItem: initialComponent

        ColumnLayout {
            id: initialComponent
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 25

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                XSNLabel {
                    font.family: mediumFont.name
                    font.pixelSize: 18
                    text: "Deposit channel"
                    color: SkinColors.mainText
                    font.capitalization: Font.Capitalize
                }

                Item {
                    Layout.fillWidth: true
                }

                CloseButton {
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 30
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    onClicked: root.close()
                }
            }

            Row {
                spacing: 15

                SecondaryLabel {
                    text: qsTr("Channel address: ")
                    font.pixelSize: 12
                }

                SelectedText {
                    text: channelAddress
                    font.pixelSize: 13
                    font.family: fontRegular.name
                    color: SkinColors.mainText
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 41
                spacing: 8

                StyledTextInput {
                    id: amount
                    Layout.fillWidth: true
                    Layout.preferredHeight: 41
                    placeholderText: "Enter amount" // (min = %1, max = %2)".arg(
                    //                                                 Utils.formatBalance(
                    //                                                     comboBox.currentMinLndCapacity)).arg(
                    //                                                 Utils.formatBalance(
                    //                                                     comboBox.currentConfirmedBalanceOnChain))
                    validator: RegExpValidator {
                        regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                    }
                    unitValue: currentSymbol
                    onTextChanged: {
                        if (text === ".") {
                            text = "0."
                        }
                        if (parseFloat(text) <= 0 || isNaN(parseFloat(text))) {
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
                        stackView.allowAmount()
                    }
                }

                StyledTextInput {
                    id: amountLocal
                    Layout.preferredWidth: 135
                    Layout.preferredHeight: 41
                    placeholderText: "Amount"
                    validator: RegExpValidator {
                        regExp: /^(([1-9]{1,})+[0-9]*(\.?[0-9]{0,2})$|0?(\.[0-9]{0,2}))$/gm
                    }
                    unitValue: ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode

                    onTextChanged: {
                        if (text === ".") {
                            amountLocal.text = "0."
                        }

                        if (parseFloat(text) <= 0 || isNaN(parseFloat(text))) {
                            if (!amount.focus) {
                                amount.text = ""
                                return
                            }
                        }

                        var newAmoutCoin = ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                    assetID, text)
                        if (!amount.focus) {
                            amount.text = newAmoutCoin
                        }
                        stackView.allowAmount()
                    }
                }
            }

            ColumnLayout {
                Layout.leftMargin: 5
                Layout.fillWidth: true
                spacing: 3

                Text {
                    id: ethNotification
                    property bool isConnextEnabled: ApplicationViewModel.paymentNodeViewModel.paymentNodeActivity(
                                                        assetID)
                    color: SkinColors.transactionItemSent
                    visible: text.length > 0
                    font.pixelSize: 12
                    font.family: fontRegular.name
                    text: !isConnextEnabled ? "%1 Connext is not enabled. If you want to deposit channel please enable %1 Connext.".arg(
                                                  currentSymbol) : checkPaymentNodeState(
                                                  pNodeViewModel.stateModel) ? "" : "%1 Connext is syncing. Please wait for it to become active.".arg(currentSymbol)
                }

                Text {
                    id: amountMessage
                    color: SkinColors.transactionItemSent
                    visible: text.length > 0
                    font.pixelSize: 12
                    font.family: fontRegular.name
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                IntroButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    Layout.maximumWidth: 80
                    Layout.minimumWidth: 80
                    text: "Deposit"
                    Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                    enabled: amountMessage.text === ""
                             && ethNotification.text === ""
                    onClicked: {
                        var isAmountEmpty = amount.text.length === 0
                        if (isAmountEmpty) {
                            if (isAmountEmpty) {
                                amountMessage.text = "Enter channel capacity!"
                            }
                            return
                        }

                        stackView.push(
                                    "qrc:/Components/PopupProgressBarComponent.qml",
                                    {
                                        "progressBarMsg": "Deposit to channel ..."
                                    })
                        pNodeViewModel.depositChannel(amount.text,
                                                      channelAddress)
                    }
                }
            }
        }

        function allowAmount() {
            if (amount.text === "") {
                amountMessage.text = ""
                return
            }

            var amountSats = Utils.parseCoinsToSatoshi(amount.text)

            if (amountSats < 0) {
                amountMessage.text = "Amount cannot be less than %1 %2.".arg(
                            "0").arg(currentSymbol)
            } else if (amountSats > currentBalanceOnChain) {
                amountMessage.text = "Amount cannot be greater than confirmed on chain balance."
            } else {
                amountMessage.text = ""
            }
        }
    }
}
