#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QTableWidget>
#include <functional>

class QHBoxLayout;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;

namespace adminui {
struct TableValue {
  QString text;
  QVariant sort;
};
QVariant cell(const QString &text, const QVariant &sort);

class DataTable final : public QTableWidget {
public:
  using Columns = std::function<QVariantList(const QJsonObject &)>;
  DataTable(const QStringList &headers, const QString &name);
  QWidget *panel(QHBoxLayout *actions = nullptr);
  void fill(const QJsonArray &rows, const Columns &columns);
  void setSearch(const QString &query);
  void setColumnFilter(int column, const QString &query,
                       const QString &operation = "包含",
                       const QSet<QString> &excluded = {});
  void clearFilters();
  QString copyText(bool headers = false) const;
  QString csvText() const;

private:
  struct Filter {
    QString query;
    QString operation;
    QSet<QString> excluded;
  };
  QStringList headers;
  QString search;
  QMap<int, Filter> filters;
  QLineEdit *searchEdit = nullptr;
  QLabel *countLabel = nullptr;
  QMenu *actionsMenu = nullptr;
  QWidget *emptyState = nullptr;
  QLabel *emptyLabel = nullptr;
  QPushButton *emptyClear = nullptr;
  void applyFilters();
  void filterMenu(int column, const QPoint &position);
  void selectVisible();
  void exportCsv();
  QString recordKey(int row) const;
};
} // namespace adminui

Q_DECLARE_METATYPE(adminui::TableValue)
