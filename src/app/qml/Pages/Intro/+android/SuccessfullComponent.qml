import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0
import Popups 1.0
import Pages 1.0

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ColumnLayout {
    spacing: 60

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    Image {
        Layout.topMargin: 50
        Layout.alignment: Qt.AlignHCenter
        source: "qrc:/images/AW_RESTORE_SUCCESS.png"
    }

    XSNLabel {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillHeight: true
        font.pixelSize: 20
        text: "Your wallet was \n successfully restored"
        horizontalAlignment: Text.AlignHCenter
        font.weight: Font.Thin
    }

    SecondaryLabel {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        lineHeight: 1.3
        font.family: regularFont.name
        font.pixelSize: 12
        horizontalAlignment: Text.AlignHCenter
        text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi eu elit et dolor elementum molestie. Sed neque massa, rhoncus maximus ipsum nec, bibendum tincidunt justo."
    }

    MobileActionButton {
        Layout.bottomMargin: 15
        Layout.preferredHeight: 41
        Layout.fillWidth: true
        buttonColor: SkinColors.menuBackgroundGradientFirst
        buttonText: "DONE"
        onClicked: {
            navigateToFirst()
            replaceView(mainPage)
        }
    }
}
