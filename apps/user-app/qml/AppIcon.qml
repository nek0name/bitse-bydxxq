import QtQuick

Image {
  property string name: ''
  property color color: Theme.ink
  width: Theme.iconSize
  height: Theme.iconSize
  source: name ? appearance.iconSource(':/icons/' + name + '.svg', color) : ''
  sourceSize.width: width * 2
  sourceSize.height: height * 2
  fillMode: Image.PreserveAspectFit
  Accessible.ignored: true
}
