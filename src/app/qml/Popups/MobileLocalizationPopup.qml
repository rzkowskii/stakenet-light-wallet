import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls 2.2 as NewControls

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root

    width: mainWindow.width - 30
    height: mainWindow.height - 60
    dialogBackground.radius: 8
    dialogBackground.color:  SkinColors.mobileSecondaryBackground
    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }
    FontLoader { id: lightFont; source: "qrc:/Rubik-Light.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 15
        anchors.bottomMargin: 10
        spacing: 15

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            MobileTitle {
                Layout.alignment: Qt.AlignHCenter
                text: "localizations"
            }

            SecondaryLabel {
                text: "Select your desired currency"
                font.pixelSize:  14
                font.family: lightFont.name
            }
        }

        Rectangle {
            Layout.leftMargin: 5
            Layout.rightMargin: 5
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: SkinColors.mobileLineSeparator
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            model: CurrencyModel {
                id: currencyModel
                Component.onCompleted: initialize(ApplicationViewModel)
                onModelReset: {
                    listView.currentIndex = getInitial(ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode);
                }
            }

            delegate: Item {
                id: delegateItem
                width: parent.width
                height: 41

                Rectangle {
                    anchors.fill: parent
                    color: delegateItem.ListView.isCurrentItem ? SkinColors.mobileLocalizationSelectedItemBackground : "transparent"
                    radius: 8
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15

                    Row {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 0

                        Text {
                            text: model.code
                            font.pixelSize: 14
                            font.family: delegateItem.ListView.isCurrentItem ? mediumFont.name : regularFont.name
                            font.capitalization: Font.AllUppercase
                            color: delegateItem.ListView.isCurrentItem ? SkinColors.mainText : SkinColors.menuItemText
                            font.weight: delegateItem.ListView.isCurrentItem ? Font.Medium : Font.Normal
                        }

                        Text {
                            font.pixelSize: 14
                            font.family: delegateItem.ListView.isCurrentItem ? mediumFont.name : regularFont.name
                            text: " - %1" .arg(model.name)
                            color: delegateItem.ListView.isCurrentItem ? SkinColors.mainText : SkinColors.menuItemText
                            font.weight: delegateItem.ListView.isCurrentItem ? Font.Medium : Font.Normal
                            font.capitalization: Font.Capitalize
                        }
                    }

                    Item {
                        visible: delegateItem.ListView.isCurrentItem
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        Layout.preferredWidth: 14

                        Image {
                            anchors.centerIn: parent
                            source: "qrc:/images/IC_CHECK.png"
                            sourceSize: Qt.size(14, 14)
                        }
                    }
                }

                PointingCursorMouseArea {
                    onClicked:
                    {
                        listView.currentIndex = index;
                        ApplicationViewModel.localCurrencyViewModel.changeLocalCurrency(currencyModel.getCode(index))
                    }
                }
            }
        }

        MobileActionButton {
            Layout.preferredHeight: 41
            Layout.fillWidth: true
            Layout.leftMargin: 15
            Layout.rightMargin: 15
            buttonColor: SkinColors.menuBackgroundGradientFirst
            buttonText: "DONE"
            onClicked: close()
        }
    }
}
