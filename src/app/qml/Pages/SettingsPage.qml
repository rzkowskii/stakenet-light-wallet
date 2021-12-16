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

    property alias rootLayout: layout

    background: Rectangle {
        color: SkinColors.menuBackground
        opacity: 0.2
    }

    FontLoader { id: fontRegular; source: "qrc:/Rubik-Regular.ttf" }
    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }
    FontLoader { id: lightFont; source: "qrc:/Rubik-Light.ttf" }

    Component {
        id: lnChannelsFeeComponent

        LNChannelsFeesPopup {
        }
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        spacing: 1

        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: 268
            Layout.fillWidth: isMobile

            Rectangle {
                anchors.fill: parent
                color: isMobile ? "transparent" : SkinColors.mainBackground
            }

            Item {
                anchors.fill: parent
                anchors.leftMargin: isMobile ? 16 : 40
                anchors.rightMargin: isMobile ? 16 : 0
                anchors.topMargin: 48

                ColumnLayout {
                    anchors.fill: parent
                    spacing: isMobile ? 32 : 64

                    SettingsHeader {
                        Layout.fillWidth: true
                    }

                    SettingsOptionsListView {
                        id: settingsListView
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                    }
                }
            }
        }

        SettingsView {
            id: settingsView
            visible: !isMobile
            Layout.topMargin: 48
            Layout.leftMargin: 30
            Layout.rightMargin: 20
            Layout.bottomMargin: 20
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: settingsListView.actualIndex
            onCurrentIndexChanged: forceActiveFocus()
        }
    }

    function openLNChannelsFeeDialog(params){
        return openDialog(lnChannelsFeeComponent, params);
    }
}

