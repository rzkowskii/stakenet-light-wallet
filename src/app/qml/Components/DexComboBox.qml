import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0
import com.xsn.models 1.0
import com.xsn.viewmodels 1.0

ComboBox {
    id: control
    property var currentStatus: undefined

    FontLoader { id: fontRegular; source: "qrc:/Rubik-Regular.ttf" }

    Connections {
        target: walletDexViewModel
        function onSwapAssetsChanged() {
            if(!walletDexViewModel.hasSwapPair)
            {
                content.editText.textFormat = TextEdit.PlainText
                content.text = currentComboBoxText = currentSwapPair = "";
            }
        }
    }

    Connections {
        target: swapAssetsModel
        function onDexStatusChanged(){
            update();
        }
    }

    property alias comboBoxBackground: backGrd
    property alias contentSearch : content
    property string actualComboBoxText: ""
    property string currentComboBoxText: ""
    property string currentSwapPair: ""

    textRole: "swapPair"
    font.pixelSize: 14

    delegate: ItemDelegate {
        property bool isPairEnable: isSwapPairEnable(model.swapPair)
        width: control.width
        enabled: isPairEnable

        contentItem: Text {
            anchors.left: parent.left
            anchors.leftMargin: 15
            text: '%1 - %2'.arg(model.swapPair).arg(isPairEnable ? ('<font color="%1">%2</font>'.arg(getColor(model.dexStatus)).arg(getText(model.dexStatus))) : "Coming soon")
            font.pixelSize: 14
            font.family: fontRegular.name
            color: parent.highlighted ? SkinColors.mainText : SkinColors.secondaryText
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: parent.highlighted ? SkinColors.headerBackground : SkinColors.menuBackground
            opacity: 0.5
        }

        onClicked: {
            currentStatus = model.dexStatus
            currentSwapPair = model.swapPair
            currentComboBoxText = '%1 - <font color="%2">%3</font>'.arg(model.swapPair).arg(getColor(model.dexStatus)).arg(getText(model.dexStatus))
        }

        highlighted: control.highlightedIndex === index
    }

    contentItem: DexStyledTextEdit {
        id: content
        anchors.fill: parent
        onFocusChanged: {
            if(focus && !popup.opened)
            {
                if(text.includes(currentSwapPair))
                {
                    actualComboBoxText = currentComboBoxText
                    editText.textFormat = TextEdit.PlainText
                    text = ""
                }
                popup.open();
                return;
            }
            if(!text.includes(currentSwapPair) && !focus)
            {
                editText.textFormat = TextEdit.RichText
                text = currentComboBoxText
                popup.close()
            }
        }
    }

    background: Rectangle {
        id: backGrd
        implicitWidth: 200
        implicitHeight: 35
        radius: 10
        color: SkinColors.menuBackground
        border.color: SkinColors.comboboxIndicatorColor
        opacity: 0.5
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
            color: SkinColors.headerBackground
            border.color: SkinColors.comboboxIndicatorColor
        }

        onClosed: content.focus = false
        onOpened: MouseEventSpy.setEventFilerEnabled(false)
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
            function onPressedChanged() {
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

    function getText(status) {
        switch(status)
        {
        case Enums.DexTradingStatus.Online: return "Online";
        case Enums.DexTradingStatus.Offline: return "Offline";
        case Enums.DexTradingStatus.Syncing: return "Syncing";
        default:
            break;
        }

        return ""
    }

    function getColor(status) {
        switch(status)
        {
        case Enums.DexTradingStatus.Online: return "#42C451";
        case Enums.DexTradingStatus.Offline: return "#A7A8AE";
        case Enums.DexTradingStatus.Syncing: return "#F19E1E";
        default:
            break;
        }

        return "";
    }

    function update() {
        if(currentComboBoxText !== "")
        {
            var currentItem = swapAssetsModel.getByPair(currentSwapPair)
            var text = '%1 - <font color="%2">%3</font>'.arg(currentItem.swapPair).arg(getColor(currentItem.dexStatus)).arg(getText(currentItem.dexStatus))

            if(content.text !== "" && !content.focus)
            {
                content.text = text;
            }
            actualComboBoxText = currentComboBoxText = text
            currentSwapPair = currentItem.swapPair
            currentStatus = currentItem.dexStatus
        }
    }

    function isSwapPairEnable(swapPair){
        return swapPair !== "BTC/USDT" && swapPair !== "BTC/USDC" && swapPair !== "ETH/USDC" && swapPair !== "LTC/BTC"
    }
}
