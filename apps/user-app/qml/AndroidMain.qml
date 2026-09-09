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
  Loader {
    anchors.fill: parent
    source: "qrc:/qml/Main.qml"
  }
}
