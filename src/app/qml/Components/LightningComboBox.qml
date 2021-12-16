import QtQuick 2.12
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.models 1.0
import com.xsn.utils 1.0

ComboBox {
    id: control

    property string currentSymbol: ""
    property string currentName: ""
    property string currentColor: ""
    property int currentAssetId: -1
    property string currentBalance: ""

    onCurrentIndexChanged: {
        fillCurrentIndexData();
    }

    property bool isRightView: false

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
                source: "qrc:/images/ICON_%1.svg" .arg(model.name)
                sourceSize: Qt.size(35, 40)
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
                    text: "%1  %2".arg(model.balance).arg(model.symbol)
                    font.capitalization: Font.AllUppercase
                }
            }
        }


        background: Rectangle {
            color: parent.highlighted ? currentColor : SkinColors.lightningPageWarningGradientFirst
            opacity: 0.3

            LinearGradient {
                visible: delegate.highlighted
                anchors.fill: parent
                start: Qt.point(width, 0)
                end: Qt.point(0, 0)
                gradient: Gradient {
                    GradientStop { position: 1.0; color: isRightView ? currentColor : SkinColors.lightningPageWarningGradientFirst}
                    GradientStop { position: 0.0; color: isRightView ? SkinColors.lightningPageWarningGradientFirst : currentColor  }
                }
            }
        }

        highlighted: control.highlightedIndex === index
    }

    contentItem: XSNLabel {
        text: "%1 - %2" .arg(control.displayText) .arg(control.currentSymbol)
        font: control.font
        color:  SkinColors.mainText
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    background: RowLayout {
        spacing: 1

        Rectangle {
            visible: isRightView
            Layout.alignment: Qt.AlignLeft
            Layout.preferredWidth: 1
            Layout.preferredHeight: parent.height
            color: currentColor
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            radius: 2
            color: "transparent"
            opacity: 0.3

            LinearGradient {
                anchors.fill: parent
                start: Qt.point(width, 0)
                end: Qt.point(0, 0)
                gradient: Gradient {
                    GradientStop { position: 1.0; color: isRightView ? currentColor : SkinColors.lightningPageWarningGradientFirst }
                    GradientStop { position: 0.0; color: isRightView ? SkinColors.lightningPageWarningGradientFirst : currentColor  }
                }
            }
        }

        Rectangle {
            visible: !isRightView
            Layout.alignment: Qt.AlignRight
            Layout.preferredWidth: 1
            Layout.preferredHeight: parent.height
            color: currentColor
        }
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
            color:  SkinColors.transactionItemSelectedBackground
        }
    }

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 10
        height: 7
        contextType: "2d"

        Connections {
            target: control
            function onPressedChanged(){
                canvas.requestPaint()
            }
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = control.pressed ? SkinColors.mainText : SkinColors.comboboxIndicatorColor;
            context.fill();
        }
    }

    function fillCurrentIndexData() {
        var currentItem = model.get(currentIndex);
        currentSymbol = currentItem.symbol;
        currentName = currentItem.name;
        currentColor = currentItem.color;
        currentAssetId = currentItem.id;
        currentBalance = currentItem.lightningBalance;
    }
}
