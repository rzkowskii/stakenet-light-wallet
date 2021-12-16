import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Page {
    id: root

    property WalletDexViewModel walletDexViewModel

    background: Rectangle {
        color: SkinColors.botPageBackground
    }

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    FontLoader {
        id: fontMedium
        source: "qrc:/Rubik-Medium.ttf"
    }

    FontLoader {
        id: fontLight
        source: "qrc:/Rubik-Light.ttf"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 35
        spacing: 0.02 * root.height

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 0.03 * root.height
            spacing: 5

            Text {
               Layout.fillWidth: true
               text: "Vortex"
               font.capitalization: Font.AllUppercase
               color: SkinColors.mainText
               font.pixelSize: 30
               font.family: fontLight.name
               font.weight: Font.Light
            }
        }

//        MainBotView {
//            Layout.fillHeight: true
//            Layout.fillWidth: true
//            walletDexViewModel: root.walletDexViewModel
//        }
    }
}
