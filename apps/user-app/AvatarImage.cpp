#include "AvatarImage.h"

#include <QBuffer>
#include <QFile>
#include <QImageReader>
#include <QtMath>
#include <cmath>

bool AvatarImage::load(const QUrl &url) {
  clear();
  // QFile on Android understands content:// URIs granted by the system picker.
  QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());
  if (!file.open(QIODevice::ReadOnly)) {
    emit failed(QStringLiteral("无法读取这张照片，请重新选择"));
    return false;
  }
  constexpr qint64 maxSourceBytes = 32 * 1024 * 1024;
  if (file.size() > maxSourceBytes) {
    emit failed(QStringLiteral("照片不能超过 32 MB，请选择其他照片"));
    return false;
  }
  // Content providers may not report a reliable size. Bound the actual read
  // too.
  QByteArray source = file.read(maxSourceBytes + 1);
  if (source.size() > maxSourceBytes) {
    emit failed(QStringLiteral("照片不能超过 32 MB，请选择其他照片"));
    return false;
  }
  QBuffer input(&source);
  input.open(QIODevice::ReadOnly);
  QImageReader reader(&input);
  reader.setAutoTransform(true);
  const QSize size = reader.size();
  if (!size.isValid() || size.width() > 32768 || size.height() > 32768) {
    emit failed(QStringLiteral("图片尺寸无法读取，请选择其他照片"));
    return false;
  }
  // Some formats decode at full resolution before scaling. Check the source
  // pixel count before read(), using 64-bit multiplication to avoid overflow.
  if (qint64(size.width()) * size.height() > 64 * 1000 * 1000) {
    emit failed(QStringLiteral("照片不能超过 6400 万像素，请选择其他照片"));
    return false;
  }
  if (qMax(size.width(), size.height()) > 2048)
    reader.setScaledSize(size.scaled(2048, 2048, Qt::KeepAspectRatio));
  m_image = reader.read();
  if (m_image.isNull()) {
    emit failed(
      QStringLiteral("暂时无法打开这种图片，请选择 JPEG 或 PNG 照片"));
    return false;
  }
  QByteArray bytes;
  QBuffer output(&bytes);
  output.open(QIODevice::WriteOnly);
  m_image.save(&output, "PNG");
  m_preview = QStringLiteral("data:image/png;base64,")
            + QString::fromLatin1(bytes.toBase64());
  emit imageChanged();
  return true;
}

QString AvatarImage::crop(double x, double y, double side) {
  if (m_image.isNull() || !std::isfinite(x) || !std::isfinite(y)
      || !std::isfinite(side) || side <= 0)
    return {};
  // Coordinates are normalized against the decoded image, not its QML preview.
  const int length = qBound(1, qRound(side * m_image.width()),
                            qMin(m_image.width(), m_image.height()));
  const int left = qBound(0, qRound(x * m_image.width()),
                          m_image.width() - length);
  const int top = qBound(0, qRound(y * m_image.height()),
                         m_image.height() - length);
  const QImage result = m_image.copy(left, top, length, length)
                          .scaled(512, 512, Qt::IgnoreAspectRatio,
                                  Qt::SmoothTransformation)
                          .convertToFormat(QImage::Format_RGB32);
  QByteArray bytes;
  QBuffer output(&bytes);
  output.open(QIODevice::WriteOnly);
  if (!result.save(&output, "JPEG", 90)) {
    emit failed(QStringLiteral("头像处理失败，请重新选择"));
    return {};
  }
  return QString::fromLatin1(bytes.toBase64());
}

void AvatarImage::clear() {
  m_image = QImage();
  m_preview.clear();
  emit imageChanged();
}
