import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0
import com.xsn.models 1.0
import com.xsn.viewmodels 1.0

Rectangle {
    SequentialAnimation on opacity {
        loops: Animation.Infinite
        PropertyAnimation { to: 1; duration: 0 }
        PauseAnimation { duration: 500 }
        PropertyAnimation { to: 0; duration: 0 }
        PauseAnimation { duration: 500 }
    }
    
    color: "white"
    width: 1
}
