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

    popUpText: "Open channel"
    width: 700
    height: 460
    clip: true
    property double recommendedNetworkFeeRate: 0
    property string networkFeeRateCurrentOption: ""
    property double networkFeeRate: 0

    onRecommendedNetworkFeeRateChanged: {
        root.networkFeeRate = calculateNetworkFeeRate(
                    networkFeeRateCurrentOption ? networkFeeRateCurrentOption : "Medium",
                    recommendedNetworkFeeRate)
    }

    PaymentNodeViewModel {
        id: pNodeViewModel
        currentAssetID: comboBox.currentAssetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    OpenChannelViewModel {
        id: openChannelViewModel
        currentAssetID: comboBox.currentAssetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
        onChannelOpened: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": "Channel was opened!",
                               "resultMsg": "Success!",
                               "imgPath": "qrc:/images/SUCCESS_ICON.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }
        onRequestCreated: {
            stackView.push(confirmComponent, {
                               "networkFee": networkFee,
                               "deployFee": deployFee,
                               "symbol": comboBox.currentSymbol,
                               "pubkey": identityKey,
                           })
        }

        onRequestCreatingFailed: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": errorMessage,
                               "resultMsg": "Failed!",
                               "imgPath": "qrc:/images/crossIcon.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }

        onChannelOpeningFailed: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": errorMessage,
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

    Component {
        id: lnChannelsFeeComponent

        LNChannelsFeesPopup {}
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

            CoinsCombobox {
                id: comboBox

                model: QMLSortFilterListProxyModel {
                    id: sortedAssetModel
                    source: WalletAssetsListModel {
                        id: assetModel
                        Component.onCompleted: {
                            initialize(ApplicationViewModel)
                            averageFeeForAsset(assetID)
                        }

                        onCountChanged: {
                            for (var i = 0; i < sortedAssetModel.count; i++) {
                                if (sortedAssetModel.get(i).id === assetID) {
                                    comboBox.currentIndex = i
                                    return
                                }
                            }
                        }

                        onAverageFeeForAssetFinished: {
                            recommendedNetworkFeeRate = value
                        }
                    }
                    filterRole: "isActive"
                    filterString: "1"
                    filterCaseSensitivity: Qt.CaseInsensitive
                }

                onCurrentAssetIDChanged: {
                    assetModel.averageFeeForAsset(currentAssetID)
                }
            }

            StackLayout {
                id: stackLayout
                Layout.fillHeight: true
                Layout.fillWidth: true
                currentIndex: 0

                Loader {
                    sourceComponent: pNodeViewModel ? (pNodeViewModel.type === Enums.PaymentNodeType.Lnd ? lndInitialComponent : ethInitialComponent) : undefined
                }
            }

            Component {
                id: lndInitialComponent

                ColumnLayout {
                    id: layout
                    spacing: 25

                    Connections {
                        target: comboBox
                        function onCurrentAssetIDChanged(currentAssetID) {
                            if (channelCapacity.text !== "") {
                                channelCapacity.focus = true
                                amountLocal.text = ""
                                channelCapacity.text = ""
                                channelCapacity.focus = false
                            }
                        }
                    }

                    PubKeyComboBox {
                        id: publicKey
                        Layout.fillWidth: true
                        Layout.preferredHeight: 41
                        currentIndex: -1
                        model: QMLSortFilterListProxyModel {
                            source: pNodeViewModel.hostModel
                            filterString: publicKey.contentSearch.text
                            filterCaseSensitivity: Qt.CaseInsensitive
                        }
                        onCurrentTextChanged: layout.allowPubKey()
                        contentSearch.onTextChanged: layout.allowPubKey()
                        contentSearch.placeholderText: "Enter public key with remote ip (pubkey@ip)"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 41
                        spacing: 8

                        StyledTextInput {
                            id: channelCapacity
                            Layout.fillWidth: true
                            Layout.preferredHeight: 41
                            placeholderText: "Enter channel capacity (min = %1, max = %2)".arg(
                                                 Utils.formatBalance(
                                                     comboBox.currentMinLndCapacity)).arg(
                                                 Utils.formatBalance(
                                                     comboBox.currentMaxLndCapacity))
                            validator: RegExpValidator {
                                regExp: /^(([1-9]{1}([0-9]{1,7})?)(\.[0-9]{0,8})?$|0?(\.[0-9]{0,8}))$/gm
                            }
                            unitValue: comboBox.currentSymbol
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
                                            comboBox.currentAssetID, text)
                                if (!amountLocal.focus) {
                                    amountLocal.text = newAmoutLocal
                                }
                                layout.allowCapacity()
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

                                if (parseFloat(text) <= 0 || isNaN(
                                            parseFloat(text))) {
                                    if (!channelCapacity.focus) {
                                        channelCapacity.text = ""
                                        return
                                    }
                                }

                                var newAmoutCoin = ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                            comboBox.currentAssetID, text)
                                if (!channelCapacity.focus) {
                                    channelCapacity.text = newAmoutCoin
                                }
                                layout.allowCapacity()
                            }
                        }

                        MaxButton {
                            Layout.alignment: Qt.AlignBottom
                            Layout.preferredHeight: 41
                            Layout.preferredWidth: 41
                            onClicked: {
                                channelCapacity.focus = true
                                channelCapacity.text = Utils.formatBalance(
                                            comboBox.currentConfirmedBalanceOnChain)
                                channelCapacity.focus = false
                            }
                        }
                    }

                    NetworkFeeRateView {
                        id: networkFeeRateView
                        Layout.fillWidth: true
                        feeRate: root.networkFeeRate
                        onCurrentOptionFeeChanged: {
                            networkFeeRateCurrentOption = currentOption
                            root.networkFeeRate = calculateNetworkFeeRate(
                                        currentOption,
                                        recommendedNetworkFeeRate)
                        }
                    }

                    ColumnLayout {
                        Layout.leftMargin: 5
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            id: lndNotification
                            property bool isLightningEnabled: ApplicationViewModel.paymentNodeViewModel.paymentNodeActivity(
                                                                  comboBox.currentAssetID)
                            color: SkinColors.transactionItemSent
                            visible: text.length > 0
                            font.pixelSize: 12
                            font.family: fontRegular.name
                            text: !isLightningEnabled ? "%1 L2 is not enabled. If you want to open channel please enable %1 L2.".arg(comboBox.currentSymbol) : !pNodeViewModel.stateModel
                                                        || checkPaymentNodeState(
                                                            pNodeViewModel.stateModel) ? "" : "%1 L2 is syncing. Please wait for it to become active.".arg(comboBox.currentSymbol)
                        }

                        Text {
                            id: capacityMessage
                            color: SkinColors.transactionItemSent
                            visible: text.length > 0
                            font.pixelSize: 12
                            font.family: fontRegular.name
                        }

                        Text {
                            id: pubKeyMessage
                            color: SkinColors.transactionItemSent
                            visible: text.length > 0
                            font.pixelSize: 12
                            font.family: fontRegular.name
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        RowLayout {
                            Layout.leftMargin: 5
                            visible: false

                            XSNLabel {
                                text: "NETWORK FEE  :"
                                font.family: fontRegular.name
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: SkinColors.secondaryText
                            }

                            XSNLabel {
                                text: "Automatic" // "%1  %2" .arg(Utils.formatBalance(networkFee)).arg(comboBox.currentSymbol) : "Automatic"
                                font.family: fontRegular.name
                                font.pixelSize: 14
                                color: SkinColors.mainText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        IntroButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            Layout.maximumWidth: 80
                            Layout.minimumWidth: 80
                            text: "Open"
                            Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                            enabled: capacityMessage.text === ""
                                     && pubKeyMessage.text === ""
                                     && lndNotification.text === ""
                            onClicked: {
                                var isPubKeyEmpty = publicKey.contentSearch.text.length === 0
                                var isCapacityEmpty = channelCapacity.text.length === 0
                                if (isPubKeyEmpty || isCapacityEmpty) {
                                    if (isPubKeyEmpty) {
                                        pubKeyMessage.text = "Enter public key with remote ip!"
                                    }
                                    if (isCapacityEmpty) {
                                        capacityMessage.text = "Enter channel capacity!"
                                    }
                                    return
                                }

                                openChannelViewModel.createOpenChannelRequest(
                                            publicKey.contentSearch.text,
                                            channelCapacity.text,
                                            networkFeeRate)
                            }
                        }
                    }

                    function allowPubKey() {
                        var pubKey = publicKey.contentSearch.text
                        if ((!pubKey.includes(":") || !pubKey.includes("@"))
                                && pubKey !== "") {
                            pubKeyMessage.text = "Wrong public key remote ip format."
                        } else {
                            pubKeyMessage.text = ""
                        }
                    }

                    function allowCapacity() {
                        if (channelCapacity.text === "") {
                            capacityMessage.text = ""
                            return
                        }

                        var capacity = Utils.parseCoinsToSatoshi(
                                    channelCapacity.text)

                        if (capacity < comboBox.currentMinLndCapacity) {
                            capacityMessage.text
                                    = "Channel capacity cannot be less than %1 %2.".arg(
                                        Utils.formatBalance(
                                            comboBox.currentMinLndCapacity)).arg(
                                        comboBox.currentSymbol)
                        } else if (capacity > comboBox.currentMaxLndCapacity) {
                            capacityMessage.text
                                    = "Channel capacity cannot be greater than %1 %2.".arg(
                                        Utils.formatBalance(
                                            comboBox.currentMaxLndCapacity)).arg(
                                        comboBox.currentSymbol)
                        } else if (capacity > comboBox.currentConfirmedBalanceOnChain) {
                            capacityMessage.text = "Channel capacity cannot be greater than confirmed on chain balance."
                        } else {
                            capacityMessage.text = ""
                        }
                    }
                }
            }

            Component {
                id: ethInitialComponent

                ColumnLayout {
                    id: layout
                    spacing: 25

                    PubKeyComboBox {
                        id: publicKey
                        Layout.fillWidth: true
                        Layout.preferredHeight: 41
                        currentIndex: -1
                        model: QMLSortFilterListProxyModel {
                            source: pNodeViewModel.hostModel
                            filterString: publicKey.contentSearch.text
                            filterCaseSensitivity: Qt.CaseInsensitive
                        }
                        onCurrentTextChanged: layout.allowPubKey()
                        contentSearch.onTextChanged: layout.allowPubKey()
                        contentSearch.placeholderText: "Enter public key"
                    }

                    ColumnLayout {
                        Layout.leftMargin: 5
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            id: ethNotification
                            property bool isConnextEnabled: ApplicationViewModel.paymentNodeViewModel.paymentNodeActivity(
                                                                comboBox.currentAssetID)
                            color: SkinColors.transactionItemSent
                            visible: text.length > 0
                            font.pixelSize: 12
                            font.family: fontRegular.name
                            text: !isConnextEnabled ? "%1 Connext is not enabled. If you want to open channel please enable %1 Connext.".arg(comboBox.currentSymbol) : checkPaymentNodeState(pNodeViewModel.stateModel) ? "" : "%1 Connext is syncing. Please wait for it to become active.".arg(comboBox.currentSymbol)
                        }

                        Text {
                            id: pubKeyMessage
                            color: SkinColors.transactionItemSent
                            visible: text.length > 0
                            font.pixelSize: 12
                            font.family: fontRegular.name
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        RowLayout {
                            Layout.leftMargin: 5
                            visible: false

                            XSNLabel {
                                text: "NETWORK FEE  :"
                                font.family: fontRegular.name
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: SkinColors.secondaryText
                            }

                            XSNLabel {
                                text: "Automatic" // "%1  %2" .arg(Utils.formatBalance(networkFee)).arg(comboBox.currentSymbol) : "Automatic"
                                font.family: fontRegular.name
                                font.pixelSize: 14
                                color: SkinColors.mainText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        IntroButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            Layout.maximumWidth: 80
                            Layout.minimumWidth: 80
                            text: "Open"
                            Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                            enabled: pubKeyMessage.text === ""
                                     && ethNotification.text === ""
                            onClicked: {
                                var isPubKeyEmpty = publicKey.contentSearch.text.length === 0
                                if (isPubKeyEmpty) {
                                    if (isPubKeyEmpty) {
                                        pubKeyMessage.text = "Enter public key!"
                                    }
                                    return
                                }

                                openChannelViewModel.createOpenChannelRequest(
                                            publicKey.contentSearch.text,
                                            "0",
                                            networkFeeRate)
                            }
                        }
                    }

                    function allowPubKey() {
                        var pubKey = publicKey.contentSearch.text
                        if ((!pubKey.startsWith('vector')) && pubKey !== "") {
                            pubKeyMessage.text = "Wrong public key format (Public key should start with 'vector')."
                        } else {
                            pubKeyMessage.text = ""
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

                    property real networkFee: undefined
                    property real deployFee: undefined
                    property var pubkey: undefined
                    property string symbol: ""
                    property bool hasDeployFee: parseFloat(layout.deployFee) > 0

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 15
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 7

                        XSNLabel {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredHeight: 20
                            font.family: mediumFont.name
                            font.pixelSize: 18
                            text: "Are you sure you want to open channel"
                        }

                        XSNLabel {
                            Layout.preferredHeight: 20
                            Layout.alignment: Qt.AlignHCenter
                            font.family: mediumFont.name
                            font.pixelSize: 18
                            text: "to"
                        }

                        XSNLabel {
                            Layout.preferredHeight: 40
                            Layout.alignment: Qt.AlignHCenter
                            lineHeight: 1.5
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            Layout.preferredWidth: 530
                            wrapMode: Text.WrapAnywhere
                            font.family: mediumFont.name
                            font.pixelSize: 16
                            text: 'Public key:<font color="%1"> %2 </font> ?'.arg(
                                      SkinColors.sendPopupConfirmText).arg(
                                      layout.pubkey)
                        }

                        Item {
                          Layout.preferredHeight: hasDeployFee ? 20 : 30
                        }

                        XSNLabel {
                            Layout.preferredHeight: 20
                            Layout.alignment: Qt.AlignHCenter
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            font.family: mediumFont.name
                            font.pixelSize: 16
                            text: 'Network fee: <font color="%1"> %2  %3</font> '.arg(
                                      SkinColors.sendPopupConfirmText).arg(
                                      Utils.formatBalance(layout.networkFee)).arg(symbol)
                        }

                        XSNLabel {
                            Layout.preferredHeight: 20
                            Layout.alignment: Qt.AlignHCenter
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            font.family: mediumFont.name
                            font.pixelSize: 16
                            text: 'Deployment channel fee: <font color="%1"> %2  %3</font> '.arg(
                                      SkinColors.sendPopupConfirmText).arg(
                                      Utils.formatBalance(layout.deployFee)).arg(symbol)
                            visible: hasDeployFee
                        }

                        Item {
                          Layout.preferredHeight: 20
                        }

                        XSNLabel {
                            Layout.preferredHeight: 20
                            Layout.alignment: Qt.AlignHCenter
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            font.family: mediumFont.name
                            font.pixelSize: 16
                            visible: hasDeployFee
                            text: 'Total fee: <font color="%1"> %2  %3</font> '.arg(
                                      SkinColors.sendPopupConfirmText).arg(
                                      Utils.formatBalance(layout.networkFee + layout.deployFee)).arg(symbol)
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
                                openChannelViewModel.cancelRequest()
                                stackView.pop()
                            }
                        }

                        IntroButton {
                            Layout.preferredWidth: 110
                            Layout.preferredHeight: 50
                            font.family: mediumFont.name
                            text: qsTr("Confirm")
                            onClicked: {
                                openChannelViewModel.confirmRequest()
                                stackView.push(
                                            "qrc:/Components/PopupProgressBarComponent.qml",
                                            {
                                                "progressBarMsg": "Opening channel ..."
                                            })
                            }
                        }
                    }
                }
            }
        }
    }
}
