import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3

import "Pages"
import "Pages/Intro"
import "Popups"
import "Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Window {
    id: mainWindow
    visible: true
    property bool small: false
    property var activePopup: undefined

    width: isMobile ? 360 : (small ? 840 : 1240)
    height: isMobile ? 640 : 710

    minimumWidth: isMobile ? 360 : 1240
    minimumHeight: isMobile ? 640 : 710

    title: qsTr("Stakenet")

    Rectangle {
        anchors.fill: parent

        Image {
            id: name
            anchors.fill: parent
            source: SkinColors.mainBackgroundPic
        }
    }

    Connections {
        target: ApplicationViewModel
        function onLoadContextFinished() {
            navigateToStartedPage()
        }

        function onDestroyContextFinished() {
            replaceView(loadingComponent)
        }
    }

    Component {
        id: loadingComponent
         LoadingComponent {

         }
    }

    Component {
        id: loadingPage
         LoadingPage {

         }
    }

    Component {
        id: unlockWalletPage
         UnlockWalletPage {

         }
    }

    Component {
        id: mainPage

        MainPage {}
    }

    Component {
        id: introScreen

        IntroScreen {}
    }

    StackView {
        id: rootStackView
        anchors.fill: parent
        Component.onCompleted: {
            navigateToStartedPage()
            //navigateToItem("Pages/Intro/IntroScreen.qml");
            //navigateToItem("Pages/MainPage.qml");
        }
        initialItem: Component {
            LoadingWalletComponent {}
        }

        pushEnter: Transition {
            XAnimator {
                from: (rootStackView.mirrored ? -1 : 1) * rootStackView.width
                to: 0
                duration: 400
                easing.type: Easing.OutCubic
            }
        }

        pushExit: Transition {
            XAnimator {
                from: 0
                to: (rootStackView.mirrored ? -1 : 1) * -rootStackView.width
                duration: 800
                easing.type: Easing.OutCubic
            }
        }
    }

    FontLoader {
        id: mediumFont
        source: "qrc:/Rubik-Medium.ttf"
    }
    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    function navigateToStartedPage(){
        if(ApplicationViewModel.lockingViewModel.isEncrypted){
             replaceView(unlockWalletPage)
        }
        else {
            replaceView(loadingPage)
        }
    }

    function navigateBackImmediate() {
        rootStackView.pop(StackView.Immediate)
    }

    function navigateBack() {
        rootStackView.pop()
    }

    function navigateToFirst() {
        rootStackView.pop(null)
    }

    function replaceItem(componentItem, properties) {
        rootStackView.replace(componentItem, properties)
    }

    function replaceView(componentItem, properties) {
        rootStackView.clear()
        rootStackView.push(componentItem, properties)
    }

    function replaceAllExceptFirst(componentItem) {
        navigateToFirst()
        rootStackView.push(componentItem)
    }

    function navigateToItem(componentItem, properties, operation) {
        rootStackView.push(componentItem, properties, operation)
    }

    function openDialog(component, params) {
        if (!params)
            activePopup = component.createObject(mainWindow)
        else
            activePopup = component.createObject(mainWindow, params)
        activePopup.open()

        return activePopup
    }
}
