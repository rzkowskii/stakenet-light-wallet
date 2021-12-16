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
    height: 170

    property int assetID: 0
    property int currentIndex: -1
    property string currentName: assetModel.get(currentIndex).name
    property string currentFeeSatsPerByte: assetModel.get(currentIndex).satsPerByte
    property string everageFee: ""

    WalletAssetsListModel {
        id: assetModel
        onAverageFeeForAssetFinished: {
            everageFee = value;
        }

        Component.onCompleted: {
            initialize(ApplicationViewModel);
            root.currentIndex = getInitial(assetID)
            averageFeeForAsset(assetID)
        }
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        spacing: 20

        SliderBoxView {
            Layout.fillWidth: true
            Layout.preferredHeight: 85
            title: "%1 Transaction fee".arg(currentName)

            CustomizedSlider {
                id: slider
                anchors.centerIn: parent
                anchors.topMargin: 5
                from: 1
                to: 200
                value: currentFeeSatsPerByte
                startText: "Min"
                finishText: "Max"
                valueSymbol: "satoshi/vbyte"
                stepSize: 1
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
                text: "Recommended fee: %1".arg(everageFee !== "" ? ("%1 satoshi/vbyte".arg(everageFee)) : "...")                
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
                    onClicked: root.close();
                }

                IntroButton {
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 70
                    font.pixelSize: 12
                    text: "Ok"
                    onClicked: {
                        assetModel.changeAssetSatsPerByte(assetID, slider.value);
                        root.close();
                    }
                }
            }
        }
    }
}
