import QtQuick
import QtQuick.Layouts

RowLayout {
  id: money
  required property real cents
  property string suffix: ''
  property color valueColor: Theme.primaryText
  property int valueSize: Theme.headlineSize
  spacing: Theme.microSpace
  baselineOffset: currency.y + currency.baselineOffset
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
    font.pixelSize: money.valueSize
    font.weight: Font.DemiBold
    color: money.valueColor
    Layout.alignment: Qt.AlignBaseline
  }
  AppText {
    visible: !!money.suffix
    text: money.suffix
    font.pixelSize: Theme.labelSize
    color: money.valueColor
    Layout.alignment: Qt.AlignBaseline
  }
}
