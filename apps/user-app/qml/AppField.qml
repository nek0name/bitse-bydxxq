import QtQuick
import QtQuick.Controls

TextField {
  id: control
  implicitHeight: 56
  font.pixelSize: Theme.bodyLargeSize
  color: enabled ? Theme.ink : Theme.disabledText
  placeholderTextColor: Theme.muted
  leftPadding: Theme.cardPadding
  rightPadding: Theme.cardPadding
  selectByMouse: true
  selectionColor: Theme.primaryLight
  selectedTextColor: Theme.ink
  Accessible.name: placeholderText
  readonly property var scrollView: {
    let view = parent
    while (view && view.contentY === undefined) view = view.parent
    return view
  }
  function ensureVisible() {
    if (!activeFocus) return
    const view = scrollView
    if (!view || !view.contentItem) return
    const top = mapToItem(view.contentItem, 0, 0).y
    const maximum = Math.max(0, view.contentHeight - view.height)
    view.contentY = Math.min(maximum, Math.max(0,
      Math.min(top - 16, Math.max(view.contentY, top + height + 16 - view.height))))
  }
  onActiveFocusChanged: Qt.callLater(ensureVisible)
  Connections {
    target: control.scrollView
    ignoreUnknownSignals: true
    // IME insets resize only the scene content on Android 30+. Keep focused
    // fields visible after that layout settles without changing its height.
    function onHeightChanged() { Qt.callLater(control.ensureVisible) }
  }
  Connections {
    target: Qt.inputMethod
    function onVisibleChanged() { Qt.callLater(control.ensureVisible) }
    function onKeyboardRectangleChanged() { Qt.callLater(control.ensureVisible) }
  }
  background: Rectangle {
    color: control.enabled ? Theme.card : Theme.disabled
    radius: 8
    Rectangle {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      height: 2
      color: Theme.primary
      visible: control.activeFocus
    }
  }
}
