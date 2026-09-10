import QtQuick

Item {
  id: control
  required property var view
  readonly property real pullDistance: Math.max(0, view.originY - view.contentY)
  readonly property bool refreshing: mobile.busy || mobile.loadingStations
  parent: view
  anchors.top: parent.top
  anchors.horizontalCenter: parent.horizontalCenter
  width: label.implicitWidth + 32
  height: 40
  z: 5
  visible: refreshing || view.dragging && pullDistance > 8
  Rectangle {
    anchors.fill: parent
    radius: 20
    color: Theme.primaryLight
    AppText {
      id: label
      anchors.centerIn: parent
      text: control.refreshing ? '正在刷新…' : control.pullDistance >= 48 ? '松手刷新' : '下拉刷新'
      color: Theme.primaryText
    }
  }
  Connections {
    target: control.view
    function onDraggingChanged() {
      if (!control.view.dragging && control.pullDistance >= 48 && !control.refreshing) mobile.refresh()
    }
  }
}
