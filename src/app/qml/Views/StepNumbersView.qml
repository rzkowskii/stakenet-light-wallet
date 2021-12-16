import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.utils 1.0

ListView {
    id: listView
    orientation: ListView.Horizontal
    boundsBehavior: Flickable.StopAtBounds
    spacing: 10

    property int currentStep: 0

    FontLoader {
        id: lightFont
        source: "qrc:/Rubik-Light.ttf"
    }

    delegate: Column {
        spacing: 10

        Row {
            spacing: 10

            Rectangle {
                id: circle
                width: 35
                height: 35
                color: currentStep > index ? "#00DC9F" : SkinColors.secondaryBackground
                radius: 17
                border.width: 2
                border.color: currentStep > index ? "#00DC9F" : (currentStep === index ? SkinColors.mainText : SkinColors.secondaryText)

                Text {
                    anchors.centerIn: parent
                    font.pixelSize: 18
                    text: index + 1
                    font.family: lightFont.name
                    color: currentStep === index ? SkinColors.mainText : SkinColors.secondaryText
                    visible: currentStep <= index
                }

                ColorOverlayImage {
                    width: 20
                    height: 20
                    x: 8
                    y: 8
                    imageSize: 23
                    color: SkinColors.mainText
                    imageSource: "qrc:/images/checkmark-round.png"
                    visible: currentStep > index
                }
            }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                height: 4
                width: 85
                color: currentStep > index ? "#00DC9F" : SkinColors.secondaryText
                radius: 2
                visible: index != count - 1
            }
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 17 - contentWidth / 2
            font.pixelSize: 12
            text: modelData
            font.family: lightFont.name
            color: currentStep >= index ? SkinColors.mainText : SkinColors.secondaryText
        }
    }
}
