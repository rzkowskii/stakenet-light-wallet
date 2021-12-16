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

    width: 420
    height: isAdvanced || rentChannelStackLayout.currentIndex === 2 ? 630 : 380

    property int assetID: 0
    property string symbol: ""
    property int bitcoinId: 0

    property double minLndCapacity: ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                        assetID, minLocalLndCapacity)
    property string minLocalLndCapacity: assetID === bitcoinId ? "15.00" : "5.00"
    property string depositLocalLndBalance: ApplicationViewModel.localCurrencyViewModel.convert(
                                                comboBox.currentAssetID,
                                                Utils.formatBalance(
                                                    comboBox.currentActiveLndBalance))
    property string minimumRentBalance: ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                            comboBox.currentAssetID,
                                            minLocalLndCapacity)
    property bool isLightningEnabled: ApplicationViewModel.paymentNodeViewModel.paymentNodeActivity(
                                          comboBox.currentAssetID)
    property bool isPaymentLndSynced: checkPaymentNodeState(
                                          paymentNodeViewModel.stateModel)

    property double minimumAllowedLocalCapacity: minLocalLndCapacity === "< 0.01" ? 0.01 : parseFloat(minLocalLndCapacity) + 0.01 //rounding convert fault $1.532 = $1.53, but actually min is more than $1.53
    property bool сanNotRentChannel: depositLocalLndBalance === "< 0.01"
                                     || parseFloat(depositLocalLndBalance) === 0
                                     || parseFloat(
                                         depositLocalLndBalance) < minimumAllowedLocalCapacity
                                     || capacitySlider.from > capacitySlider.to

    property bool isAdvanced: false

    property var feeInfo: channelRentingModel ? channelRentingModel.rentingFee : undefined

    property string totalFee: channelRentingModel ? Utils.formatBalance(
                                                        feeInfo.fee) : ""

    property string rentingFee: channelRentingModel ? Utils.formatBalance(
                                                          feeInfo.rentingFee) : ""

    property string onChainFee: channelRentingModel ? Utils.formatBalance(
                                                          feeInfo.onChainFee) : ""

    property string localRentalFee: ApplicationViewModel.localCurrencyViewModel.convert(
                                        comboBox.currentAssetID, rentingFee)

    property string localOnChainFee: ApplicationViewModel.localCurrencyViewModel.convert(
                                         comboBox.currentAssetID, onChainFee)

    property string localTotalFee: ApplicationViewModel.localCurrencyViewModel.convert(
                                       comboBox.currentAssetID, totalFee)

    property bool enableRenting: !сanNotRentChannel && isLightningEnabled
                                 && isPaymentLndSynced
    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    ChannelRentingModel {
        id: channelRentingModel
        assetID: root.assetID

        Component.onCompleted: {
            initialize(ApplicationViewModel)
            initialComponent.updateFee()
            initComboBox()
        }

        onRentingSuccess: {
            rentComponentResult.operationMsg = "Successfully rented!"
            rentComponentResult.resultMsg = "Success!"
            rentComponentResult.imgPath = "qrc:/images/SUCCESS_ICON.png"
            rentComponentResult.confirmBtnAction = (rentChannelStackLayout.currentIndex = 0)

            rentChannelStackLayout.currentIndex = 2
        }

        onRentingFailure: {
            rentComponentResult.operationMsg = errorString
            rentComponentResult.resultMsg = "Failed!"
            rentComponentResult.imgPath = "qrc:/images/crossIcon.png"
            rentComponentResult.confirmBtnAction = (rentChannelStackLayout.currentIndex = 0)

            rentChannelStackLayout.currentIndex = 2
        }
    }

    PaymentNodeViewModel {
        id: paymentNodeViewModel
        currentAssetID: comboBox.currentAssetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    StackLayout {
        id: rentChannelStackLayout
        anchors.fill: parent
        currentIndex: 0
        anchors.margins: 10

        ColumnLayout {
            id: initialComponent
            Layout.fillHeight: true
            Layout.fillWidth: true

            Layout.leftMargin: 5
            Layout.rightMargin: 5
            spacing: 10

            RowLayout {
                Layout.preferredHeight: 25
                Layout.fillWidth: true

                Text {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                    font.family: regularFont.name
                    font.pixelSize: 16
                    text: "Channel rental %1".arg(symbol)
                    font.capitalization: Font.Capitalize
                    color: SkinColors.mainText
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                CloseButton {
                    Layout.preferredHeight: 25
                    Layout.preferredWidth: 25
                    Layout.alignment: Qt.AlignTop
                    onClicked: root.close()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                color: SkinColors.mainBackground

                Text {
                    lineHeight: 1.2
                    anchors.centerIn: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 11
                    color: SkinColors.secondaryText
                    textFormat: Text.RichText
                    font.family: regularFont.name
                    text: сanNotRentChannel ? '<font color="%1">%2</font> balance is not enough to rent this channel.<br/><font color="%3"> Minimum amount to rent is %4 %5 %6</font>'.arg(SkinColors.mainText).arg(comboBox.currentSymbol).arg(SkinColors.transactionItemSent).arg(minLndCapacity).arg(root.symbol).arg(assetID === comboBox.currentAssetID ? "" : "(%1 %2)".arg(minimumRentBalance !== "0.00000000" ? minimumRentBalance : "0.00000001").arg(comboBox.currentSymbol)) : (isLightningEnabled ? (isPaymentLndSynced ? 'To trade this coin you will need to rent a channel.<br/>Based on your balances we recommend a rental of <br/><font color="%1">%3 %2</font>'.arg(SkinColors.mainText).arg(symbol).arg(ApplicationViewModel.localCurrencyViewModel.convertToCoins(assetID, capacitySlider.to)) : '<font color="%1">%2</font> lightning is syncing. Please wait for it to become active.'.arg(SkinColors.mainText).arg(comboBox.currentSymbol)) : '<font color="%1">%2</font> lightning is not enabled. If you want to rent this channel<br/>please enable <font color="%1">%2</font> L2.'.arg(SkinColors.mainText).arg(comboBox.currentSymbol))
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5

                Text {
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 11
                    color: SkinColors.secondaryText
                    text: "Payment in"
                }

                CoinsCombobox {
                    id: comboBox
                    showOnlyLndBalance: true
                    model: QMLSortFilterListProxyModel {
                        id: sortedAssetModel
                        source: QMLSortFilterListProxyModel {
                            source: WalletAssetsListModel {
                                id: assetModel
                                Component.onCompleted: {
                                    initialize(ApplicationViewModel)
                                }
                            }
                            filterRole: "isActive"
                            filterString: "1"
                            filterCaseSensitivity: Qt.CaseInsensitive
                        }

                        filterRole: "symbol"
                        filterCaseSensitivity: Qt.CaseInsensitive
                    }
                    onCurrentAssetIDChanged: initialComponent.updateFee()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                spacing: 10

                Item {
                    Layout.leftMargin: 10
                    Layout.alignment: Qt.AlignLeft
                    Layout.preferredWidth: 85
                    Layout.preferredHeight: 20

                    Row {
                        anchors.fill: parent
                        spacing: 10

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Advanced"
                            color: enableRenting ? SkinColors.mainText : SkinColors.secondaryText
                            font.family: regularFont.name
                            font.pixelSize: 12
                        }

                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            source: enableRenting ? (isAdvanced ? "qrc:/images/dropdown_selected.png" : "qrc:/images/dropdown_default.png") : "qrc:/images/dropdown_default_grey.svg"
                        }
                    }

                    PointingCursorMouseArea {
                        anchors.fill: parent
                        enabled: enableRenting
                        onClicked: isAdvanced = !isAdvanced
                        cursorShape: enableRenting ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: SkinColors.popupFieldBorder
                }
            }

            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 10
                visible: isAdvanced

                SliderBoxView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    helpIconText: "The rented channel enables you to receive the rented capacity of this coin on lightning"
                    title: "Capacity"

                    ColumnLayout {
                        anchors.fill: parent

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            CustomizedSlider {
                                id: capacitySlider
                                anchors.centerIn: parent
                                from: minimumAllowedLocalCapacity
                                to: depositLocalLndBalance > 500.0 ? // max rental is 500$
                                        500.0 : parseFloat(depositLocalLndBalance) - 0.01

                                value: to
                                startText: "Low"
                                finishText: "High"
                                valueSymbol: ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol
                                valueSymbolBefore: true
                                stepSize: 0.01
                                onValueChanged: {
                                    initialComponent.updateFee()
                                }
                            }
                        }

                        XSNLabel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 20
                            Layout.leftMargin: 30
                            Layout.alignment: Qt.AlignLeft
                            font.family: regularFont.name
                            font.pixelSize: 12
                            color: SkinColors.mainText
                            text: "= %1 %2".arg(
                                      ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                          assetID,
                                          capacitySlider.value)).arg(symbol)
                        }
                    }
                }

                SliderBoxView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    helpIconText: "This rented channel will be closed automatically once the duration time has run out"
                    title: "Duration"

                    CustomizedSlider {
                        id: durationSlider
                        anchors.centerIn: parent
                        from: 1
                        to: 24 * 7 // 7 days
                        value: 24 * 7
                        startText: "Min"
                        finishText: "Max"
                        valueSymbol: durationSlider.value > 1 ? "hours" : "hour"
                        secondValueSymbol: Math.floor(durationSlider.value / 24)
                                           > 1 ? "days" : (Math.floor(
                                                               durationSlider.value / 24)
                                                           === 1 ? "day" : "")
                        secondStep: 24
                        stepSize: 1
                        onValueChanged: {
                            initialComponent.updateFee()
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width
                        height: 1
                        color: SkinColors.popupFieldBorder
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                visible: enableRenting && feeInfo.fee > 0

                RowLayout {
                    spacing: 20

                    ColumnLayout {
                        Layout.preferredWidth: 140
                        spacing: 3

                        Text {
                            color: SkinColors.secondaryText
                            font.family: regularFont.name
                            font.pixelSize: 11
                            text: "%1 rental capacity".arg(symbol)
                        }

                        Repeater {
                            model: ["Period", "On chain fees (open + close)", "Rental fee", "Total"]
                            delegate: Text {
                                color: SkinColors.secondaryText
                                font.family: regularFont.name
                                font.pixelSize: 11
                                text: modelData
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 3

                        Repeater {
                            model: 5
                            delegate: Text {
                                color: SkinColors.secondaryText
                                font.family: regularFont.name
                                font.pixelSize: 11
                                text: ":"
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 3

                        Text {
                            color: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 11
                            text: "%1 %2".arg(
                                      ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                          assetID,
                                          capacitySlider.value)).arg(symbol)
                        }

                        Text {
                            color: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 11
                            text: Utils.formatDuration(durationSlider.value)
                        }

                        Text {
                            color: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 11
                            text: "%1 %2 (%3 %4)".arg(onChainFee).arg(
                                      comboBox.currentSymbol).arg(
                                      "%1 %2".arg("≈").arg(
                                          localOnChainFee.replace("<",
                                                                  ""))).arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                        }

                        Text {
                            color: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 11
                            text: "%1 %2 (%3 %4)".arg(rentingFee).arg(
                                      comboBox.currentSymbol).arg(
                                      "%1 %2".arg("≈").arg(
                                          localRentalFee.replace("<", ""))).arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                        }

                        Text {
                            color: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 11
                            text: "%1 %2 (%3 %4)".arg(totalFee).arg(
                                      comboBox.currentSymbol).arg(
                                      "%1 %2".arg("≈").arg(
                                          localTotalFee.replace("<", ""))).arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                        }
                    }
                }
            }

            Item {
                Layout.preferredHeight: 90
                Layout.fillWidth: true
                visible: feeInfo.fee === 0 && isAdvanced
            }

            RowLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    Layout.fillHeight: true
                    spacing: 5

                    SecondaryButton {
                        Layout.preferredHeight: 30
                        Layout.preferredWidth: 70
                        font.pixelSize: 12
                        borderColor: SkinColors.popupFieldBorder
                        hoveredBorderColor: SkinColors.headerText
                        activeStateColor: SkinColors.popupFieldBorder
                        font.capitalization: Font.MixedCase
                        text: "Cancel"
                        onClicked: root.close()
                    }

                    IntroButton {
                        Layout.preferredHeight: 30
                        Layout.preferredWidth: 70
                        font.pixelSize: 12
                        text: "Confirm"
                        buttonGradientColor: enabled ? SkinColors.introBtnGradientColor : SkinColors.popupFieldBorder
                        buttonColor: enabled ? SkinColors.buttonBorderColor : SkinColors.popupFieldBorder
                        enabled: enableRenting && feeInfo.fee > 0
                        onClicked: {
                            var capacityValue = Utils.parseCoinsToSatoshi(
                                        ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                            assetID, capacitySlider.value))
                            channelRentingModel.rent(comboBox.currentAssetID,
                                                     capacityValue,
                                                     durationSlider.value)
                            rentChannelStackLayout.currentIndex = 1
                        }
                    }
                }
            }

            function updateFee() {
                var capacityValue = Utils.parseCoinsToSatoshi(
                            ApplicationViewModel.localCurrencyViewModel.convertToCoins(
                                assetID, capacitySlider.value))
                channelRentingModel.updateRentingFee(comboBox.currentAssetID,
                                                     capacityValue,
                                                     durationSlider.value)
            }
        }

        ColumnLayout {
            id: progressComponent
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 30

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            XSNLabel {
                text: "Opening channel ..."
                font.family: mediumFont.name
                font.pixelSize: 20
            }

            ProgressBar {
                id: progressBar
                indeterminate: true
                Layout.preferredHeight: 10
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignCenter

                background: Rectangle {
                    anchors.fill: parent
                    color: SkinColors.sendPopupConfirmText
                    radius: 2
                    opacity: 0.7
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            id: rentComponentResult
            property string operationMsg: ""
            property string resultMsg: ""
            property string imgPath: ""
            property var confirmBtnAction

            FontLoader {
                id: mediumFont
                source: "qrc:/Rubik-Medium.ttf"
            }

            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 30

            Item {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight: 110
                Layout.topMargin: 40
                Layout.fillWidth: true

                ColorOverlayImage {
                    anchors.centerIn: parent
                    imageSize: 110
                    color: SkinColors.mainText
                    imageSource: rentComponentResult.imgPath
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                XSNLabel {
                    anchors.centerIn: parent
                    text: rentComponentResult.resultMsg
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                XSNLabel {
                    anchors.centerIn: parent
                    font.pixelSize: 20
                    text: rentComponentResult.operationMsg
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            IntroButton {
                id: confirmBtn
                Layout.topMargin: 100
                Layout.preferredWidth: 200
                Layout.preferredHeight: 45
                text: qsTr("OK")
                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                onClicked: root.close()
            }
        }
    }

    function initComboBox() {
        sortedAssetModel.filterString = parseAssetsModel(
                    channelRentingModel.payingAssets)

        if (assetID !== bitcoinId) {
            for (var i = 0; i < sortedAssetModel.count; i++) {

                if (sortedAssetModel.get(i).id === bitcoinId) {
                    comboBox.currentIndex = i
                    return
                }
            }
        } else {
            for (var j = 0; j < sortedAssetModel.count; j++) {
                if (ApplicationViewModel.paymentNodeViewModel.paymentNodeActivity(
                            sortedAssetModel.get(j).id)) {
                    comboBox.currentIndex = j
                    return
                }
            }
            comboBox.currentIndex = 0
        }
    }
}
