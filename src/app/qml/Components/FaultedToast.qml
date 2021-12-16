import QtQuick 2.12

Toast {
    id: errorToast
    function show(error) {
        text = error
        open()
    }

    Connections {
        target: settingsController
        function onFaulted() {
            show(error)
        }
    }
    Connections {
        target: lockController
        function onFaulted() {
            show(error)
        }
    }
    Connections {
        target: contactsController
        function onFaulted() {
            show(error)
        }
    }
    Connections {
        target: accountController
        function onFaulted() {
            show(error)
        }
    }
    Connections {
        target: mailController
        function onFaulted() {
            show(error)
        }
    }
    Connections {
        target: voiceMessagesController
        function onError() {
            show(error)
        }
    }
    Connections {
        target: vorbisRecorder
        function onError() {
            show(error)
        }
    }
}
