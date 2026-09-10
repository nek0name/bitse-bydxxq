import QtQuick
import QtQuick.Layouts

RowLayout {
  id: money
  required property real cents
  property string suffix: ''
  property bool fitToWidth: false
  property color valueColor: Theme.primaryText
  property int valueSize: Theme.headlineSize
  spacing: Theme.microSpace
  baselineOffset: currency.y + currency.baselineOffset
  FontMetrics {
    id: valueMetrics
    font.pixelSize: money.valueSize
    font.weight: Font.DemiBold
  }
  AppText {
    id: currency
    text: '¥'
    font.pixelSize: Theme.bodySize
    color: money.valueColor
    Layout.alignment: Qt.AlignBaseline
  }
  RollingNumber {
    value: money.cents / 100
    decimals: 2
    font.pixelSize: money.fitToWidth
      ? Math.max(1, Math.floor(money.valueSize * Math.min(1,
          Math.max(1, money.width - currency.implicitWidth - (unit.visible ? unit.implicitWidth : 0) - money.spacing * 3)
          / Math.max(1, valueMetrics.advanceWidth((money.cents / 100).toFixed(2))))))
      : money.valueSize
    font.weight: Font.DemiBold
    color: money.valueColor
    Layout.alignment: Qt.AlignBaseline
  }
  AppText {
    id: unit
    visible: !!money.suffix
    text: money.suffix
    font.pixelSize: Theme.labelSize
    color: money.valueColor
    Layout.alignment: Qt.AlignBaseline
  }
  Item {
    visible: money.fitToWidth
    Layout.fillWidth: true
  }
}
