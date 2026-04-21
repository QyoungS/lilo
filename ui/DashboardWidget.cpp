#include "DashboardWidget.h"
#include "DatabaseManager.h"
#include "AccountModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSqlQuery>
#include <QDateTime>
#include <QSizePolicy>
#include <QPainter>
#include <QProgressBar>
#include <QScrollArea>
#include <QHeaderView>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <QtWidgets>

// ─── 요약 카드 ───────────────────────────────────────────────
static QGroupBox* makeSummaryCard(const QString& title, QLabel*& valLabel,
                                  const QString& borderColor, const QString& bgColor,
                                  const QString& valueColor, QWidget* parent) {
    auto* box = new QGroupBox(parent);
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    box->setFixedHeight(94);
    box->setStyleSheet(QString(
        "QGroupBox { border:1px solid #E5E7EB; border-left:4px solid %1; "
        "border-radius:10px; background:%2; margin-top:0; }").arg(borderColor, bgColor));

    auto* titleLbl = new QLabel(title, box);
    QFont tf("맑은 고딕", 9, QFont::DemiBold);
    titleLbl->setFont(tf);
    titleLbl->setStyleSheet("color:#374151; background:transparent;");

    valLabel = new QLabel("₩0", box);
    QFont vf("맑은 고딕", 16, QFont::Bold);
    valLabel->setFont(vf);
    valLabel->setStyleSheet(
        QString("color:%1; background:transparent; letter-spacing:-0.5px;").arg(valueColor));
    valLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    valLabel->setFixedHeight(32);

    auto* l = new QVBoxLayout(box);
    l->setContentsMargins(16, 10, 16, 10);
    l->setSpacing(4);
    l->addWidget(titleLbl);
    l->addWidget(valLabel);
    return box;
}

// ════════════════════════════════════════════════════════════
DashboardWidget::DashboardWidget(int userId, QWidget* parent)
    : QWidget(parent), m_userId(userId)
{
    setupUi();
    refresh();
}

void DashboardWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

    // ── 1. 요약 카드 ─────────────────────────────────────────
    auto* cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(12);
    cardsRow->addWidget(makeSummaryCard(
        "총 자산",      m_totalBalanceLabel, "#1565C0", "#E3F2FD", "#1565C0", this));
    cardsRow->addWidget(makeSummaryCard(
        "이번 달 수입", m_monthIncomeLabel,  "#2E7D32", "#E8F5E9", "#2E7D32", this));
    cardsRow->addWidget(makeSummaryCard(
        "이번 달 지출", m_monthExpenseLabel, "#C62828", "#FFEBEE", "#C62828", this));
    root->addLayout(cardsRow);

    // ── 2. 최근 거래 내역 + 예산 대비 지출 ───────────────────
    auto* recentGroup = new QGroupBox("최근 거래 내역", this);
    recentGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    recentGroup->setFixedHeight(230);
    auto* recentLay = new QVBoxLayout(recentGroup);
    recentLay->setContentsMargins(8, 4, 8, 8);

    m_recentTxTable = new QTableWidget(0, 4, recentGroup);
    m_recentTxTable->setHorizontalHeaderLabels({"날짜", "계좌명", "유형", "금액"});
    m_recentTxTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_recentTxTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_recentTxTable->setColumnWidth(0, 140);
    m_recentTxTable->setColumnWidth(2, 55);
    m_recentTxTable->setColumnWidth(3, 110);
    m_recentTxTable->horizontalHeader()->setFont(QFont("맑은 고딕", 8, QFont::DemiBold));
    m_recentTxTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recentTxTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recentTxTable->verticalHeader()->hide();
    m_recentTxTable->setAlternatingRowColors(true);
    m_recentTxTable->setShowGrid(false);
    m_recentTxTable->setFrameShape(QFrame::NoFrame);
    m_recentTxTable->setFont(QFont("맑은 고딕", 9));
    recentLay->addWidget(m_recentTxTable);

    auto* budgetGroup = new QGroupBox("예산 대비 지출", this);
    budgetGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    budgetGroup->setFixedHeight(210);
    auto* budgetGroupLay = new QVBoxLayout(budgetGroup);
    budgetGroupLay->setContentsMargins(8, 4, 8, 8);

    auto* budgetScroll = new QScrollArea(budgetGroup);
    budgetScroll->setWidgetResizable(true);
    budgetScroll->setFrameShape(QFrame::NoFrame);
    m_budgetContainer = new QWidget;
    m_budgetLayout = new QVBoxLayout(m_budgetContainer);
    m_budgetLayout->setContentsMargins(4, 4, 4, 4);
    m_budgetLayout->setSpacing(10);
    budgetScroll->setWidget(m_budgetContainer);
    budgetGroupLay->addWidget(budgetScroll);

    auto* midRow = new QHBoxLayout;
    midRow->setSpacing(12);
    midRow->addWidget(recentGroup, 1);
    midRow->addWidget(budgetGroup, 1);
    root->addLayout(midRow);

    // ── 3. 차트 ──────────────────────────────────────────────
    setupCharts();

    auto* barGroup = new QGroupBox("월별 수입 / 지출", this);
    barGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* barLay = new QVBoxLayout(barGroup);
    barLay->setContentsMargins(8, 4, 8, 8);
    barLay->addWidget(m_barChartView);

    auto* pieGroup = new QGroupBox("이번 달 카테고리별 지출", this);
    pieGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* pieLay = new QVBoxLayout(pieGroup);
    pieLay->setContentsMargins(8, 4, 8, 8);
    pieLay->addWidget(m_pieChartView);

    auto* chartRow = new QHBoxLayout;
    chartRow->setSpacing(12);
    chartRow->addWidget(barGroup, 3);
    chartRow->addWidget(pieGroup, 2);
    root->addLayout(chartRow, 1);
}

