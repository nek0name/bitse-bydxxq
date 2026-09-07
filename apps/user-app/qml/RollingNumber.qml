import QtQuick

Item {
  id: root
  property real value: 0
  property int decimals: 0
  property string suffix: ''
  property alias font: current.font
  property alias color: current.color
  property bool ready: false
  property real previousValue: value
  readonly property string formatted: value.toFixed(decimals) + suffix
  implicitWidth: animation.running ? Math.max(current.implicitWidth, outgoing.implicitWidth) : current.implicitWidth
  implicitHeight: current.implicitHeight
  baselineOffset: current.baselineOffset
  clip: true

  Component.onCompleted: {
    current.text = formatted
    previousValue = value
    ready = true
  }
  onFormattedChanged: {
    if (!ready) return
    if (current.text === formatted) return
    animation.stop()
    outgoing.text = current.text
    current.text = formatted
    const direction = value >= previousValue ? 1 : -1
    previousValue = value
    incomingMove.from = direction * height
    outgoingMove.to = -direction * height
    animation.restart()
  }
  AppText {
    id: current
    anchors.horizontalCenter: parent.horizontalCenter
  }
  AppText {
    id: outgoing
    anchors.horizontalCenter: parent.horizontalCenter
    font: current.font
    color: current.color
    opacity: 0
  }
  ParallelAnimation {
    id: animation
    NumberAnimation {
      id: incomingMove
      target: current
      property: 'y'
      to: 0
      duration: 260
      easing.type: Easing.OutCubic
    }
    NumberAnimation {
      id: outgoingMove
      target: outgoing
      property: 'y'
      from: 0
      duration: 260
      easing.type: Easing.OutCubic
    }
    NumberAnimation {
      target: outgoing
      property: 'opacity'
      from: 1
      to: 0
      duration: 220
    }
  }
}
