import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5
import QtQuick.Dialogs 1.0

import com.xsn.utils 1.0

import "../Components"

ColumnLayout {
    spacing: 50

    ButtonGroup {
        id: group
        buttons: buttonRow.children
    }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true

        Flow {
            id: buttonRow
            anchors.fill: parent
            spacing: 5

            Repeater {
                id: elements
                model: ListModel {
                    ListElement { skinIcon: "qrc:/images/skin_default.png"; skinPath: ":/data/basicSkin.json"; skinName: "Default" }
//                    ListElement { skinIcon: "qrc:/images/skin_auraspace.png"; skinPath: ":/data/auraspace.json"; skinName: "Aura Space" }
//                    ListElement { skinIcon: "qrc:/images/skin_mono.png"; skinPath: ":/data/basicSkin1.json"; skinName: "Mono" }
//                    ListElement { skinIcon: "qrc:/images/skin_red.png"; skinPath: ":/data/basicSkin2.json"; skinName: "Red" }
//                    ListElement { skinIcon: "qrc:/images/skin_golden.png"; skinPath: ":/data/basicSkin3.json"; skinName: "Golden" }
//                    ListElement { skinIcon: "qrc:/images/skin_green.png"; skinPath: ":/data/basicSkin4.json"; skinName: "Green" }
//                    ListElement { skinIcon: "qrc:/images/skin_gradient.png"; skinPath: ":/data/basicSkin5.json"; skinName: "Gradient" }
//                    ListElement { skinIcon: "qrc:/images/skin_graphite.png"; skinPath: ":/data/graphiteSkin.json"; skinName: "Graphite" }
//                    ListElement { skinIcon: "qrc:/images/skin_hema.png"; skinPath: ":/data/hemaSkin.json"; skinName: "Hema" }
//                    ListElement { skinIcon: "qrc:/images/skin_ray.png"; skinPath: ":/data/raySkin.json"; skinName: "Ray" }
//                    ListElement { skinIcon: "qrc:/images/skin_slateblue.png"; skinPath: ":/data/slateblueSkin.json"; skinName: "Slateblue" }
//                    ListElement { skinIcon: "qrc:/images/skin_haze.png"; skinPath: ":/data/hazeSkin.json"; skinName: "Haze" }
                }

                delegate: SkinButton {
                    skinPath: model.skinPath
                    onClicked: {
                        SkinColors.initSkin(model.skinPath, model.skinName)
                    }
                }
            }
        }
    }

    IntroButton {
        visible: !isMobile
        text: qsTr("Import skin")
        Layout.topMargin: 30
        Layout.bottomMargin: 50
        Layout.preferredWidth: 150
        Layout.preferredHeight: 50
        onClicked: {
            fileDialog.open();
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.desktop
        onAccepted: {
            SkinColors.initSkin(fileDialog.fileUrl, "Custom");

        }
        onRejected: {
        }
    }
}