void DashboardWidget::setupCharts() {
    auto* barView = new QChartView(this);
    auto* pieView = new QChartView(this);
    barView->setRenderHint(QPainter::Antialiasing);
    pieView->setRenderHint(QPainter::Antialiasing);
    barView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pieView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    barView->setMinimumHeight(180);
    pieView->setMinimumHeight(200);
    barView->setFrameShape(QFrame::NoFrame);
    pieView->setFrameShape(QFrame::NoFrame);
    m_barChartView = barView;
    m_pieChartView = pieView;
}

void DashboardWidget::refresh() {
    updateSummaryCards();
    updateBarChart();
    updatePieChart();
    updateRecentTransactions();
    updateBudgetProgress();
}

void DashboardWidget::updateSummaryCards() {
    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);

    auto fmt = [](double v) { return AccountModel::formatKRW(v); };

    q.prepare("SELECT COALESCE(SUM(balance),0) FROM accounts WHERE user_id=:uid");
    q.bindValue(":uid", m_userId);
    if (q.exec() && q.next()) m_totalBalanceLabel->setText(fmt(q.value(0).toDouble()));

    q.prepare(R"(SELECT COALESCE(SUM(t.amount),0)
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND t.type='입금'
          AND strftime('%Y-%m',t.created_at)=strftime('%Y-%m','now','localtime'))");
    q.bindValue(":uid", m_userId);
    if (q.exec() && q.next()) m_monthIncomeLabel->setText(fmt(q.value(0).toDouble()));

    q.prepare(R"(SELECT COALESCE(SUM(t.amount),0)
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND t.type='출금'
          AND strftime('%Y-%m',t.created_at)=strftime('%Y-%m','now','localtime'))");
    q.bindValue(":uid", m_userId);
    if (q.exec() && q.next()) m_monthExpenseLabel->setText(fmt(q.value(0).toDouble()));
}

