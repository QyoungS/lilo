#include "DashboardWidget.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSqlQuery>
#include <QSplitter>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

DashboardWidget::DashboardWidget(int userId, QWidget* parent)
    : QWidget(parent), m_userId(userId)
{
    setupUi();
    refresh();
}

static QGroupBox* makeCard(const QString& title, QLabel*& valueLabel, QWidget* parent) {
    auto* box = new QGroupBox(title, parent);
    valueLabel = new QLabel("—", box);
    valueLabel->setAlignment(Qt::AlignCenter);
    valueLabel->setStyleSheet("font-size: 18pt; font-weight: bold;");
    auto* l = new QVBoxLayout(box);
    l->addWidget(valueLabel);
    return box;
}

void DashboardWidget::setupUi() {
    auto* root = new QVBoxLayout(this);

    auto* cardsRow = new QHBoxLayout;
    cardsRow->addWidget(makeCard("총 잔액",        m_totalBalanceLabel, this));
    cardsRow->addWidget(makeCard("이번 달 수입",   m_monthIncomeLabel,  this));
    cardsRow->addWidget(makeCard("이번 달 지출",   m_monthExpenseLabel, this));
    root->addLayout(cardsRow);

    m_ratesLabel = new QLabel("환율 불러오는 중...", this);
    m_ratesLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_ratesLabel);

    setupCharts();

    auto* chartRow = new QHBoxLayout;
    chartRow->addWidget(m_barChartView);
    chartRow->addWidget(m_pieChartView);
    root->addLayout(chartRow, 1);
}

void DashboardWidget::setupCharts() {
    m_barChartView = new QChartView(this);
    m_pieChartView = new QChartView(this);
    static_cast<QChartView*>(m_barChartView)->setRenderHint(QPainter::Antialiasing);
    static_cast<QChartView*>(m_pieChartView)->setRenderHint(QPainter::Antialiasing);
    m_barChartView->setMinimumHeight(250);
    m_pieChartView->setMinimumHeight(250);
}

void DashboardWidget::refresh() {
    updateSummaryCards();
    updateBarChart();
    updatePieChart();
    fetchExchangeRates();
}

void DashboardWidget::updateSummaryCards() {
    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);

    q.prepare("SELECT COALESCE(SUM(balance),0) FROM accounts WHERE user_id = :uid");
    q.bindValue(":uid", m_userId);
    if (q.exec() && q.next())
        m_totalBalanceLabel->setText(QString("₩ %1").arg(q.value(0).toDouble(), 0, 'f', 0));

    q.prepare(R"(
        SELECT COALESCE(SUM(t.amount),0)
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND t.type='입금'
          AND strftime('%Y-%m', t.created_at)=strftime('%Y-%m','now','localtime')
    )");
    q.bindValue(":uid", m_userId);
    if (q.exec() && q.next())
        m_monthIncomeLabel->setText(QString("₩ %1").arg(q.value(0).toDouble(), 0, 'f', 0));

    q.prepare(R"(
        SELECT COALESCE(SUM(t.amount),0)
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND t.type='출금'
          AND strftime('%Y-%m', t.created_at)=strftime('%Y-%m','now','localtime')
    )");
    q.bindValue(":uid", m_userId);
    if (q.exec() && q.next())
        m_monthExpenseLabel->setText(QString("₩ %1").arg(q.value(0).toDouble(), 0, 'f', 0));
}

void DashboardWidget::updateBarChart() {
    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT strftime('%m', t.created_at) as month,
               SUM(CASE WHEN t.type='입금' THEN t.amount ELSE 0 END) as income,
               SUM(CASE WHEN t.type='출금' THEN t.amount ELSE 0 END) as expense
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid
          AND strftime('%Y', t.created_at)=strftime('%Y','now','localtime')
        GROUP BY month ORDER BY month
    )");
    q.bindValue(":uid", m_userId);

    auto* incomeSet  = new QBarSet("수입");
    auto* expenseSet = new QBarSet("지출");
    QStringList months;

    if (q.exec()) {
        while (q.next()) {
            months << q.value(0).toString() + "월";
            *incomeSet  << q.value(1).toDouble();
            *expenseSet << q.value(2).toDouble();
        }
    }

    if (months.isEmpty()) {
        months << "—";
        *incomeSet  << 0;
        *expenseSet << 0;
    }

    auto* series = new QBarSeries;
    series->append(incomeSet);
    series->append(expenseSet);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle("월별 수입 / 지출");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* axisX = new QBarCategoryAxis;
    axisX->append(months);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    static_cast<QChartView*>(m_barChartView)->setChart(chart);
}

void DashboardWidget::updatePieChart() {
    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT t.category, SUM(t.amount)
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND t.type='출금'
          AND strftime('%Y-%m', t.created_at)=strftime('%Y-%m','now','localtime')
        GROUP BY t.category ORDER BY 2 DESC
    )");
    q.bindValue(":uid", m_userId);

    auto* series = new QPieSeries;
    if (q.exec()) {
        while (q.next())
            series->append(q.value(0).toString(), q.value(1).toDouble());
    }
    if (series->count() == 0)
        series->append("데이터 없음", 1);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle("카테고리별 지출 (이번 달)");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setAlignment(Qt::AlignRight);

    static_cast<QChartView*>(m_pieChartView)->setChart(chart);
}

void DashboardWidget::fetchExchangeRates() {
    auto* nam   = new QNetworkAccessManager(this);
    auto* reply = nam->get(QNetworkRequest(QUrl("https://open.er-api.com/v6/latest/KRW")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            m_ratesLabel->setText("환율 정보를 가져올 수 없습니다");
            reply->deleteLater();
            return;
        }
        auto doc   = QJsonDocument::fromJson(reply->readAll());
        auto rates = doc.object()["rates"].toObject();
        m_ratesLabel->setText(
            QString("USD: %1  |  JPY: %2  |  EUR: %3")
                .arg(rates["USD"].toDouble(), 0, 'f', 6)
                .arg(rates["JPY"].toDouble(), 0, 'f', 6)
                .arg(rates["EUR"].toDouble(), 0, 'f', 6));
        reply->deleteLater();
    });
}
