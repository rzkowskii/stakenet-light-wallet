import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.utils 1.0

ColumnLayout {
    property alias searchArea: searchField
    spacing: 12

    Text {
        text: qsTr("%1 wallets total").arg(assetsListView.count)
        font.pixelSize: 15
        font.family: fontRegular.name
        color: SkinColors.mainText
    }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 35

        Rectangle {
            id: backGroundRect
            anchors.fill: parent
            radius: 10
            gradient: Gradient {
                GradientStop {position: 0.0; color: SkinColors.delegatesBackgroundLightColor}
                GradientStop {position: 1.0; color: SkinColors.botTextFieldActiveBorderColor}
            }
        }

        RowLayout {
            anchors.fill: parent
            spacing: 5

            SearchTextField {
                id: searchField
                Layout.leftMargin: 7
                Layout.fillHeight: true
                Layout.fillWidth: true
                placeholderText: "Search Wallet"
                validator: RegExpValidator {
                    regExp: /[a-zA-Z]{1,10}\D/g
                }
            }

            ColorOverlayImage {
                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: 7
                imageSize: 32
                width: imageSize
                height: imageSize
                imageSource: "qrc:/images/magnifyingGlass.png"
                color: SkinColors.magnifyingGlass
            }
        }
    }
}
