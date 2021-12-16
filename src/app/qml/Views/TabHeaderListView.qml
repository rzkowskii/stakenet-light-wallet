import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"

import com.xsn.utils 1.0

ListView {
    id: listView
    currentIndex: 0

    property string highlightColor: SkinColors.menuBackgroundGradientFirst
    property int delegateWidth: 210

    clip: true
    spacing: -33
    orientation: ListView.Horizontal
    boundsBehavior: Flickable.StopAtBounds

    delegate: WalletTabDelegate {
        id: delegate
        z: isCurrentTab ? count : count - index
        width: delegateWidth
        height: listView.height
        isCurrentTab: ListView.isCurrentItem
        onSelected: listView.currentIndex = index
    }
}
