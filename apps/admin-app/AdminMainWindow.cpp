#include "AdminMainWindow.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
constexpr int kMargin = 12;
constexpr int kSpacing = 8;

QFrame *createMetric(const QString &label, const QString &value) {
    auto *frame = new QFrame;
    frame->setFrameShape(QFrame::StyledPanel);
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(2);
    layout->addWidget(new QLabel(label));
    layout->addWidget(new QLabel(value));
    return frame;
}

QWidget *createOverviewPage() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(kMargin, kMargin, kMargin, kMargin);
    layout->setSpacing(kSpacing);
    auto *headingRow = new QHBoxLayout;
    auto *heading = new QLabel("运营概览");
    QFont headingFont = heading->font();
    headingFont.setPointSize(headingFont.pointSize() + 3);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    headingRow->addWidget(heading);
    headingRow->addStretch(1);
    headingRow->addWidget(new QLabel("数据更新时间：尚未同步"));
    headingRow->addWidget(new QPushButton("刷新"));
    layout->addLayout(headingRow);

    auto *metrics = new QGridLayout;
    metrics->setSpacing(kSpacing);
    metrics->addWidget(createMetric("今日营收", "¥ 0.00"), 0, 0);
    metrics->addWidget(createMetric("今日订单", "0"), 0, 1);
    metrics->addWidget(createMetric("在线电桩", "0 / 0"), 0, 2);
    metrics->addWidget(createMetric("设备健康度", "--"), 0, 3);
    metrics->setColumnStretch(0, 1);
    metrics->setColumnStretch(1, 1);
    metrics->setColumnStretch(2, 1);
    metrics->setColumnStretch(3, 1);
    layout->addLayout(metrics);

    auto *contentRow = new QHBoxLayout;
    auto *trend = new QFrame;
    trend->setFrameShape(QFrame::StyledPanel);
    auto *trendLayout = new QVBoxLayout(trend);
    trendLayout->addWidget(new QLabel("近 7 日经营趋势"));
    trendLayout->addWidget(new QLabel("图表组件将在数据接口接入后显示"), 1,
                           Qt::AlignCenter);
    auto *operations = new QFrame;
    operations->setFrameShape(QFrame::StyledPanel);
    auto *operationsLayout = new QVBoxLayout(operations);
    operationsLayout->addWidget(new QLabel("设备与待办"));
    operationsLayout->addWidget(new QLabel("故障设备：0"));
    operationsLayout->addWidget(new QLabel("峰值预警：0"));
    operationsLayout->addWidget(new QLabel("待处理事项：0"));
    operationsLayout->addStretch(1);
    operationsLayout->addWidget(new QPushButton("查看设备状态"));
    contentRow->addWidget(trend, 2);
    contentRow->addWidget(operations, 1);
    layout->addLayout(contentRow, 1);
    return page;
}

QWidget *createTablePage(const QString &title, const QStringList &headers,
                         const QString &primaryAction) {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(kMargin, kMargin, kMargin, kMargin);
    layout->setSpacing(kSpacing);
    auto *heading = new QLabel(title);
    QFont headingFont = heading->font();
    headingFont.setPointSize(headingFont.pointSize() + 2);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    layout->addWidget(heading);

    auto *toolbar = new QHBoxLayout;
    auto *search = new QLineEdit;
    search->setClearButtonEnabled(true);
    search->setPlaceholderText("输入关键词搜索");
    auto *statusFilter = new QComboBox;
    statusFilter->addItems({"全部状态", "正常", "使用中", "故障"});
    toolbar->addWidget(search, 1);
    toolbar->addWidget(statusFilter);
    toolbar->addStretch(1);
    toolbar->addWidget(new QPushButton("刷新"));
    toolbar->addWidget(new QPushButton(primaryAction));

    auto *table = new QTableWidget(0, headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addLayout(toolbar);
    layout->addWidget(table, 1);

    auto *footer = new QHBoxLayout;
    footer->addWidget(new QLabel("共 0 条记录"));
    footer->addStretch(1);
    auto *previous = new QPushButton("上一页");
    previous->setEnabled(false);
    auto *next = new QPushButton("下一页");
    next->setEnabled(false);
    footer->addWidget(previous);
    footer->addWidget(new QLabel("第 1 / 1 页"));
    footer->addWidget(next);
    layout->addLayout(footer);
    return page;
}
} // namespace

AdminMainWindow::AdminMainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("充电运营管理端");
    resize(1100, 720);
    setMinimumSize(720, 480);
    const QStringList navigationLabels{"运营概览", "充电站", "充电桩",
                                       "用户管理", "负荷预测"};

    auto *central = new QWidget;
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    auto *navigation = new QFrame;
    navigation->setFrameShape(QFrame::StyledPanel);
    auto *navigationLayout = new QVBoxLayout(navigation);
    navigationLayout->setContentsMargins(6, 8, 6, 8);
    navigationLayout->setSpacing(4);
    navigationLayout->addWidget(new QLabel("运营管理"));
    navigationLayout->addWidget(new QLabel("管理员：未登录"));

    navigationGroup_ = new QButtonGroup(this);
    for (int index = 0; index < navigationLabels.size(); ++index) {
        auto *button = new QPushButton(navigationLabels.at(index));
        button->setCheckable(true);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        navigationLayout->addWidget(button);
        navigationGroup_->addButton(button, index);
    }
    navigationGroup_->button(0)->setChecked(true);
    navigationLayout->addStretch(1);

    auto *pages = new QStackedWidget;
    pages->addWidget(createOverviewPage());
    pages->addWidget(createTablePage(
      "充电站管理", {"名称", "区域", "电桩数", "状态"}, "新增电站"));
    pages->addWidget(createTablePage(
      "充电桩管理", {"编号", "所属站点", "类型", "功率", "状态"}, "新增电桩"));
    pages->addWidget(createTablePage(
      "用户管理", {"手机号", "昵称", "余额", "账号状态"}, "冻结/解冻"));
    pages->addWidget(createTablePage(
      "负荷预测", {"站点", "预测时段", "预测负荷", "预计空闲", "预警"},
      "重新训练"));
    connect(navigationGroup_, &QButtonGroup::idClicked, pages,
            &QStackedWidget::setCurrentIndex);

    rootLayout->addWidget(navigation);
    rootLayout->addWidget(pages, 1);
    setCentralWidget(central);
    statusBar()->showMessage("服务未连接 · 当前显示占位数据");
}
