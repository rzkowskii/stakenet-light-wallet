import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ColumnLayout {
    spacing: 10
    property double feeRate: 0
    property alias currentOption: choosingListView.currentOption
    signal currentOptionFeeChanged()

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    
    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 15
        
        XSNLabel {
            text: "NETWORK FEE RATE :"
            font.family: regularFont.name
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: SkinColors.secondaryText
        }
        
        SelectedText {
            id: networkFeeRateText
            selectByMouse: feeRate > 0
            font.family: regularFont.name
            font.pixelSize: 12
            color: SkinColors.mainText
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: feeRate > 0 ? "%1  %2".arg(
                                           feeRate).arg(
                                           "satoshi/vbyte") : "Automatic"
        }
    }
    
    ChoosingListView {
        id: choosingListView
        Layout.preferredWidth: 230
        Layout.preferredHeight: 25
        color: SkinColors.mainBackground
        highlightBorderColor: SkinColors.popupFieldBorder
        highlighRectangleColor: SkinColors.menuBackground
        model: ["Low", "Medium", "High"]
        textItemPixelSize: 12
        textItemCapitalization: Font.AllUppercase
        currentIndex: 1
        onCurrentOptionChanged: currentOptionFeeChanged()
    }
}
