#include "DataTable.h"
#include "Appearance.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollBar>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStyleOptionHeader>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <algorithm>

namespace adminui {
namespace {
constexpr int SortRole = Qt::UserRole + 1;
class SortableItem final : public QTableWidgetItem {
public:
  bool operator<(const QTableWidgetItem &other) const override {
    const auto left = data(SortRole);
    const auto right = other.data(SortRole);
    if (left.isNull() != right.isNull()) return left.isNull();
    if (left.metaType().id() != QMetaType::QString)
      return left.toDouble() < right.toDouble();
    return QString::localeAwareCompare(left.toString(), right.toString()) < 0;
  }
};
class FilterHeader final : public QHeaderView {
public:
  std::function<bool(int)> filtered;
  std::function<void(int, const QPoint &)> openFilter;

  explicit FilterHeader(QWidget *parent)
      : QHeaderView(Qt::Horizontal, parent) {}

protected:
  void paintSection(QPainter *painter, const QRect &rect,
                    int column) const override {
    if (!rect.isValid()) return;
    QStyleOptionHeader option;
    initStyleOption(&option);
    option.rect = rect;
    option.section = column;
    painter->save();
    style()->drawControl(QStyle::CE_HeaderSection, &option, painter, this);
    const bool sorted = isSortIndicatorShown()
                     && sortIndicatorSection() == column;
    const auto textRect = rect.adjusted(10, 0, sorted ? -48 : -30, 0);
    const auto text = model()->headerData(column, orientation()).toString();
    painter->setPen(palette().color(QPalette::WindowText));
    painter->drawText(
      textRect, Qt::AlignLeft | Qt::AlignVCenter,
      fontMetrics().elidedText(text, Qt::ElideRight, textRect.width()));
    painter->setRenderHint(QPainter::Antialiasing);
    if (sorted) {
      const QPointF center(rect.right() - 34, rect.center().y());
      const int direction = sortIndicatorOrder() == Qt::AscendingOrder ? 1 : -1;
      painter->setPen(QPen(palette().color(QPalette::PlaceholderText), 1.4));
      painter->drawPolyline(QPolygonF{center + QPointF(-3, 2 * direction),
                                      center + QPointF(0, -2 * direction),
                                      center + QPointF(3, 2 * direction)});
    }
    const QPointF center(rect.right() - 16, rect.center().y());
    QPainterPath funnel;
    funnel.moveTo(center + QPointF(-5, -4));
    funnel.lineTo(center + QPointF(5, -4));
    funnel.lineTo(center + QPointF(1, 0));
    funnel.lineTo(center + QPointF(1, 4));
    funnel.lineTo(center + QPointF(-1, 3));
    funnel.lineTo(center + QPointF(-1, 0));
    funnel.closeSubpath();
    const bool active = filtered(column);
    const auto color = palette().color(active ? QPalette::Highlight
                                              : QPalette::PlaceholderText);
    painter->setPen(QPen(color, 1.2));
    painter->setBrush(active ? QBrush(color) : Qt::NoBrush);
    painter->drawPath(funnel);
    painter->restore();
  }

