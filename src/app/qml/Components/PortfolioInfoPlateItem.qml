import QtQuick 2.12
import QtQuick.Layouts 1.3

import com.xsn.utils 1.0

Rectangle {
    property string plateHeader: ""
    property string plateMainText: ""
    property string platePercentageNumber: ""

    Layout.preferredHeight: 97
    Layout.fillWidth: true
    color: SkinColors.secondaryBackground
    radius: 8

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 16
        anchors.bottomMargin: 13

        XSNLabel
        {
            Layout.alignment: Qt.AlignHCenter
            text: plateHeader
            font.pixelSize: 11
            color: SkinColors.headerText
            font.family: regularFont.name
        }

        XSNLabel
        {
            Layout.alignment: Qt.AlignHCenter
            text: plateMainText
            font.pixelSize: 20
            font.weight: Font.Thin
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 3

            Image {
                visible: platePercentageNumber
                source: "qrc:/images/DOWN.png"
            }

            XSNLabel
            {
                visible: platePercentageNumber
                font.pixelSize: 14
                text: "%1%2%".arg("-").arg(platePercentageNumber)
                color: "red"
            }

        }
    }
}