void DashboardWidget::updateRecentTransactions() {
    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT t.created_at, a.name, t.type, t.amount
        FROM transactions t
        JOIN accounts a ON t.account_id = a.id
        WHERE a.user_id = :uid
        ORDER BY t.created_at DESC
        LIMIT 5
    )");
    q.bindValue(":uid", m_userId);

    m_recentTxTable->setRowCount(0);
    if (!q.exec()) return;

    int row = 0;
    while (q.next()) {
        m_recentTxTable->insertRow(row);

        QString rawDate = q.value(0).toString();
        QDateTime dt = QDateTime::fromString(rawDate, "yyyy-MM-dd HH:mm:ss");
        if (!dt.isValid()) dt = QDateTime::fromString(rawDate, Qt::ISODate);
        QString fmtDate = dt.isValid() ? dt.toString("MM/dd HH:mm") : rawDate.left(10);

        QString type   = q.value(2).toString();
        double  amount = q.value(3).toDouble();
        QColor  color  = (type == "입금") ? QColor("#059669") : QColor("#EF4444");

        auto* dateItem = new QTableWidgetItem(fmtDate);
        auto* acctItem = new QTableWidgetItem(q.value(1).toString());
        auto* typeItem = new QTableWidgetItem(type);
        typeItem->setForeground(color);
        typeItem->setTextAlignment(Qt::AlignCenter);

        auto* amtItem = new QTableWidgetItem(AccountModel::formatKRW(amount));
        amtItem->setForeground(color);
        amtItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_recentTxTable->setItem(row, 0, dateItem);
        m_recentTxTable->setItem(row, 1, acctItem);
        m_recentTxTable->setItem(row, 2, typeItem);
        m_recentTxTable->setItem(row, 3, amtItem);
        ++row;
    }
}

void DashboardWidget::updateBudgetProgress() {
    while (QLayoutItem* item = m_budgetLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT b.category, b.monthly_limit,
               COALESCE(SUM(t.amount), 0) AS spent
        FROM budgets b
        LEFT JOIN accounts a ON a.user_id = b.user_id
        LEFT JOIN transactions t ON t.account_id = a.id
            AND t.type = '출금'
            AND t.category = b.category
            AND strftime('%Y-%m', t.created_at) = strftime('%Y-%m', 'now', 'localtime')
        WHERE b.user_id = :uid
        GROUP BY b.id, b.category, b.monthly_limit
        ORDER BY b.category
    )");
    q.bindValue(":uid", m_userId);

    bool hasData = false;
    if (q.exec()) {
        while (q.next()) {
            hasData = true;
            QString category = q.value(0).toString();
            double  budget   = q.value(1).toDouble();
            double  spent    = q.value(2).toDouble();
            int     pct      = budget > 0 ? qMin((int)(spent * 100.0 / budget), 100) : 0;

            auto* rowWidget = new QWidget(m_budgetContainer);
            rowWidget->setMinimumHeight(52);
            auto* rowLay    = new QVBoxLayout(rowWidget);
            rowLay->setContentsMargins(0, 2, 0, 2);
            rowLay->setSpacing(4);

            auto* hdr    = new QHBoxLayout;
            auto* catLbl = new QLabel(category, rowWidget);
            catLbl->setStyleSheet("color:#374151; font-size:9pt; font-weight:600;");
            auto* amtLbl = new QLabel(
                QString("%1 / %2")
                    .arg(AccountModel::formatKRW(spent))
                    .arg(AccountModel::formatKRW(budget)),
                rowWidget);
            amtLbl->setStyleSheet("color:#6B7280; font-size:8pt;");
            hdr->addWidget(catLbl);
            hdr->addStretch();
            hdr->addWidget(amtLbl);

            auto* bar = new QProgressBar(rowWidget);
            bar->setRange(0, 100);
            bar->setValue(pct);
            bar->setTextVisible(true);
            bar->setFormat(QString::number(pct) + "%");
            bar->setFixedHeight(16);
            QString barColor = pct >= 100 ? "#EF4444" : pct >= 80 ? "#F59E0B" : "#3B82F6";
            bar->setStyleSheet(QString(
                "QProgressBar { background:#F3F4F6; border-radius:8px; border:none; "
                "height:16px; text-align:center; font-size:7pt; color:#374151; }"
                "QProgressBar::chunk { background:%1; border-radius:8px; }")
                .arg(barColor));

            rowLay->addLayout(hdr);
            rowLay->addWidget(bar);
            m_budgetLayout->addWidget(rowWidget);
        }
    }

    if (!hasData) {
        auto* emptyLbl = new QLabel("이번 달 등록된 예산이 없습니다.", m_budgetContainer);
        emptyLbl->setAlignment(Qt::AlignCenter);
        emptyLbl->setStyleSheet("color:#9CA3AF; font-size:9pt; padding:20px;");
        m_budgetLayout->addWidget(emptyLbl);
    }
    m_budgetLayout->addStretch();
}