  void mousePressEvent(QMouseEvent *event) override {
    const int column = logicalIndexAt(event->position().toPoint());
    if (event->button() == Qt::LeftButton && column >= 0
        && filterRect(column).contains(event->position().toPoint())) {
      pressedFilter = column;
      event->accept();
      return;
    }
    QHeaderView::mousePressEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent *event) override {
    if (pressedFilter >= 0) {
      const int column = pressedFilter;
      pressedFilter = -1;
      if (filterRect(column).contains(event->position().toPoint()))
        openFilter(column,
                   viewport()->mapToGlobal(filterRect(column).bottomLeft()));
      event->accept();
      return;
    }
    QHeaderView::mouseReleaseEvent(event);
  }

private:
  int pressedFilter = -1;
  QRect filterRect(int column) const {
    return QRect(sectionViewportPosition(column) + sectionSize(column) - 28, 0,
                 24, height());
  }
};

QString quoted(QString text) {
  text.replace('"', "\"\"");
  return '"' + text + '"';
}
} // namespace

QVariant cell(const QString &text, const QVariant &sort) {
  return QVariant::fromValue(TableValue{text, sort});
}

DataTable::DataTable(const QStringList &labels, const QString &name)
    : QTableWidget(0, labels.size()), headers(labels) {
  setObjectName(name);
  searchEdit = new QLineEdit(this);
  connect(searchEdit, &QLineEdit::textChanged, this, &DataTable::setSearch);
  auto *escapeSearch = new QShortcut(QKeySequence(Qt::Key_Escape), searchEdit);
  escapeSearch->setContext(Qt::WidgetShortcut);
  connect(escapeSearch, &QShortcut::activated, this, [this] {
    setSearch({});
    setFocus();
  });
  emptyState = new QWidget(viewport());
  emptyState->setObjectName("dataTableEmpty");
  auto *emptyLayout = new QVBoxLayout(emptyState);
  emptyLabel = new QLabel;
  emptyLabel->setObjectName("dataTableEmptyLabel");
  emptyLayout->addWidget(emptyLabel, 0, Qt::AlignHCenter);
  emptyClear = new QPushButton("清除筛选");
  emptyClear->setObjectName("dataTableEmptyClear");
  emptyClear->setFlat(true);
  emptyLayout->addWidget(emptyClear, 0, Qt::AlignHCenter);
  connect(emptyClear, &QPushButton::clicked, this, &DataTable::clearFilters);
  auto *viewportLayout = new QVBoxLayout(viewport());
  viewportLayout->addStretch();
  viewportLayout->addWidget(emptyState, 0, Qt::AlignHCenter);
  viewportLayout->addStretch();
  emptyState->hide();
  auto *header = new FilterHeader(this);
  header->filtered = [this](int column) {
    return filters.contains(column);
  };
  header->openFilter = [this](int column, const QPoint &position) {
    filterMenu(column, position);
  };
  setHorizontalHeader(header);
  setHorizontalHeaderLabels(headers);
  setAlternatingRowColors(false);
  setShowGrid(false);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSortingEnabled(true);
  sortItems(0, Qt::AscendingOrder);
  verticalHeader()->setDefaultSectionSize(28);
  horizontalHeader()->setStretchLastSection(true);
  horizontalHeader()->setSectionsClickable(true);
  horizontalHeader()->setSectionsMovable(true);
  horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  horizontalHeader()->setDefaultSectionSize(150);
  horizontalHeader()->setMinimumSectionSize(72);
  horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
  horizontalHeader()->setToolTip("点击列名排序，点击图标筛选，拖动调整列顺序");
  connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, this,
          [this](const QPoint &position) {
            const int column = horizontalHeader()->logicalIndexAt(position);
            if (column >= 0)
              filterMenu(column, horizontalHeader()->mapToGlobal(position));
          });
  actionsMenu = new QMenu(this);
  actionsMenu->setObjectName("dataTableActions");
  auto *copy = actionsMenu->addAction("复制\tCtrl+C", this, [this] {
    QApplication::clipboard()->setText(copyText());
  });
  auto *copyHeaders = actionsMenu->addAction(
    "复制并包含表头\tCtrl+Shift+C", this, [this] {
      QApplication::clipboard()->setText(copyText(true));
    });
  actionsMenu->addAction("全选\tCtrl+A", this, &DataTable::selectVisible);
  actionsMenu->addSeparator();
  actionsMenu->addAction("清除筛选", this, &DataTable::clearFilters);
  actionsMenu->addAction("导出 CSV", this, &DataTable::exportCsv);
  connect(actionsMenu, &QMenu::aboutToShow, this, [this, copy, copyHeaders] {
    const bool hasSelection = !selectedItems().isEmpty();
    copy->setEnabled(hasSelection);
    copyHeaders->setEnabled(hasSelection);
  });
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested, this,
          [this](const QPoint &position) {
            actionsMenu->exec(viewport()->mapToGlobal(position));
          });
  setMinimumHeight(140);
  auto shortcut = [this](const QKeySequence &key, auto action) {
    auto *binding = new QShortcut(key, this);
    binding->setContext(Qt::WidgetWithChildrenShortcut);
    connect(binding, &QShortcut::activated, this, action);
  };
  shortcut(QKeySequence::Copy, [this] {
    QApplication::clipboard()->setText(copyText());
  });
  shortcut(QKeySequence("Ctrl+Shift+C"), [this] {
    QApplication::clipboard()->setText(copyText(true));
  });
  shortcut(QKeySequence::SelectAll, [this] {
    selectVisible();
  });
}

