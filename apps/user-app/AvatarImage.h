#pragma once

#include <QImage>
#include <QObject>
#include <QUrl>

// Owns a bounded, orientation-correct image while the user edits their avatar.
class AvatarImage : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString preview READ preview NOTIFY imageChanged)
  Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageChanged)
  Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageChanged)
public:
  explicit AvatarImage(QObject *parent = nullptr) : QObject(parent) {}
  QString preview() const { return m_preview; }
  int imageWidth() const { return m_image.width(); }
  int imageHeight() const { return m_image.height(); }
  Q_INVOKABLE bool load(const QUrl &url);
  Q_INVOKABLE QString crop(double x, double y, double side);
  Q_INVOKABLE void clear();
signals:
  void imageChanged();
  void failed(const QString &message);

private:
  QImage m_image;
  QString m_preview;
};
