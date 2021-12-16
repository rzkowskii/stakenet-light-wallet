import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Page {
    id: root

    property int assetID: 0
    property int currentIndex: -1
    property string currentSymbol: assetModel.get(currentIndex).symbol
    property var currentBalance: assetModel.get(currentIndex).balance
    property string currentBalanceStr: Utils.formatBalance(currentBalance)
    property string currentName: assetModel.get(currentIndex).name
    property var networkFee: sendTransactionViewModel.networkFee
    property string currentCurrencySymbol: ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol
    property string currentColor: assetModel.get(currentIndex).color

    property double recommendedNetworkFeeRate: 0.0

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    FontLoader { id: lightFont; source: "qrc:/Rubik-Light.ttf" }
    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }

    background: Rectangle {
        color: "transparent"
    }

    WalletAssetsListModel {
        id: assetModel

        onAverageFeeForAssetFinished: {
            recommendedNetworkFeeRate = value;
        }

        Component.onCompleted: {
            initialize(ApplicationViewModel);
            root.currentIndex = getInitial(assetID)
            averageFeeForAsset(assetID)
        }
    }

    SendTransactionViewModel {
        id: sendTransactionViewModel

        onTransactionSendingFailed: {
            stackView.push(errorComponent, {errorMessage : error});
        }

        Component.onCompleted: {
            initialize(ApplicationViewModel);
            requestAddressDetails(assetID)
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.topMargin: 25
        anchors.bottomMargin: 30
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        clip: true
        initialItem: initialComponent

        Component {
            id: initialComponent

            ColumnLayout {
                id: layout
                property bool enableCoins: true
                // property var remainingBalance: calcRemainingBalance(amountCoin.text, includeFee)
                property var moneyToSend: undefined
                property bool txCreated: false
                property bool includeFee: false

                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 30
                Layout.bottomMargin: 40

                Connections {
                    target: sendTransactionViewModel
                    function onTransactionCreatingFailed(errorMessage) {
                        txCreated = false
                        notification.text = errorMessage
                        notification.color = SkinColors.transactionItemSent
                        sendButton.enabled = false
                    }

                    function onTransactionCreated(moneyToSend, networkFee) {
                        txCreated = true
                        notification.text =  ""
                        root.networkFee = networkFee
                        layout.moneyToSend = moneyToSend
                        sendButton.enabled = true
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignCenter
                    spacing: 18

                    MobileTitle {
                        Layout.alignment: Qt.AlignCenter
                        text: "send %1" .arg(currentName)
                    }

                    Image {
                        Layout.alignment: Qt.AlignCenter
                        source: currentName !== "" ? "qrc:/images/ICON_%1.svg".arg(currentName): ""
                        sourceSize: Qt.size(43, 49)
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        spacing: 5

                        SecondaryLabel {
                            Layout.alignment: Qt.AlignHCenter
                            font.family: regularFont.name
                            font.pixelSize: 12
                            text: "Available:"
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.fillWidth: true
                            spacing: 5

                            XSNLabel {
                                font.family: regularFont.name
                                text: currentBalanceStr
                                font.pixelSize: 20
                                color: currentColor
                                font.weight: Font.Thin
                            }

                            XSNLabel {
                                font.family: regularFont.name
                                text: currentSymbol
                                font.pixelSize: 20
                                font.capitalization: Font.AllUppercase
                                font.weight: Font.Thin
                                color: currentColor
                            }
                        }

                        SecondaryLabel {
                            Layout.alignment: Qt.AlignHCenter
                            font.family: regularFont.name
                            font.pixelSize: 14
                            text: "= %1 %2" .arg(currentCurrencySymbol).arg(ApplicationViewModel.localCurrencyViewModel.convert(assetID, currentBalanceStr))
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 49
                    radius: 8
                    color: SkinColors.mobileSecondaryBackground

                    RowLayout {
                        anchors.fill: parent
                        spacing: 3

                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            Layout.leftMargin: 5

                            ValidatedTextField {
                                id: addressTo
                                anchors.fill: parent
                                placeholderText: "Address"
                                placeholderTextColor: SkinColors.mainText
                                font.family: regularFont.name
                                font.pixelSize: 14

                                selectByMouse: true

                                background: Rectangle {
                                    color: "transparent"
                                }
                                customValidator: function(input) {
                                    return sendTransactionViewModel.validateAddress(input)
                                }
                                color: SkinColors.mainText
                            }
                        }

                        MobileButton {
                            Layout.rightMargin: 4
                            image.imageSource: "qrc:/images/QR_ICON.png"
                            backgroundButton.border.color: SkinColors.menuBackgroundGradientFirst
                            backgroundButton.color: SkinColors.mobileButtonBackground
                            backgroundButton.radius: 6
                            image.imageSize: 16
                            width: 41
                            height: width
                            image.color: "white"
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80

                    RowLayout {
                        anchors.fill: parent
                        spacing: 15

                        ColumnLayout {
                            spacing: 5

                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                Layout.leftMargin: 5

                                Row {
                                    anchors.fill: parent
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 2

                                    TextField {
                                        id: amountCoin
                                        height: parent.height
                                        placeholderText: "%1 0.00".arg(currentCurrencySymbol)
                                        placeholderTextColor: SkinColors.mainText
                                        font.family: lightFont.name
                                        font.pixelSize: 20
                                        font.weight: Font.Thin
                                        background: Rectangle {
                                            color: "transparent"
                                        }
                                        validator: RegExpValidator{regExp: /^(([1-9]{1,})+(\.?[0-9]{0,8})$|0+(\.[0-9]{0,8}))$/gm }
                                        focus: true
                                        selectByMouse: true
                                        color: SkinColors.mainText
                                        onTextChanged: {

                                            if(!isNaN(parseFloat(text)))
                                            {
                                                if(enableCoins)
                                                {
                                                    amountLocal.text = ApplicationViewModel.localCurrencyViewModel.convert(assetID, text);
                                                }
                                                else
                                                {
                                                    if(choosingListView.currentOption === "RESET")
                                                    {
                                                        amountLocal.text = ApplicationViewModel.localCurrencyViewModel.convertToCoins(assetID, text);
                                                    }
                                                }
                                            }
                                            else if(text == "")
                                            {
                                                amountLocal.text = ""
                                            }

                                            if(addressTo.text !== "" && text !== "")
                                            {
                                                if(enableCoins)
                                                {
                                                    includeFee = text == currentBalanceStr;
                                                    createUTXOTransaction(amountCoin.text, addressTo.text, includeFee, recommendedNetworkFeeRate)
                                                }
                                                else
                                                {
                                                    includeFee = amountLocal.text == currentBalanceStr;
                                                    createUTXOTransaction(amountLocal.text, addressTo.text, includeFee, recommendedNetworkFeeRate)
                                                }
                                            }
                                        }
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        font.family: lightFont.name
                                        font.pixelSize: 20
                                        font.weight: Font.Thin
                                        text: "%1" .arg(enableCoins ? currentSymbol : currentCurrencySymbol)
                                        color: SkinColors.mainText
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 2
                                color: SkinColors.mobileSecondaryBackground
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                Layout.leftMargin: 15

                                Row {
                                    anchors.fill: parent
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 0

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        font.family: regularFont.name
                                        font.pixelSize: 14
                                        text: "= %1" .arg(enableCoins ? currentCurrencySymbol : "")
                                        color: SkinColors.menuItemText
                                    }

                                    TextField {
                                        id: amountLocal
                                        height: parent.height
                                        placeholderText: "0.00"
                                        placeholderTextColor: SkinColors.menuItemText
                                        font.family: regularFont.name
                                        font.pixelSize: 14
                                        background: Rectangle {
                                            color: "transparent"
                                        }
                                        focus: false
                                        readOnly: true
                                        selectByMouse: true
                                        color: SkinColors.menuItemText
                                    }

                                    Text {
                                        visible: !enableCoins
                                        anchors.verticalCenter: parent.verticalCenter
                                        font.family: regularFont.name
                                        font.pixelSize: 14
                                        text: " %1" .arg(currentSymbol)
                                        color: SkinColors.menuItemText
                                    }
                                }
                            }
                        }

                        Item {
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: 42

                            Image {
                                anchors.verticalCenter: parent.verticalCenter
                                source: "qrc:/images/ic_switch.png"
                                sourceSize: Qt.size(42, 42)
                                width: 42
                                height: 42

                                PointingCursorMouseArea {
                                    onClicked: {
                                        enableCoins = !enableCoins
                                        var swapText =   amountCoin.text
                                        amountCoin.text = amountLocal.text
                                        amountLocal.text = swapText
                                    }
                                }
                            }
                        }
                    }
                }

                ChoosingListView {
                    id: choosingListView
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    model: ["MIN", "MAX", "RESET"]
                    currentIndex: 2
                    onCurrentOptionChanged: {
                        if(currentOption === "MIN")
                        {
                            var amount = enableCoins ? "0.0001" : ApplicationViewModel.localCurrencyViewModel.convert(assetID, "0.00010000")
                            amountLocal.text = enableCoins ? amount : "0.0001"
                            amountCoin.text = amount
                            amountCoin.readOnly = true
                        }
                        else if(currentOption === "MAX")
                        {
                            if(enableCoins)
                            {
                                amountLocal.text =  amountLocal.text = ApplicationViewModel.localCurrencyViewModel.convert(assetID, currentBalanceStr)
                                amountCoin.text = currentBalanceStr;
                            }
                            else
                            {
                                amountLocal.text = currentBalanceStr;
                                amountCoin.text = ApplicationViewModel.localCurrencyViewModel.convert(assetID, currentBalanceStr)
                            }

                            amountCoin.readOnly = true
                        }
                        else {
                            amountCoin.readOnly = false
                            amountCoin.text = "0.0"
                        }
                    }
                }


                ColumnLayout {
                    spacing: 10

                    MobileActionButton {
                        id: sendButton
                        enabled: layout.moneyToSend !== undefined
                        Layout.preferredHeight: 41
                        Layout.fillWidth: true
                        buttonColor: currentColor
                        buttonText: "SEND"
                        onClicked: {
                            if(addressTo.text === "")
                            {
                                addressTo.placeholderText = "Enter address";
                                addressTo.placeholderTextColor = SkinColors.transactionItemSent
                            }
                            else if(amountCoin.text === "")
                            {
                                amountCoin.placeholderText = "Enter amount";
                                amountCoin.placeholderTextColor = SkinColors.transactionItemSent
                            }
                            else
                            {
                                stackView.push(confirmComponent, {amount: layout.moneyToSend, fee: root.networkFee, address : addressTo.text});
                            }
                        }
                    }

                    XSNLabel {
                        id: notification
                        font.family: regularFont.name
                        font.pixelSize: 14
                    }
                }

                MobileFooter {
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    leftButton.text: "back"
                    rightButton.visible: false
                    onLeftButtonClicked: {
                        navigateBack()
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
                Layout.topMargin: 30
                Layout.alignment: Qt.AlignCenter
                spacing: 20

                property var amount: undefined
                property var fee: undefined
                property var address: undefined


                Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: "qrc:/images/AW_SEND_CONFIRMED.png"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 15

                    XSNLabel {
                        Layout.alignment: Qt.AlignHCenter
                        color: SkinColors.mainText
                        text: "You`re about to send"
                        font.pixelSize: 20
                    }

                    XSNLabel {
                        Layout.alignment: Qt.AlignHCenter
                        color: currentColor
                        text: "%1 %2" .arg(Utils.formatBalance(layout.amount)) .arg(currentSymbol)
                        font.pixelSize: 20
                        font.weight: Font.Thin
                    }

                    XSNLabel {
                        Layout.alignment: Qt.AlignHCenter
                        color: SkinColors.mainText
                        text: "to this address"
                        font.pixelSize: 20
                    }

                    XSNLabel {
                        Layout.alignment: Qt.AlignHCenter
                        color: currentColor
                        text: address
                        font.pixelSize: 13
                    }
                }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    Layout.preferredHeight: 122
                    radius: 8
                    color: SkinColors.mobileSecondaryBackground

                    SecondaryLabel {
                        lineHeight: 2
                        anchors.topMargin: 20
                        anchors.bottomMargin: 20
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        anchors.fill: parent
                        wrapMode: Text.WordWrap
                        anchors.centerIn: parent
                        font.family: regularFont.name
                        font.pixelSize: 12
                        text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer placerat ornare consectetur. Etiam posuere volutpat sapien, eu porta augue dapibus sit amet."
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.alignment: Qt.AlignTop

                    Item {
                        Layout.preferredWidth: 70
                        Layout.preferredHeight: 20

                        PointingCursorMouseArea {
                            onClicked: {
                                sendTransactionViewModel.cancelSending();
                                stackView.pop();
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 5

                            Item {
                                width: 16
                                height: 20

                                ColorOverlayImage {
                                    anchors.centerIn: parent
                                    anchors.fill: parent
                                    imageSize: parent.width
                                    imageSource: "qrc:/images/backImage.png"
                                    color: SkinColors.mainText
                                }
                            }

                            Text {
                                Layout.alignment: Qt.AlignVCenter & Qt.AlignTop
                                text: "cancel"
                                font.capitalization: Font.AllUppercase
                                font.pixelSize: 16
                                font.family: regularFont.name
                                color: SkinColors.mainText
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Item {
                        Layout.preferredWidth: 70
                        Layout.preferredHeight: 20

                        PointingCursorMouseArea {
                            onClicked: {
                                stackView.push(progressComponent, {progressBarMsg: "Sending transaction ..."})
                                sendTransactionViewModel.confirmSending();
                            }
                        }

                        Text {
                            anchors.fill: parent
                            text: "confirm"
                            font.capitalization: Font.AllUppercase
                            font.pixelSize: 16
                            font.family: regularFont.name
                            color: SkinColors.mainText
                        }
                    }
                }
            }
        }

        Component {
            id: progressComponent

            PopupProgressBarComponent {

                Connections {
                    target: sendTransactionViewModel
                    function onTransactionSendingFailed(error) {
                        stackView.push(errorComponent, {errorMessage: error})
                    }

                    function onTransactionSendingFinished(transactionID) {
                        stackView.push(successfulSentComponent, {transactionID: transactionID})
                    }
                }

            }
        }

        Component {
            id: successfulSentComponent

            ColumnLayout {
                property string transactionID: ""
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignCenter
                spacing: 50

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 50

                    XSNLabel {
                        Layout.alignment: Qt.AlignHCenter
                        font.family: mediumFont.name
                        font.pixelSize: 22
                        text: "Successful!"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 20

                        XSNLabel {
                            font.family: mediumFont.name
                            font.pixelSize: 16
                            text: "Transaction Id : "
                        }

                        XSNLabel {
                            Layout.preferredHeight: 25
                            Layout.preferredWidth: 180
                            font.family: mediumFont.name
                            font.pixelSize: 14
                            wrapMode: Text.WrapAnywhere
                            text: transactionID
                        }
                    }
                }

                MobileActionButton {
                    Layout.leftMargin: 30
                    Layout.rightMargin: 30
                    Layout.fillWidth: true
                    Layout.preferredHeight: 45
                    buttonText: qsTr("OK")
                    Layout.alignment: Qt.AlignHCenter
                    buttonColor: SkinColors.menuBackgroundGradientFirst
                    onClicked: navigateBack()
                }
            }
        }

        Component {
            id: errorComponent

            ColumnLayout {
                property string errorMessage: ""
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 30

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10

                    XSNLabel {
                        Layout.alignment: Qt.AlignHCenter
                        font.family: mediumFont.name
                        font.pixelSize: 22
                        text: "Failed!"
                    }

                    Item {
                        Layout.preferredHeight: 25
                        Layout.preferredWidth: 150
                        Layout.alignment: Qt.AlignHCenter

                        XSNLabel {
                            anchors.centerIn: parent
                            font.family: mediumFont.name
                            font.pixelSize: 16
                            text: errorMessage
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                IntroButton {
                    Layout.preferredWidth: 210
                    Layout.preferredHeight: 45
                    text: qsTr("OK")
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                    onClicked: root.close();
                }
            }
        }
    }
    function createUTXOTransaction(amountCoins, address, isFeeIncluding, feeRate) {
        sendTransactionViewModel.createSendTransaction({"type" : "utxo",
                                                        "userSelectedFeeRate" : feeRate,
                                                        "substractFeeFromAmount" : isFeeIncluding,
                                                        "addressTo" : address,
                                                        "amount" : amountCoins})
    }
}