void DashboardWidget::updateBarChart() {
    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT strftime('%m',t.created_at) as month,
               SUM(CASE WHEN t.type='입금'  THEN t.amount ELSE 0 END) as income,
               SUM(CASE WHEN t.type='출금' THEN t.amount ELSE 0 END) as expense
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND strftime('%Y',t.created_at)=strftime('%Y','now','localtime')
        GROUP BY month ORDER BY month)");
    q.bindValue(":uid", m_userId);

    auto* incomeSet  = new QBarSet("수입");
    auto* expenseSet = new QBarSet("지출");
    incomeSet->setColor(QColor("#059669"));
    incomeSet->setBorderColor(Qt::transparent);
    expenseSet->setColor(QColor("#EF4444"));
    expenseSet->setBorderColor(Qt::transparent);
    QStringList months;

    if (q.exec()) {
        while (q.next()) {
            months << q.value(0).toString() + "월";
            *incomeSet  << q.value(1).toDouble();
            *expenseSet << q.value(2).toDouble();
        }
    }
    if (months.isEmpty()) { months << "—"; *incomeSet << 0; *expenseSet << 0; }

    auto* series = new QBarSeries;
    series->append(incomeSet);
    series->append(expenseSet);
    series->setBarWidth(0.6);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(20, 10, 10, 10));

    auto* legend = chart->legend();
    legend->setAlignment(Qt::AlignTop);
    legend->setFont(QFont("맑은 고딕", 8));
    legend->setLabelColor(QColor("#374151"));
    legend->setMarkerShape(QLegend::MarkerShapeRectangle);

    auto* axisX = new QBarCategoryAxis;
    axisX->append(months);
    axisX->setLabelsFont(QFont("맑은 고딕", 8));
    axisX->setLabelsColor(QColor("#6B7280"));
    axisX->setGridLineVisible(false);
    axisX->setLinePenColor(QColor("#E5E7EB"));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setLabelsFont(QFont("맑은 고딕", 8));
    axisY->setLabelsColor(QColor("#6B7280"));
    axisY->setGridLineColor(QColor("#F3F4F6"));
    axisY->setLinePenColor(Qt::transparent);
    axisY->setTickCount(5);
    axisY->setLabelFormat("%'.0f");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    static_cast<QChartView*>(m_barChartView)->setChart(chart);
}

void DashboardWidget::updatePieChart() {
    static const QString kColors[] = {
        "#3B82F6", "#059669", "#F59E0B", "#EF4444",
        "#8B5CF6", "#06B6D4", "#F97316", "#EC4899"
    };

    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT t.category, SUM(t.amount)
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND t.type='출금'
          AND strftime('%Y-%m',t.created_at)=strftime('%Y-%m','now','localtime')
        GROUP BY t.category ORDER BY 2 DESC)");
    q.bindValue(":uid", m_userId);

    auto* series = new QPieSeries;
    series->setHoleSize(0.38);
    series->setPieSize(0.78);
    if (q.exec()) {
        while (q.next())
            series->append(q.value(0).toString(), q.value(1).toDouble());
    }
    if (series->count() == 0) series->append("데이터 없음", 1);

    int i = 0;
    for (auto* slice : series->slices()) {
        slice->setColor(QColor(kColors[i % 8]));
        slice->setBorderColor(QColor("#FFFFFF"));
        slice->setBorderWidth(2);
        slice->setLabelVisible(slice->percentage() > 0.05);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelFont(QFont("맑은 고딕", 9, QFont::Bold));
        slice->setLabelColor(QColor("#111827"));
        ++i;
    }

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(8, 8, 8, 8));

    auto* legend = chart->legend();
    legend->setAlignment(Qt::AlignBottom);
    legend->setFont(QFont("맑은 고딕", 9));
    legend->setLabelColor(QColor("#111827"));
    legend->setMarkerShape(QLegend::MarkerShapeCircle);

    static_cast<QChartView*>(m_pieChartView)->setChart(chart);
}
