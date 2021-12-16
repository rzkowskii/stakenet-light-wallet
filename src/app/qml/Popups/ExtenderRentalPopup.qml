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
    height: 440

    property int assetID: 0
    property string symbol: ""
    property double capacity: 0.0
    property string fundingTxOutpoint: ""
    property int bitcoinId: 0
    property double rentingFee: channelRentingModel ? channelRentingModel.extendedRentingFee : 0
    property string rentingFeeStr: channelRentingModel ? Utils.formatBalance(
                                                             rentingFee) : ""
    property bool canExtended: rentingFee <= comboBox.currentActiveLndBalance
    property string localRentalFee: rentingFeeStr
                                    !== "" ? ApplicationViewModel.localCurrencyViewModel.convert(
                                                 comboBox.currentAssetID,
                                                 rentingFeeStr) : ""
    property bool isLightningEnabled: ApplicationViewModel.paymentNodeViewModel.paymentNodeActivity(
                                          comboBox.currentAssetID)
    property bool isPaymentLndSynced: checkPaymentNodeState(
                                          paymentNodeViewModel.stateModel)
    property bool enableExtending: canExtended && isLightningEnabled
                                   && isPaymentLndSynced

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    PaymentNodeViewModel {
        id: paymentNodeViewModel
        currentAssetID: comboBox.currentAssetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    ChannelRentingModel {
        id: channelRentingModel
        assetID: root.assetID

        Component.onCompleted: {
            initialize(ApplicationViewModel)
            initialItem.updateFee()
        }

        onExtendingFailure: function (error) {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": error,
                               "resultMsg": "Failed!",
                               "imgPath": "qrc:/images/crossIcon.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }

        onExtendingSuccess: function (hours) {
            stackView.push("qrc:/Components/OperationResultComponent.qml", {
                               "operationMsg": "Channel time extended by %1".arg(
                                                   Utils.formatDuration(hours)),
                               "resultMsg": "Success!",
                               "imgPath": "qrc:/images/SUCCESS_ICON.png",
                               "confirmBtnAction": function () {
                                   root.close()
                               }
                           })
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.margins: 10
        clip: true
        initialItem: Item {
            id: initialItem

            ColumnLayout {
                anchors.fill: parent
                spacing: 15

                RowLayout {
                    Layout.preferredHeight: 25
                    Layout.fillWidth: true

                    Text {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        verticalAlignment: Text.AlignVCenter
                        font.family: regularFont.name
                        font.pixelSize: 16
                        text: "Rental Extension %1".arg(symbol)
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

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    SecondaryLabel {
                        Layout.fillWidth: true
                        text: qsTr("Can receive:")
                        font.pixelSize: 12
                    }

                    Row {
                        Layout.fillWidth: true
                        spacing: 0

                        SelectedText {
                            text: "%1 %2".arg(Utils.formatBalance(
                                                  capacity)).arg(symbol)
                            font.pixelSize: 12
                            font.family: regularFont.name
                            color: SkinColors.mainText
                        }

                        SelectedText {
                            text: " ( = %1 %2 )".arg(
                                      ApplicationViewModel.localCurrencyViewModel.convert(
                                          assetID,
                                          Utils.formatBalance(capacity))).arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                            font.pixelSize: 12
                            color: SkinColors.secondaryText
                            font.family: regularFont.name
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    color: SkinColors.mainBackground

                    Text {
                        lineHeight: 1.1
                        anchors.centerIn: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 11
                        color: SkinColors.secondaryText
                        textFormat: Text.RichText
                        font.family: regularFont.name
                        text: {
                            if (!isLightningEnabled) {
                                return '<font color="%1">%2</font> lightning is not enabled. If you want to rent this channel<br/>please enable <font color="%1">%2</font> L2.'.arg(
                                            SkinColors.mainText).arg(
                                            comboBox.currentSymbol)
                            } else if (!canExtended) {
                                return '<font color="%1">%2</font> balance is not enough to extend this channel.<br/><font color="%3"> Minimum amount to rent is %4 %5</font>'.arg(
                                            SkinColors.mainText).arg(
                                            comboBox.currentSymbol).arg(
                                            SkinColors.transactionItemSent).arg(
                                            minimumRentBalance !== "0.00000000" ? minimumRentBalance : "0.00000001").arg(
                                            comboBox.currentSymbol)
                            } else if (!isPaymentLndSynced) {
                                return '<font color="%1">%2</font> lightning is syncing. Please wait for it to become active.'.arg(
                                            SkinColors.mainText).arg(
                                            comboBox.currentSymbol)
                            } else {
                                return 'You are about to extend a <font color="%1">%2</font> channel for a period of <font color="%1">%3</font>,<br/> at the end of which the channel will be closed if not extended.<br/>
The rental will cost <font color="%1">≈ %4 %5</font> (%6 %7)<br/>paid via Lightning Network.'.arg
(SkinColors.mainText).arg(symbol).arg(Utils.formatDuration
                                      (durationSlider.value)).arg
(rentingFeeStr).arg(comboBox.currentSymbol).arg
("%1 %2".arg("≈").arg(localRentalFee.replace("<", ""))).arg
(ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                            }
                        }
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

                                    onCountChanged: {
                                        if (assetID !== bitcoinId) {
                                            for (var i = 0; i < sortedAssetModel.count; i++) {
                                                if (sortedAssetModel.get(
                                                            i).id === bitcoinId) {
                                                    comboBox.currentIndex = i
                                                    return
                                                }
                                            }
                                        } else {
                                            comboBox.currentIndex = 0
                                        }
                                    }
                                }

                                filterRole: "isActive"
                                filterString: "1"
                                filterCaseSensitivity: Qt.CaseInsensitive
                            }

                            filterRole: "id"
                            filterString: assetID !== bitcoinId ? "0" : "[^0]+"
                            filterCaseSensitivity: Qt.CaseInsensitive
                        }
                        onCurrentAssetIDChanged: initialItem.updateFee()
                    }
                }

                SliderBoxView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    title: "Added duration"

                    CustomizedSlider {
                        id: durationSlider
                        anchors.centerIn: parent
                        from: 1
                        to: 24 * 7 // 7 days
                        value: 2
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
                            initialItem.updateFee()
                        }
                    }
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
                            enabled: enableExtending
                            onClicked: {
                                channelRentingModel.extendTime(
                                            fundingTxOutpoint,
                                            comboBox.currentAssetID,
                                            durationSlider.value)
                                stackView.push(
                                            "qrc:/Components/PopupProgressBarComponent.qml",
                                            {
                                                "progressBarMsg": "Extending time ..."
                                            })
                            }
                        }
                    }
                }
            }
            function updateFee() {
                channelRentingModel.updateExtendedRentingFee(
                            fundingTxOutpoint, comboBox.currentAssetID,
                            durationSlider.value)
            }
        }
    }
}
