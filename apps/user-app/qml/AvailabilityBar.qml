import QtQuick

Item {
  id: bar
  required property int total
  required property int available
  required property int faults
  readonly property color availableColor: appearance.dark ? '#82C96C' : '#4A984B'
  readonly property color unavailableColor: appearance.dark ? '#686976' : '#9697A2'
  implicitHeight: 6
  Accessible.name: '可用 ' + available + '，占用或不可用 ' + (total - available - faults) + '，故障或异常 ' + faults

  Rectangle {
    anchors.fill: parent
    color: bar.unavailableColor
    radius: height / 2
    visible: bar.total === 0
  }
  Row {
    anchors.fill: parent
    Repeater {
      model: bar.total
      delegate: Item {
        required property int index
        width: bar.width / bar.total
        height: bar.height
        Rectangle {
          width: parent.width - (index < bar.total - 1 ? Math.min(2, parent.width * 0.03) : 0)
          height: parent.height
          radius: index === 0 || index === bar.total - 1 ? height / 2 : 0
          color: index < bar.available ? bar.availableColor : index >= bar.total - bar.faults ? Theme.danger : bar.unavailableColor
          Rectangle {
            visible: bar.total > 1 && (index === 0 || index === bar.total - 1)
            anchors.right: index === 0 ? parent.right : undefined
            anchors.left: index !== 0 ? parent.left : undefined
            width: parent.width / 2
            height: parent.height
            color: parent.color
          }
        }
      }
    }
  }
}
