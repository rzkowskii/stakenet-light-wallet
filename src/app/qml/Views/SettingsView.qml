import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

StackLayout {

    FontLoader { id: fontRegular; source: "qrc:/Rubik-Regular.ttf" }
    FontLoader { id: fontMedium; source: "qrc:/Rubik-Medium.ttf" }

    AssetsView {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

    OrderbookUXView {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

    LocalizationView {
        id: localizationView
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

//    SkinsView {
//        Layout.fillHeight: true
//        Layout.fillWidth: true
//    }

    PersonalSettingsView {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

    AppVersionInfoView {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}