QWidget *DataTable::panel(QHBoxLayout *actions) {
  auto *container = new QWidget;
  container->setObjectName("dataTablePanel");
  auto *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);
  auto *toolbarWidget = new QWidget;
  toolbarWidget->setObjectName("dataTableToolbar");
  auto *toolbar = new QHBoxLayout(toolbarWidget);
  toolbar->setContentsMargins(0, 0, 0, 0);
  toolbar->setSpacing(8);
  countLabel = new QLabel;
  countLabel->setObjectName("dataTableCount");
  if (actions) toolbar->addLayout(actions);
  toolbar->addStretch();
  searchEdit->setObjectName(objectName() + "Search");
  searchEdit->setPlaceholderText("搜索");
  searchEdit->setToolTip("搜索表格 · Ctrl+F");
  searchEdit->setClearButtonEnabled(true);
  searchEdit->setFixedWidth(180);
  toolbar->addWidget(searchEdit);
  auto *more = new QToolButton;
  more->setObjectName("dataTableMore");
  more->setText("⋯");
  more->setToolTip("更多操作");
  more->setPopupMode(QToolButton::InstantPopup);
  more->setMenu(actionsMenu);
  toolbar->addWidget(more);
  layout->addWidget(toolbarWidget);
  layout->addWidget(this, 1);
  layout->addWidget(countLabel, 0, Qt::AlignLeft);
  auto *find = new QShortcut(QKeySequence::Find, container);
  find->setContext(Qt::WidgetWithChildrenShortcut);
  connect(find, &QShortcut::activated, this, [this] {
    searchEdit->setFocus();
    searchEdit->selectAll();
  });
  auto *filter = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Down), container);
  filter->setContext(Qt::WidgetWithChildrenShortcut);
  connect(filter, &QShortcut::activated, this, [this] {
    const int column = currentColumn() >= 0
                       ? currentColumn()
                       : horizontalHeader()->sortIndicatorSection();
    const auto *header = horizontalHeader();
    filterMenu(column,
               header->viewport()->mapToGlobal(QPoint(
                 header->sectionViewportPosition(column), header->height())));
  });
  applyFilters();
  return container;
}

void DataTable::exportCsv() {
  const auto path = QFileDialog::getSaveFileName(
    this, "导出表格", objectName() + ".csv", "CSV (*.csv)");
  if (path.isEmpty()) return;
  QSaveFile file(path);
  const auto bytes = QByteArray("\xEF\xBB\xBF") + csvText().toUtf8();
  if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()
      || !file.commit())
    QMessageBox::warning(this, "导出失败", file.errorString());
}

QString DataTable::recordKey(int row) const {
  const auto record = item(row, 0)->data(Qt::UserRole).value<QJsonObject>();
  for (const auto *key : {"id", "date", "status"}) {
    if (record.contains(key)) return record[key].toVariant().toString();
  }
  return QString::fromUtf8(
    QJsonDocument(record).toJson(QJsonDocument::Compact));
}

void DataTable::fill(const QJsonArray &rows, const Columns &columns) {
  QMap<QString, QSet<int>> selection;
  for (const auto *selected : selectedItems())
    selection[recordKey(selected->row())].insert(selected->column());
  const auto currentKey = currentRow() < 0 ? QString()
                                           : recordKey(currentRow());
  const int previousColumn = currentColumn();
  const auto scroll = verticalScrollBar()->value();
  const auto sortColumn = horizontalHeader()->sortIndicatorSection();
  const auto sortOrder = horizontalHeader()->sortIndicatorOrder();
  setUpdatesEnabled(false);
  setSortingEnabled(false);
  clearContents();
  setRowCount(rows.size());
  for (int row = 0; row < rows.size(); ++row) {
    const auto record = rows[row].toObject();
    const auto values = columns(record);
    for (int column = 0; column < columnCount(); ++column) {
      auto *item = new SortableItem;
      const auto value = values[column];
      const bool formatted = value.metaType()
                          == QMetaType::fromType<TableValue>();
      item->setData(Qt::DisplayRole,
                    formatted ? value.value<TableValue>().text : value);
      item->setData(SortRole,
                    formatted ? value.value<TableValue>().sort : value);
      item->setToolTip(item->text());
      if (column == 0) item->setData(Qt::UserRole, record);
      setItem(row, column, item);
    }
  }
  setSortingEnabled(true);
  sortItems(sortColumn, sortOrder);
  clearSelection();
  setCurrentItem(nullptr);
  applyFilters();
  for (int row = 0; row < rowCount(); ++row) {
    if (isRowHidden(row)) continue;
    const auto key = recordKey(row);
    if (key == currentKey)
      setCurrentCell(row, previousColumn, QItemSelectionModel::NoUpdate);
    for (const auto column : selection.value(key))
      item(row, column)->setSelected(true);
  }
  verticalScrollBar()->setValue(scroll);
  setUpdatesEnabled(true);
}

