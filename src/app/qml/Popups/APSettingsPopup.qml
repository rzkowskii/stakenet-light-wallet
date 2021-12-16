import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"
import "../Views"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root

    width: 420
    height: 300

    property int assetID: -1
    property int currentIndex: -1
    property string currentName: assetModel.get(currentIndex).name
    property PaymentNodeViewModel paymentNodeViewModel

    WalletAssetsListModel {
        id: assetModel
        onAverageFeeForAssetFinished: {
            everageFee = value;
        }

        Component.onCompleted: {
            initialize(ApplicationViewModel);
            root.currentIndex = getInitial(assetID)
        }
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.topMargin: 20
        anchors.bottomMargin: 10
        spacing: 20

        SliderBoxView {
            Layout.fillWidth: true
            Layout.preferredHeight: 85
            title: "AP channel capacity"

            CustomizedSlider {
                id: capacityPercentSlider
                anchors.centerIn: parent
                anchors.topMargin: 5
                from: 1
                to: 100
                value: paymentNodeViewModel.stateModel.autopilotDetails.allocation * 100
                stepSize: 1
                startText: "Min"
                finishText: "Max"
                valueSymbol: "%"
            }
        }

        SliderBoxView {
            Layout.fillWidth: true
            Layout.preferredHeight: 85
            title: "Maximum channels".arg(currentName)

            CustomizedSlider {
                id: maxChannelsSlider
                anchors.centerIn: parent
                anchors.topMargin: 5
                from: 1
                to: 100
                value: paymentNodeViewModel.stateModel.autopilotDetails.maxChannels
                stepSize: 1
                startText: "Min"
                finishText: "Max"
                valueSymbol: "channel/s"
            }
        }

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true

            XSNLabel {
                Layout.leftMargin: 5
                Layout.preferredWidth: 100
                font.family: regularFont.name
                font.pixelSize: 12
                text: "Recommended capacity:  %1 %".arg("50")
            }

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

                    onClicked: {
                        close()
                    }
                }

                IntroButton {
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 70
                    font.pixelSize: 12
                    text: "Ok"
                    onClicked: {
                        paymentNodeViewModel.changeAutopilotDetails((capacityPercentSlider.value / 100).toFixed(2), maxChannelsSlider.value);
                        close();
                    }
                }

            }
        }
    }
}
