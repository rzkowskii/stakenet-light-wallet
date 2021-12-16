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

    width: 520
    height: 350

    property int assetID: 0
    property int currentIndex: -1
    property string currentName: assetModel.get(currentIndex).name
    property PaymentNodeViewModel paymentNodeViewModel

    WalletAssetsListModel {
        id: assetModel

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
        spacing: 15

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 35

            Item {
                Layout.fillWidth: true
            }

            CloseButton {
                Layout.preferredHeight: 20
                Layout.preferredWidth: 20
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                onClicked: {
                    autopilotSwitch.checked = false;
                    root.close();
                }
            }
        }

        Text {
            lineHeight: 1.4
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 16
            color: SkinColors.mainText
            textFormat: Text.RichText
            font.family: regularFont.name
            wrapMode: Text.WordWrap
            text: 'You have enabled Auto-Pilot which is about to open a Lightning Channel with an outbound capacity of <b><font color="%2">%1%</font></b> of your balance.(You can change this value in LN autopilot settings.)'.
            arg(paymentNodeViewModel.stateModel.autopilotDetails.allocation * 100).arg(SkinColors.secondaryText)

        }

        RowLayout {
            Layout.preferredHeight: 20
            Layout.fillWidth: true
            Layout.leftMargin: 20
            spacing: 5

            CheckBox {
                id: checkbox
                checked: false

                indicator: Rectangle {
                    implicitWidth: 17
                    implicitHeight: 17
                    x: checkbox.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 6
                    border.color: SkinColors.mainText
                    color: "transparent"
                    border.width: 1


                    ColorOverlayImage {
                        width: 15
                        height: 15
                        x: 2
                        y: 1
                        imageSize: 15
                        color: SkinColors.mainText
                        imageSource: "qrc:/images/checkmark-round.png"
                        visible: checkbox.checked
                    }
                }

                PointingCursorMouseArea {
                    onClicked:  {
                       checkbox.checked = !checkbox.checked
                    }

                    onEntered:  {
                        checkbox.scale = scale * 1.1
                    }
                    onExited: {
                        checkbox.scale = scale
                    }
                }
            }

            XSNLabel {
                font.weight: Font.Thin
                text: "Ignore this message in the future"
                font.family: regularFont.name
                font.pixelSize: 12
                color: SkinColors.mainText

            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 45
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            spacing: 10

            IntroButton {
                Layout.preferredWidth: 210
                Layout.preferredHeight: 45
                text: qsTr("Cancel")

                onClicked: {
                    if(checkbox.checked)
                    {
                        paymentNodeViewModel.changeApPreOpenPopupVisibility();
                    }
                    autopilotSwitch.checked = false;
                    root.close();
                }
            }

            IntroButton {
                Layout.preferredWidth: 210
                Layout.preferredHeight: 45
                text: qsTr("Confrim")

                onClicked: {
                    if(checkbox.checked)
                    {
                        paymentNodeViewModel.changeApPreOpenPopupVisibility();
                    }

                    paymentNodeViewModel.activateAutopilot(true);
                    root.close();
                }
            }
        }

    }
}