void DataTable::setSearch(const QString &query) {
  const QSignalBlocker blocker(searchEdit);
  searchEdit->setText(query);
  search = query.trimmed();
  applyFilters();
}

void DataTable::setColumnFilter(int column, const QString &query,
                                const QString &operation,
                                const QSet<QString> &excluded) {
  if (query.isEmpty() && excluded.isEmpty())
    filters.remove(column);
  else
    filters[column] = {query, operation, excluded};
  applyFilters();
}

void DataTable::clearFilters() {
  filters.clear();
  setSearch({});
}

void DataTable::applyFilters() {
  int visible = 0;
  for (int row = 0; row < rowCount(); ++row) {
    bool matches = search.isEmpty();
    for (int column = 0; column < columnCount(); ++column)
      matches |= item(row, column)
                   ->text()
                   .contains(search, Qt::CaseInsensitive);
    for (auto it = filters.cbegin(); matches && it != filters.cend(); ++it) {
      const auto *value = item(row, it.key());
      const auto &filter = it.value();
      matches = !filter.excluded.contains(value->text());
      if (!matches || filter.query.isEmpty()) continue;
      if (filter.operation == "包含") {
        matches = value->text().contains(filter.query, Qt::CaseInsensitive);
      } else {
        const auto sort = value->data(SortRole);
        bool numeric;
        const auto number = filter.query.toDouble(&numeric);
        int compare;
        if (sort.metaType().id() != QMetaType::QString) {
          if (!numeric || sort.isNull()) {
            matches = false;
            continue;
          }
          compare = (sort.toDouble() > number) - (sort.toDouble() < number);
        } else
          compare = QString::compare(value->text(), filter.query,
                                     Qt::CaseInsensitive);
        if (filter.operation == "=") matches = compare == 0;
        if (filter.operation == "≠") matches = compare != 0;
        if (filter.operation == ">") matches = compare > 0;
        if (filter.operation == "≥") matches = compare >= 0;
        if (filter.operation == "<") matches = compare < 0;
        if (filter.operation == "≤") matches = compare <= 0;
      }
    }
    setRowHidden(row, !matches);
    if (matches)
      ++visible;
    else {
      for (int column = 0; column < columnCount(); ++column)
        item(row, column)->setSelected(false);
      if (currentRow() == row) setCurrentItem(nullptr);
    }
  }
  horizontalHeader()->viewport()->update();
  const bool filtering = !search.isEmpty() || !filters.isEmpty();
  if (countLabel)
    countLabel->setText(filtering
                          ? QString("%1 / %2 行").arg(visible).arg(rowCount())
                          : QString("%1 行").arg(rowCount()));
  emptyLabel->setText(filtering ? "没有匹配结果" : "暂无数据");
  emptyClear->setVisible(filtering);
  emptyState->setVisible(visible == 0);
}

