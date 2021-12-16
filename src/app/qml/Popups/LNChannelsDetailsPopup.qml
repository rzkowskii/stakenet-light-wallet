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

    width: 900
    height: 415

    property int assetID
    property string pubKey
    property int currentFeeSatsPerByte

    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }

    Component {
        id: openChannelDialogComponent
        OpenChannelPopup {

        }
    }

    PaymentNodeViewModel {
        id: paymentNodeViewModel
        currentAssetID: assetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }

        onAllChannelsClosed: {
            navigateToResultComponent("All channels were closed!", "Success!", "qrc:/images/SUCCESS_ICON.png");
        }

        onAllChannelsFailedToClose: {
            navigateToResultComponent(errMsg, "Failed!", "qrc:/images/crossIcon.png");
        }

        onChannelClosed: {            
            navigateToResultComponent("Channel was closed!", "Success!", "qrc:/images/SUCCESS_ICON.png");
        }

        onChannelFailedToClose: {
            navigateToResultComponent(errMsg, "Failed!", "qrc:/images/crossIcon.png");
        }
    }

    StackLayout {
        id: channelsStackLayout
        anchors.fill: parent
        currentIndex: 0
        anchors.margins: 10

        ColumnLayout {
            id: initialComponent
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 10

            RowLayout{
                Layout.maximumHeight: 60
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                XSNLabel {
                    text: "Active Channels"
                    Layout.alignment: Qt.AlignLeft
                    font.pixelSize: 20
                    font.bold: true
                }

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

                CloseButton {
                    Layout.preferredHeight: 25
                    Layout.preferredWidth: 25
                    Layout.alignment: Qt.AlignRight
                    onClicked: root.close()
                }
            }

            SecondaryLabel {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                font.pixelSize: 14
                color: SkinColors.lightningPageInfoText
                text: "PUBLIC KEY"
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

                    XSNLabel {
                        id: pubKeyLbl
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        font.pixelSize: 14
                        text: pubKey
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
                                Clipboard.setText(pubKeyLbl.text);
                                showBubblePopup("Copied");
                            }
                        }
                    }
                }
            }

            SecondaryLabel {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                font.pixelSize: 14
                color: SkinColors.lightningPageInfoText
                text: "CHANNELS"
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.minimumHeight: 120
                color: "transparent"
                border.color: SkinColors.popupFieldBorder
                border.width: 1

                Item {
                    visible: lightningChannelsListView.count === 0
                    anchors.fill: parent

                    XSNLabel {
                        anchors.centerIn: parent
                        text: "No channels found"
                        color: "#F63252"
                        font.pixelSize: 12
                    }
                }
            }

            RowLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true

                IntroButton {
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 144
                    Layout.alignment: Qt.AlignLeft
                    Layout.bottomMargin: 20
                    Layout.topMargin: 20
                    font.capitalization: Font.MixedCase
                    font.pixelSize: 12
                    text: "Close all channels"
                    visible: channelsModel.count !== 0

                    onClicked: {
                        channelsStackLayout.currentIndex = 3;
                        confirmComponent.confirmDeletingAction = function(){ paymentNodeViewModel.closeAllChannels(currentFeeSatsPerByte) };
                    }
                }

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

                IntroButton {
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 144
                    Layout.alignment: Qt.AlignRight
                    Layout.bottomMargin: 20
                    Layout.topMargin: 20
                    font.capitalization: Font.MixedCase
                    font.pixelSize: 12
                    text: "Open channel"

                    onClicked: {
                        var popup = openDialog(openChannelDialogComponent, {assetID : assetID});
                        popup.visibleChanged.connect(function() {
                            if(!popup.visible) {
                                channelsModel.refresh();
                            }
                        })
                    }
                }
            }
        }

        PopupProgressBarComponent {
            id: progressComponent
            progressBarMsg: "Closing channels ..."
        }

        OperationResultComponent {
            id: closeComponentResult
        }

        ColumnLayout {
            id: confirmComponent
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 15

            property string channelsAmount: ""
            property var confirmDeletingAction

            XSNLabel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Layout.topMargin: 15
                font.family: mediumFont.name
                font.pixelSize: 22
                text: "Are you sure you want close all channels?"
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
                        channelsStackLayout.currentIndex = 0;
                    }
                }

                IntroButton {
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 50
                    font.family: mediumFont.name
                    text: qsTr("Close")
                    onClicked: {
                        channelsStackLayout.currentIndex = 1;
                        confirmComponent.confirmDeletingAction();
                    }
                }
            }
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
