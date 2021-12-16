import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Page {
    id: root

    background: Rectangle {
        color: "transparent"
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 35
        anchors.bottomMargin: 30
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 25

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Skins"
            font.family: regularFont.name
            font.pixelSize: 14
            color: SkinColors.mainText
            font.weight: Font.Medium
        }


        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: 750
            clip: true

            SkinsView {
                id: skinsView
                anchors.fill: parent
            }
        }

        MobileFooter {
            Layout.leftMargin: 13
            Layout.preferredHeight: 40
            leftButton.text: "back"
            onLeftButtonClicked: {
                navigateBack()
            }
        }
    }
}