void DataTable::filterMenu(int column, const QPoint &position) {
  QMenu menu;
  menu.setObjectName("columnFilterPopup");
  auto *form = new QWidget;
  form->setObjectName("tableFilter");
  form->setFixedWidth(288);
  auto *layout = new QVBoxLayout(form);
  layout->setContentsMargins(12, 10, 12, 10);
  layout->setSpacing(10);
  auto *top = new QHBoxLayout;
  top->setSpacing(4);
  auto *title = new QLabel(headers[column]);
  title->setObjectName("columnFilterTitle");
  title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  title->setToolTip(headers[column]);
  top->addWidget(title, 1);
  auto *sortGroup = new QButtonGroup(form);
  for (const auto order : {Qt::AscendingOrder, Qt::DescendingOrder}) {
    const bool ascending = order == Qt::AscendingOrder;
    auto *sort = new QToolButton;
    sort->setObjectName(ascending ? "columnSortAscending"
                                  : "columnSortDescending");
    const auto label = ascending ? "升序" : "降序";
    sort->setAccessibleName(label);
    sort->setToolTip(label);
    auto updateIcon = [sort, ascending] {
      auto *appearance = Appearance::instance();
      const auto
        color = appearance->colors().value("primaryText").value<QColor>();
      const auto scale = sort->devicePixelRatioF();
      QPixmap pixmap(QSize(24, 24) * scale);
      pixmap.setDevicePixelRatio(scale);
      pixmap.fill(Qt::transparent);
      QPainter painter(&pixmap);
      painter.setRenderHint(QPainter::Antialiasing);
      painter.setPen(
        QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      painter.drawLine(QPointF(7, 5), QPointF(7, 19));
      const auto tip = ascending ? 5 : 19;
      const auto shoulder = ascending ? 9 : 15;
      painter.drawPolyline(QPolygonF{QPointF(3, shoulder), QPointF(7, tip),
                                     QPointF(11, shoulder)});
      for (int index = 0; index < 3; ++index) {
        const int width = ascending ? 3 + index * 2 : 7 - index * 2;
        painter.drawLine(QPointF(14, 6 + index * 6),
                         QPointF(14 + width, 6 + index * 6));
      }
      painter.end();
      sort->setIcon(QIcon(pixmap));
    };
    connect(Appearance::instance(), &Appearance::changed, sort, updateIcon);
    updateIcon();
    sort->setIconSize(QSize(20, 20));
    sort->setFixedSize(30, 30);
    sort->setCheckable(true);
    sort->setChecked(horizontalHeader()->sortIndicatorSection() == column
                     && horizontalHeader()->sortIndicatorOrder() == order);
    sortGroup->addButton(sort);
    top->addWidget(sort);
    connect(sort, &QToolButton::clicked, this, [this, column, order] {
      sortItems(column, order);
    });
  }
  layout->addLayout(top);
  auto *condition = new QHBoxLayout;
  condition->setSpacing(6);
  auto *operation = new QComboBox;
  operation->setObjectName("columnFilterOperation");
  operation->setAccessibleName("筛选关系");
  operation->setFixedWidth(76);
  operation->addItems({"包含", "=", "≠", ">", "≥", "<", "≤"});
  const auto filter = filters.value(column);
  operation->setCurrentText(filter.operation.isEmpty() ? "包含"
                                                       : filter.operation);
  condition->addWidget(operation);
  auto *query = new QLineEdit(filter.query);
  query->setObjectName("columnFilterQuery");
  query->setPlaceholderText("输入条件");
  query->setClearButtonEnabled(true);
  condition->addWidget(query, 1);
  layout->addLayout(condition);
  auto *choices = new QHBoxLayout;
  choices->setSpacing(10);
  auto *selectAll = new QCheckBox("全选");
  selectAll->setObjectName("columnSelectAll");
  selectAll->setTristate(true);
  choices->addWidget(selectAll);
  auto *valueSearch = new QLineEdit;
  valueSearch->setObjectName("columnValueSearch");
  valueSearch->setPlaceholderText("查找选项");
  valueSearch->setClearButtonEnabled(true);
  choices->addWidget(valueSearch, 1);
  layout->addLayout(choices);
  auto *values = new QListWidget;
  values->setObjectName("columnValues");
  QSet<QString> unique;
  for (int row = 0; row < rowCount(); ++row)
    unique.insert(item(row, column)->text());
  QStringList labels(unique.begin(), unique.end());
  labels.sort(Qt::CaseInsensitive);
  values->setFixedHeight(qBound(56, int(labels.size()) * 28, 224));
  for (const auto &text : labels) {
    auto *entry = new QListWidgetItem(text, values);
    entry->setCheckState(filter.excluded.contains(text) ? Qt::Unchecked
                                                        : Qt::Checked);
  }
  layout->addWidget(values);
  auto updateSelectAll = [values, selectAll] {
    int total = 0;
    int checked = 0;
    for (int i = 0; i < values->count(); ++i) {
      const auto *item = values->item(i);
      if (item->isHidden()) continue;
      ++total;
      if (item->checkState() == Qt::Checked) ++checked;
    }
    const QSignalBlocker blocker(selectAll);
    selectAll->setEnabled(total > 0);
    selectAll->setCheckState(checked == 0       ? Qt::Unchecked
                             : checked == total ? Qt::Checked
                                                : Qt::PartiallyChecked);
  };
  connect(values, &QListWidget::itemChanged, selectAll, updateSelectAll);
  connect(valueSearch, &QLineEdit::textChanged, values,
          [values, updateSelectAll](const QString &query) {
            for (int i = 0; i < values->count(); ++i)
              values->item(i)->setHidden(
                !values->item(i)->text().contains(query, Qt::CaseInsensitive));
            updateSelectAll();
          });
  connect(selectAll, &QCheckBox::clicked, values,
          [values, selectAll, updateSelectAll] {
            const bool checked = selectAll->checkState() != Qt::Unchecked;
            const QSignalBlocker blocker(values);
            for (int i = 0; i < values->count(); ++i)
              if (!values->item(i)->isHidden())
                values->item(i)->setCheckState(checked ? Qt::Checked
                                                       : Qt::Unchecked);
            updateSelectAll();
          });
  updateSelectAll();
  auto *footer = new QHBoxLayout;
  auto *reset = new QPushButton("清除");
  reset->setObjectName("resetColumnFilter");
  reset->setEnabled(filters.contains(column));
  connect(reset, &QPushButton::clicked, this, [this, column, &menu] {
    setColumnFilter(column, {});
    menu.close();
  });
  footer->addWidget(reset);
  footer->addStretch();
  auto *apply = new QPushButton("应用");
  apply->setObjectName("applyColumnFilter");
  footer->addWidget(apply);
  layout->addLayout(footer);
  auto accept = [this, column, query, operation, values, &menu] {
    QSet<QString> excluded;
    for (int i = 0; i < values->count(); ++i)
      if (values->item(i)->checkState() == Qt::Unchecked)
        excluded.insert(values->item(i)->text());
    setColumnFilter(column, query->text(), operation->currentText(), excluded);
    menu.close();
  };
  connect(apply, &QPushButton::clicked, this, accept);
  connect(query, &QLineEdit::returnPressed, this, accept);
  auto *action = new QWidgetAction(&menu);
  action->setDefaultWidget(form);
  menu.addAction(action);
  QTimer::singleShot(0, &menu, [query] {
    query->setFocus();
  });
  menu.exec(position);
}

void DataTable::selectVisible() {
  QItemSelection selection;
  for (int row = 0; row < rowCount(); ++row)
    if (!isRowHidden(row))
      selection.select(model()->index(row, 0),
                       model()->index(row, columnCount() - 1));
  selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

QString DataTable::copyText(bool includeHeaders) const {
  QSet<int> rows, columns;
  for (const auto *value : selectedItems()) {
    if (isRowHidden(value->row())) continue;
    rows.insert(value->row());
    columns.insert(value->column());
  }
  QList<int> rowOrder(rows.begin(), rows.end());
  std::sort(rowOrder.begin(), rowOrder.end());
  QStringList lines;
  auto line = [&](int row) {
    QStringList cells;
    for (int index = 0; index < columnCount(); ++index) {
      const auto column = horizontalHeader()->logicalIndex(index);
      if (!columns.contains(column)) continue;
      QString text = row < 0                         ? headers[column]
                   : item(row, column)->isSelected() ? item(row, column)->text()
                                                     : QString();
      if (text.contains('\t') || text.contains('\n') || text.contains('"'))
        text = quoted(text);
      cells.append(text);
    }
    return cells.join('\t');
  };
  if (rows.isEmpty()) return {};
  if (includeHeaders) lines.append(line(-1));
  for (const auto row : rowOrder) lines.append(line(row));
  return lines.join('\n');
}

QString DataTable::csvText() const {
  QStringList lines;
  for (int row = -1; row < rowCount(); ++row) {
    if (row >= 0 && isRowHidden(row)) continue;
    QStringList cells;
    for (int index = 0; index < columnCount(); ++index) {
      const auto column = horizontalHeader()->logicalIndex(index);
      cells.append(
        quoted(row < 0 ? headers[column] : item(row, column)->text()));
    }
    lines.append(cells.join(','));
  }
  return lines.join("\r\n") + "\r\n";
}
} // namespace adminui
