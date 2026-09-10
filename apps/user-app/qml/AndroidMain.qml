import QtQuick
import QtQuick.Controls

ApplicationWindow {
  id: window
  objectName: "androidWindow"
  visible: true
  width: 430
  height: 860
  color: Theme.paper
  title: "智充出行"
  function goBack() {
    if (Qt.inputMethod.visible || nativeMobile.keyboardInset > 0) {
      Qt.inputMethod.hide()
    } else if (mobile.handleSystemBack()) {
      exitTimer.stop()
    } else if (exitTimer.running) {
      Qt.quit()
    } else {
      mobile.notification('再次返回退出应用')
      exitTimer.restart()
    }
  }
  onClosing: function(close) {
    close.accepted = false
    goBack()
  }
  Shortcut {
    sequence: 'Back'
    context: Qt.WindowShortcut
    onActivated: window.goBack()
  }
  Timer { id: exitTimer; interval: 2000 }
  Loader {
    anchors.fill: parent
    // Android 30+ keeps the Qt surface fixed and animates scene insets. Earlier
    // Android versions retain native adjustResize and report no extra inset.
    anchors.bottomMargin: Math.min(window.height, nativeMobile.keyboardInset)
    source: "qrc:/qml/Main.qml"
  }
}
