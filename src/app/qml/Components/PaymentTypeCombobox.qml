import QtQuick 2.12

import QtQuick 2.12
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.models 1.0
import com.xsn.utils 1.0

ComboBox {
    id: control

    property var currentItem: control.model ? control.model.get(
                                                  currentIndex) : undefined
    property string currentSymbol: currentItem ? currentItem.symbol : ""
    property string currentName: currentItem ? currentItem.name : ""
    property string currentColor: currentItem
                                  && currentItem.color !== undefined ? currentItem.color : ""
    property int currentAssetId: currentItem ? currentItem.id : -1

    textRole: "name"
    font.pixelSize: 14

    delegate: ItemDelegate {
        id: delegate
        width: control.width

        contentItem: Row {
            id: row
            spacing: 10

            Image {
                id: coinImage
                anchors.verticalCenter: parent.verticalCenter
                source: model.name !== "Lightning" ? "qrc:/images/ic_wallet_payment.svg" : "qrc:/images/ic_lightning_payment.svg"
                sourceSize: Qt.size(35, 40)
                width: 35
                height: 40
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 5

                XSNLabel {
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: "%1 - %2".arg(model.name).arg(model.symbol)
                    color: "white"
                    font: control.font
                    elide: Text.ElideRight
                }

                SecondaryLabel {
                    visible: model.balance !== undefined
                    text: "%1  %2".arg(
                              model.balance !== undefined ? model.balance : "").arg(
                              model.symbol !== undefined ? model.symbol : "")
                    font.capitalization: Font.AllUppercase
                }
            }
        }

        background: Rectangle {
            color: parent.highlighted ? SkinColors.headerBackground : SkinColors.mainBackground
            opacity: 0.5

            border.width: 1
            border.color: SkinColors.popupFieldBorder
        }

        highlighted: control.highlightedIndex === index
    }

    contentItem: Item {

        Row {
            id: contentItemRow
            spacing: 10
            anchors.fill: parent
            anchors.leftMargin: 15

            Image {
                id: contentImage
                anchors.verticalCenter: parent.verticalCenter
                source: control.currentName !== "Lightning" ? "qrc:/images/ic_wallet_payment.svg" : "qrc:/images/ic_lightning_payment.svg"
                sourceSize: Qt.size(35, 40)
                width: 35
                height: 40
            }

            XSNLabel {
                anchors.verticalCenter: parent.verticalCenter
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: "%1 - %2".arg(currentName).arg(currentSymbol)
                color: "white"
                font: control.font
                elide: Text.ElideRight
            }
        }
    }

    background: Rectangle {
        color: "transparent"
        border.color: SkinColors.popupFieldBorder
        border.width: 1
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: contentItem.implicitHeight
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
        }

        background: Rectangle {
            radius: 2
            color: isMobile ? SkinColors.mobileSecondaryBackground : SkinColors.headerBackground

            border.width: 1
            border.color: SkinColors.mainText
        }
    }

    indicator: Image {
        width: 20
        height: 20
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: control.right
        anchors.rightMargin: 10
        source: control.popup.visible ? "qrc:/images/ic_dropdown_close.png" : "qrc:/images/ic_dropdown_open.png"
    }
}
