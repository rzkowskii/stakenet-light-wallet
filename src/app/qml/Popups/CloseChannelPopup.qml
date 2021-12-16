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

    width: 600
    height: 415

    property PaymentNodeViewModel paymentNodeViewModel
    property int assetID
    property string channelOutpoint: ""
    property string confirmMsg: ""
    property bool containsInactiveClosingChannel: false
    property double recommendedNetworkFeeRate: 0
    property double networkFeeRate: 0

    onRecommendedNetworkFeeRateChanged: {
        root.networkFeeRate = calculateNetworkFeeRate(
                    networkFeeRateView.currentOption ? networkFeeRateView.currentOption : "Medium",
                    recommendedNetworkFeeRate)
    }

    WalletAssetsListModel {
        id: assetModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
            averageFeeForAsset(assetID)
        }
        onAverageFeeForAssetFinished: {
            networkFeeRate = recommendedNetworkFeeRate = value
        }
    }

    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }

    Connections {
        target: paymentNodeViewModel

        function onAllChannelsClosed() {
            navigateToResultComponent("All channels were closed!", "Success!", "qrc:/images/SUCCESS_ICON.png");
        }

        function onAllChannelsFailedToClose(errMsg) {
            navigateToResultComponent(errMsg, "Failed!", "qrc:/images/crossIcon.png");
        }

       function onChannelClosed() {
            navigateToResultComponent("Channel was closed!", "Success!", "qrc:/images/SUCCESS_ICON.png");
        }

        function onChannelFailedToClose(errMsg) {
            navigateToResultComponent(errMsg, "Failed!", "qrc:/images/crossIcon.png");
        }

    }

    StackLayout {
        id: channelsStackLayout
        anchors.fill: parent
        currentIndex: 0
        anchors.margins: 10

        ColumnLayout {
            id: confirmComponent
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 15

            XSNLabel {
                Layout.fillWidth: true
                Layout.preferredHeight: 170
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Layout.topMargin: 15
                font.family: mediumFont.name
                font.pixelSize: 22
                text: confirmMsg
            }

            NetworkFeeRateView {
                id: networkFeeRateView
                Layout.fillWidth: true
                Layout.leftMargin: 20
                feeRate: root.networkFeeRate
                onCurrentOptionFeeChanged: {
                    root.networkFeeRate = calculateNetworkFeeRate(
                                currentOption, recommendedNetworkFeeRate)
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
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
                        root.close()
                    }
                }

                IntroButton {
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 50
                    font.family: mediumFont.name
                    text: qsTr("Close")
                    onClicked: {
                        channelsStackLayout.currentIndex = 1;

                        if(channelOutpoint === "")
                        {
                            paymentNodeViewModel.closeAllChannels(networkFeeRate);
                        }
                        else
                        {
                            paymentNodeViewModel.closeChannel(channelOutpoint, containsInactiveClosingChannel, networkFeeRate);
                        }
                    }
                }
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
                text: channelOutpoint === "" ?  "Closing channels ..." : "Closing channel ..."
                font.family: mediumFont.name
                font.pixelSize: 20
            }

            ProgressBar {
                id: progressBar
                indeterminate: true
                Layout.preferredHeight: 10
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignCenter

                background:  Rectangle {
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

        OperationResultComponent {
            id: closeComponentResult
        }
    }

    function navigateToResultComponent(errorMsg, resultMsg, imgPath)
    {
        closeComponentResult.operationMsg = errorMsg;
        closeComponentResult.resultMsg = resultMsg;
        closeComponentResult.imgPath = imgPath;
        closeComponentResult.confirmBtnAction = channelsStackLayout.currentIndex = 0;

        channelsStackLayout.currentIndex = 2;
    }


}
