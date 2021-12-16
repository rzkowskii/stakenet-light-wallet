import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.0

import Components 1.0

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0


Item  {
    width: parent.width
    height: 70
    anchors.bottom: parent.bottom
    anchors.horizontalCenter: parent.horizontalCenter

    property int currentIndex: 2
    property string currentName: "Wallets"

    Rectangle {
        id: root
        anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        anchors.bottomMargin: 15
        color: SkinColors.secondaryBackground
        opacity: 0.81
        radius: 8
        z: 2000

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            MobileMenuItem {
                Layout.fillHeight: true
                isCurrentItem: currentName === name
                Layout.preferredWidth: parent.width / 3
                name: "Dashboard"
                imageSource: isCurrentItem ? "qrc:/images/dashboard_active.svg" : "qrc:/images/dashboard_inactive.svg"
                onMenuItemClicked: { currentIndex = 0; currentName = name }
                visible: false
            }

            MobileMenuItem {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 3
                isCurrentItem: currentName === name
                name: "Portfolio"
                imageSource: isCurrentItem ? "qrc:/images/PORTFOLIO_ACTIVE.png" : "qrc:/images/PORTFOLIO_40.png"
                onMenuItemClicked: { currentIndex = 1; currentName = name }
            }

            MobileMenuItem {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 3
                isCurrentItem: currentName === name
                name: "Wallets"
                imageSource:  isCurrentItem ? "qrc:/images/WALLETS_ACTIVE.png" : "qrc:/images/WALLETS_40.png"
                onMenuItemClicked: { currentIndex = 2; currentName = name }
            }

            MobileMenuItem {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 3
                isCurrentItem: currentName === name
                name: "Lightning"
                imageSource: isCurrentItem ? "qrc:/images/LIGHTNING_ACTIVE.png" : "qrc:/images/LIGHTNING_40.png"
                onMenuItemClicked: { currentIndex = 3; currentName = name }
                visible: !isMobile
            }

            MobileMenuItem {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 3
                isCurrentItem: currentName === name
                name: "XSN DEX"
                imageSource: isCurrentItem ? "qrc:/images/xsncloud_active.svg" : "qrc:/images/xsncloud_inactive.svg"
                onMenuItemClicked: { currentIndex = 4; currentName = name }
                visible: false
            }

            MobileMenuItem {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 3
                isCurrentItem: currentName === name
                name: "Settings"
                imageSource: isCurrentItem ? "qrc:/images/SETTINGS_ACTIVE.png" : "qrc:/images/SETTINGS_40.png"
                onMenuItemClicked: { currentIndex = 5; currentName = name }
            }

            MobileMenuItem {
                id: updateBtn
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 3
                name: "Update wallet"
                imageSource: "qrc:/images/update.svg"
                visible: false//AppUpdater.updaterState === AppUpdater.Ready
                onMenuItemClicked: {
                    AppUpdater.startUpdating();
                }
            }
        }
    }
}
