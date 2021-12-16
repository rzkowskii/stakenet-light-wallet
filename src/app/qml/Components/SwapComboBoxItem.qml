import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

Item {
    property string coinName: ""
    property string coinAmount: ""
    property string coinValue: ""

    property bool defaultItem: false

    anchors.leftMargin: 25
    anchors.rightMargin: 25

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: defaultItem ? 0 : 1

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            Text {
                anchors.centerIn: parent
                text: "Select coin"
                color: SkinColors.mainText
                font.pixelSize: 18
                font.family: fontRegular.name
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 25

            Image {
                source: coinName !== "" ? "qrc:/images/ICON_%1.svg".arg(
                                              coinName) : ""
                sourceSize: Qt.size(37, 41)
            }

            Column {
                Text {
                    id: name
                    text: "%1 %2".arg(coinValue).arg(coinName)
                    color: SkinColors.mainText
                    font.pixelSize: 15
                    font.family: fontRegular.name
                }

                Text {
                    text: coinAmount
                    color: SkinColors.mainText
                    font.pixelSize: 14
                    font.family: fontRegular.name
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
