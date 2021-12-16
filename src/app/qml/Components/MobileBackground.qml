import QtQuick 2.12
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

LinearGradient {
    start: Qt.point(0, 0)
    end: Qt.point(0, height)
    gradient: Gradient {
        GradientStop { position: 0.0; color: SkinColors.mobileTopGradientBackground }
        GradientStop { position: 0.5; color: SkinColors.mobileMiddleGradientBackground }
        GradientStop { position: 1.0; color: SkinColors.mobileBottomGradientBackground }
    }
}
